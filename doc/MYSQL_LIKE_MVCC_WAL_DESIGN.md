# YourSQL 中实现 MySQL/InnoDB 风格 MVCC 的设计方案

## 1. 文档目标

这篇文档的目标不是讲一遍教科书版事务原理，而是回答一个更具体的问题：

> 在你当前这个 `YourSQL` 仓库里，应该怎样实现一套“接近 MySQL/InnoDB”的事务子系统，并且把 MVCC、undo、redo/WAL、隔离级别、锁、恢复这些东西串成一套能落地的方案。

这里强调的是“接近 MySQL/InnoDB”，不是逐字复刻 InnoDB 源码。

原因很直接：

- 你的存储层还不是 InnoDB 那种 B+Tree clustered index + secondary index 的完整结构
- 当前表页还是非常简化的 slotted page
- 事务、锁、redo、recovery 现在都还没有真正实现

因此，本文采用的策略是：

- 保留 InnoDB 的核心语义
- 在工程实现上做适合 `YourSQL` 的简化
- 明确指出哪些地方是“兼容 MySQL 思路”，哪些地方只是“第一版可接受的工程折中”

## 2. 先说结论：你应该选哪种 MVCC 模型

你前面已经写了一些 PostgreSQL 风格 MVCC 文档，但如果你想参考 MySQL/InnoDB，那么核心模型应该切换成下面这一套：

- 表页中保存“当前版本”
- 行头里保存事务 ID 和 undo 指针
- 历史版本不继续放在表页里，而是通过 undo log 还原
- 一致性读基于 `ReadView`
- `DELETE` 先做 delete-mark，而不是立刻物理删除
- 崩溃恢复依赖 redo log + undo rollback

这和 PostgreSQL 风格最本质的差异是：

- PostgreSQL：旧版本仍然是 heap tuple，本体就在表页里
- InnoDB：表页里更偏向保存当前版本，旧版本主要放在 undo 中

如果你的目标是“mysql 似的 mvcc”，那就不要再沿着 PostgreSQL 的 `xmin/xmax/t_ctid` 路线继续扩展；否则最后会变成两套模型混在一起，代码会很别扭。

## 3. 当前仓库现状

先基于当前代码说清楚现状，否则后面的设计都会漂。

### 3.1 已经有的东西

- 基础表页结构：
  - `src/include/storage/page/table_page.h`
  - `src/storage/page/table_page.cpp`
- tuple / row 序列化：
  - `src/include/storage/page/tuple.h`
  - `src/include/storage/page/row.h`
  - `src/storage/page/row.cpp`
- buffer manager / disk manager：
  - `src/include/buffer/buffer_manager.h`
  - `src/buffer/buffer_manager.cpp`
  - `src/include/storage/posix_disk_manager.h`
  - `src/storage/posix_disk_manager.cpp`
- 顺序扫描与插入执行器：
  - `src/executor/executor_seq_scan.cpp`
  - `src/executor/executor_insert.cpp`
- 事务头文件草稿：
  - `src/include/transaction/transaction.h`
  - `src/include/transaction/transaction_manager.h`
  - `src/include/transaction/read_view.h`
  - `src/include/transaction/undo_log.h`
  - `src/include/transaction/lock_manager.h`

### 3.2 现在还不具备事务语义的地方

当前代码和“mysql 似的 mvcc”直接冲突的点主要有这些：

- `TablePage::DeleteTuple()` 只是把 slot 上的 `deleted` 标记改掉
- `TablePage::updateTuple()` 仍然是原地覆盖 payload
- `Tuple` 目前只有用户数据，没有隐藏事务列
- `ExecutorContext` 里没有当前事务、事务管理器、锁管理器、日志管理器
- `TableIterator` / `SeqScan` 只会物理扫描，不会做可见性判断
- `BufferManager` 在 `Flush()` / `Release()` 时直接把页刷盘，没有 WAL 约束
- `PosixDiskManager` 只管理数据文件，没有单独的 redo 日志文件
- `ClientSession` 还是空的，无法维护 session 级事务和隔离级别

如果不把这些地方重构掉，你即使补上几个 `tx_id` 字段，也不会真的得到事务系统。

## 4. 这套设计要保留哪些 InnoDB 核心语义

根据 MySQL 官方文档，InnoDB 的关键点可以压缩成下面几条：

- 行记录会带隐藏字段，至少包括最近修改该行的事务 ID 和 undo 指针
- delete 在内部被当成一种特殊 update，先做 delete-mark
- 历史版本通过 undo log 重建，而不是直接把所有旧版本都留在主表页
- 一致性读依赖 read view
- redo log 用于崩溃后重放未落盘修改
- 崩溃恢复包含 redo roll-forward 和未完成事务 rollback
- 默认隔离级别是 `REPEATABLE READ`

这些语义是“mysql 似”的本质，不能丢。

## 5. 对 YourSQL 的总体设计建议

建议把事务子系统拆成五个模块：

1. `transaction/`
   - 事务对象、事务管理器、read view、事务状态表
2. `lock/`
   - 行锁、范围锁、等待图、死锁检测
3. `undo/`
   - undo 记录、undo page、purge
4. `log/`
   - redo/WAL、log buffer、checkpoint
5. `recovery/`
   - 启动恢复、redo 重放、undo 回滚 loser transactions

对当前项目来说，推荐的基本执行链路是：

1. session 打开事务或进入 autocommit 单语句事务
2. `TransactionManager` 分配事务 ID
3. 根据隔离级别决定是否创建 / 复用 `ReadView`
4. 执行器读取表页中的“当前版本”
5. 可见性层根据 `ReadView` 和 undo 链决定返回哪个版本
6. `INSERT` / `UPDATE` / `DELETE` 先写 undo，再改当前记录
7. 所有页修改都先产生日志记录，再允许脏页刷盘
8. `COMMIT` 刷新 commit log record
9. 后台 purge 清理不再需要的历史版本和 delete-mark 记录

## 6. 适配你当前代码的核心思路

你这个仓库并没有完整的 clustered index + secondary index 体系，所以第一版不要强行复制 InnoDB 的所有细节，而是保留最关键的行为：

- 把 `TablePage` 当成“主记录页”
- 主记录页里只保存当前版本
- 历史版本进入 undo 链
- 一致性读需要时从 undo 链回溯
- 更新主记录时允许页内搬移，但 `RID` 必须稳定

这意味着你需要把现在的 slot 从“删除位”模型改成“稳定逻辑行定位”模型：

- slot 负责稳定定位某个逻辑行
- slot 的 offset 可以随着页内 compaction 改变
- slot 不能再承担 MVCC 语义
- delete 是否可见由记录头和 undo 链决定，而不是 slot 的 `deleted`

## 7. 建议的数据结构

### 7.1 基础类型

建议先把事务相关类型显式化：

```cpp
using tx_id_t = uint64_t;
using lsn_t = uint64_t;
using undo_id_t = uint64_t;

constexpr tx_id_t INVALID_TX_ID = 0;
constexpr lsn_t INVALID_LSN = 0;
constexpr undo_id_t INVALID_UNDO_ID = 0;
```

`src/include/common/type.h` 里现在 `tx_id_t` 只是 `size_t` 别名，第一版虽然也能工作，但更推荐统一成固定宽度整数，避免不同平台行为不一致。

### 7.2 隔离级别

```cpp
enum class IsolationLevel : uint8_t {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
};
```

### 7.3 Transaction

`src/include/transaction/transaction.h` 建议扩展成：

```cpp
enum class TransactionState : uint8_t {
    IN_PROGRESS,
    COMMITTED,
    ABORTED
};

class Transaction {
public:
    tx_id_t tx_id_{INVALID_TX_ID};
    TransactionState state_{TransactionState::IN_PROGRESS};
    IsolationLevel isolation_{IsolationLevel::REPEATABLE_READ};

    bool autocommit_{true};
    bool read_only_{false};

    std::shared_ptr<ReadView> read_view_;

    std::vector<undo_id_t> undo_ids_;
    std::vector<RID> write_set_;
};
```

第一版不要把 `Transaction` 做得太复杂，先让它承担：

- 持有事务 ID
- 持有隔离级别
- 持有当前 `ReadView`
- 记录自己写过的 undo / row，便于回滚

### 7.4 ReadView

你已经有 `src/include/transaction/read_view.h`：

```cpp
tx_id_t create_trx_id_;
tx_id_t up_limit_id_;
tx_id_t low_limit_id_;
std::vector<tx_id_t> active_ids_;
```

这个字段命名和 MySQL 内部实现是能对上的，建议保留，但一定要在代码注释里写清楚语义：

- `create_trx_id_`
  - 创建这个 read view 的事务 ID
- `up_limit_id_`
  - 低水位，小于它的事务对该 view 一定可见
- `low_limit_id_`
  - 高水位，大于等于它的事务对该 view 一定不可见
- `active_ids_`
  - 拍快照时仍活跃的读写事务集合

注意这里最容易写反的是：

- `up_limit_id_` 其实是“低水位”
- `low_limit_id_` 其实是“高水位”

这和字段名直觉正好反着，但它和 MySQL 源码文档是一致的，所以建议统一保留，不要半路改命名。

### 7.5 当前记录头

你不应该继续把事务信息放在 `Tuple` 纯 payload 结构上，而是应该引入一个“物理记录头”：

```cpp
enum RecordFlag : uint16_t {
    RECORD_DELETE_MARK = 1 << 0,
    RECORD_COMPACT     = 1 << 1
};

struct RecordHeader {
    tx_id_t trx_id;
    undo_id_t roll_ptr;
    row_id_t row_id;
    uint16_t flags;
    uint16_t payload_size;
};
```

含义如下：

- `trx_id`
  - 最后一个插入或更新该记录的事务 ID
- `roll_ptr`
  - 指向最近一条 undo 记录
- `row_id`
  - 逻辑行 ID
- `flags`
  - 目前至少要支持 delete-mark
- `payload_size`
  - 当前版本的用户数据长度

和 InnoDB 对应关系是：

- `trx_id` 对应 `DB_TRX_ID`
- `roll_ptr` 对应 `DB_ROLL_PTR`
- `row_id` 对应 `DB_ROW_ID`

### 7.6 Undo 记录

你现在的 `UndoLogRecord` 只有：

```cpp
tx_id_t trx_id_;
Tuple old_;
RID prev_roll_id_;
UndoType type_;
```

这还不够。为了支持 consistent read 和 crash rollback，第一版建议存完整 before-image：

```cpp
enum class UndoType : uint8_t {
    INSERT,
    UPDATE,
    DELETE_MARK
};

struct UndoPointer {
    page_id_t page_id;
    uint32_t slot;
};

class UndoLogRecord {
public:
    undo_id_t undo_id_{INVALID_UNDO_ID};
    undo_id_t prev_undo_id_{INVALID_UNDO_ID};

    tx_id_t trx_id_{INVALID_TX_ID};
    UndoType type_;

    entry_id table_id_{0};
    RID rid_;

    tx_id_t old_trx_id_{INVALID_TX_ID};
    undo_id_t old_roll_ptr_{INVALID_UNDO_ID};
    uint16_t old_flags_{0};

    Tuple before_image_;
};
```

为什么第一版建议存完整 before-image，而不是只存修改列：

- 你当前执行器、表达式、存储层都还比较简单
- 完整 before-image 更容易实现 consistent read
- 回滚和恢复路径更简单，调试成本低很多

等系统跑通以后，再考虑像 InnoDB 一样做更细粒度的 undo 优化。

### 7.7 事务状态表

事务状态不能只靠活跃事务集合判断，否则 crash recovery 和可见性判断都不完整。

建议 `TransactionManager` 维护：

```cpp
std::atomic<tx_id_t> next_tx_id_{1};
std::unordered_map<tx_id_t, TransactionState> txn_state_;
std::unordered_map<tx_id_t, std::shared_ptr<Transaction>> active_txns_;
std::mutex mutex_;
```

第一版可以先做内存版。

如果后面要做真正崩溃恢复，需要把事务状态的一部分持久化到日志或系统页中。

## 8. `ReadView` 可见性规则

一致性读判断的是“当前记录头里的 `trx_id` 对这个 view 是否可见”；如果不可见，就顺着 `roll_ptr` 去 undo 链找旧版本。

### 8.1 规则定义

对于一个版本的修改事务 `version_trx_id`：

1. 如果 `version_trx_id == create_trx_id_`
   - 自己写的版本，对自己可见
2. 如果 `version_trx_id < up_limit_id_`
   - 一定可见
3. 如果 `version_trx_id >= low_limit_id_`
   - 一定不可见
4. 如果 `version_trx_id` 在 `active_ids_` 中
   - 说明拍快照时它还活跃，不可见
5. 否则
   - 可见

这正是你当前 `ReadView` 头文件适合承载的逻辑。

### 8.2 对 delete-mark 的判断

记录本身的 delete-mark 也要套相同逻辑。

可以这样理解：

- 当前记录头表示“最新状态”
- 如果它被某个事务标记为删除，而这个删除对当前 view 可见
  - 这条记录对本次查询不可见
- 如果这个删除对当前 view 不可见
  - 就继续顺着 undo 链回溯到一个更早版本

### 8.3 consistent read 的伪代码

```cpp
bool TryReadVisibleVersion(const RecordHeader &head,
                           const ReadView &view,
                           Tuple *result) {
    VersionImage img = LoadCurrentImage(head);

    while (true) {
        if (view.IsVisible(img.trx_id)) {
            if (img.delete_mark) {
                return false;
            }
            *result = img.payload;
            return true;
        }

        if (img.roll_ptr == INVALID_UNDO_ID) {
            return false;
        }

        img = ReconstructFromUndo(img.roll_ptr);
    }
}
```

第一版你完全可以把 `ReconstructFromUndo()` 做成“读完整 before-image 并覆盖当前 image”的方式，逻辑最稳定。

## 9. 写路径应该怎么做

### 9.1 `INSERT`

`INSERT` 的建议流程：

1. 创建事务
2. 在目标表页分配一个 slot
3. 写入当前记录头：
   - `trx_id = current_tx`
   - `roll_ptr = INVALID_UNDO_ID`
   - `delete_mark = false`
4. 写入 payload
5. 记录 insert undo
6. 记录 redo
7. 事务提交前该行只对自己可见，对其他事务不可见

关于 insert undo：

- InnoDB 的 insert undo 主要用于回滚，不用于一致性读
- 你第一版也建议保留 insert undo，因为 rollback 和 recovery 都会简单很多

### 9.2 `UPDATE`

`UPDATE` 不能继续使用当前 `TablePage::updateTuple()` 这种“直接覆盖 payload 然后挪内存”的语义。

正确流程应该是：

1. 先定位当前记录
2. 获取该行的 X 锁
3. 读取当前记录头和当前 payload
4. 写一条 update undo：
   - 保存旧 payload
   - 保存旧 `trx_id`
   - 保存旧 `roll_ptr`
   - 保存旧 flags
5. 修改主记录：
   - `trx_id = current_tx`
   - `roll_ptr = new_undo_id`
   - `delete_mark = false`
   - payload 改成新值
6. 写 redo

这就是 InnoDB 风格的“主记录保存最新版本，旧版本去 undo 链”。

### 9.3 `DELETE`

`DELETE` 不应该直接把 slot 标记为 deleted。

正确流程：

1. 获取该行 X 锁
2. 写 delete undo，保存删除前版本
3. 更新主记录头：
   - `trx_id = current_tx`
   - `roll_ptr = new_undo_id`
   - `delete_mark = true`
4. 写 redo
5. 提交后，该删除对后续 view 可见
6. purge 线程在安全时机再物理回收

### 9.4 `ROLLBACK`

rollback 不应该依赖“把页整体恢复到旧副本”，而应该按 undo 逐条回滚：

1. 逆序遍历事务自己的 undo 链
2. `INSERT`
   - 把主记录变成 delete-mark，或直接做物理删除
3. `UPDATE`
   - 恢复旧 payload、旧 `trx_id`、旧 `roll_ptr`、旧 flags
4. `DELETE`
   - 清除 delete-mark，恢复旧头信息
5. 写 abort redo

第一版实现里，回滚时允许直接恢复整行 before-image，不要一开始追求细节最优。

## 10. `TablePage` 应该怎么改

### 10.1 slot 结构

当前：

```cpp
struct Slot {
    uint16_t offset;
    uint16_t size;
    bool deleted;
};
```

建议改成：

```cpp
struct Slot {
    uint16_t offset;
    uint16_t size;
    uint16_t flags;
};
```

这里的 `flags` 只负责页内组织，比如 slot 是否空闲、是否重定向，不负责事务可见性。

### 10.2 表页接口

`TablePage` 至少要新增下面几类能力：

- 读取记录头
- 写入记录头
- 读取当前 payload
- 原子地更新“记录头 + payload”
- 追加 / 删除 / 重用 slot
- 页内 compact 之后修正 slot offset

建议增加如下 API：

```cpp
auto InsertRecord(const RecordHeader &header, const Tuple &tuple, RID *rid) -> bool;
auto ReadRecord(const RID &rid, RecordHeader *header, Tuple *tuple) -> bool;
auto UpdateRecord(const RID &rid, const RecordHeader &header, const Tuple &tuple) -> void;
auto MarkDeleted(const RID &rid, const RecordHeader &header) -> void;
auto PurgeRecord(const RID &rid) -> void;
```

`GetTuple()` 这种“默认返回用户 tuple”的 API 不再够用了，因为 MVCC 判断离不开记录头。

### 10.3 `RID` 稳定性

第一版一定要保证 `RID` 对逻辑记录稳定。

也就是说：

- 记录在页内 offset 可以变
- 但 `(page_id, row_id)` 不能因为一次 update 就变化

否则：

- undo 无法稳定指回主记录
- 行锁没法稳定挂载
- 索引项也会变得难处理

## 11. WAL / Redo Log 应该怎么设计

这一节很关键，因为你明确提到了 WAL。

### 11.1 当前代码的问题

现在的 `BufferManager` 有两个直接冲突点：

- `Flush(page_id)` 直接刷脏页
- `Release(page_id)` 里如果页脏也直接刷盘

这会破坏 WAL 规则，因为页可能先于日志落盘。

### 11.2 第一版 WAL 的目标

你不需要一开始把 redo 做到 InnoDB 那么复杂，但至少要满足下面三条：

1. 所有数据页修改都必须先生成 redo record
2. 页刷盘前，对应 redo 必须已经 durable
3. 事务 commit 成功返回前，commit record 必须 durable

只要做到这三条，你的系统才算有真正的 crash safety 基础。

### 11.3 建议的日志模块

建议新增：

- `src/include/log/log_record.h`
- `src/include/log/log_manager.h`
- `src/include/log/log_buffer.h`
- `src/log/log_manager.cpp`

基本结构可以这样：

```cpp
enum class LogRecordType : uint8_t {
    BEGIN,
    COMMIT,
    ABORT,
    INSERT_ROW,
    UPDATE_ROW,
    DELETE_MARK,
    INSERT_UNDO,
    PAGE_NEW,
    CHECKPOINT
};

struct LogRecordHeader {
    lsn_t lsn;
    tx_id_t tx_id;
    LogRecordType type;
    uint32_t payload_size;
};
```

### 11.4 页头必须增加 `page_lsn`

WAL 落地一定要有 `page_lsn`。

因此 `src/include/buffer/page.h` 里的 `Page` 建议加入：

```cpp
lsn_t page_lsn_{INVALID_LSN};
```

每次修改页时：

1. 先生成 redo record
2. `log_manager` 分配新的 LSN
3. 把该 LSN 写到页的 `page_lsn`
4. 标记页脏

### 11.5 页刷盘前的规则

当 `BufferManager::Flush()` 准备把页刷到数据文件时：

1. 取出页的 `page_lsn`
2. 如果 `durable_lsn < page_lsn`
   - 先把日志刷到 `page_lsn`
3. 再刷数据页

这就是你项目里真正的 WAL 约束点。

### 11.6 Commit 的规则

`COMMIT` 的建议流程：

1. 追加 `COMMIT` log record
2. flush log 到该记录的 LSN
3. 更新事务状态为 `COMMITTED`
4. 返回客户端

注意：

- 可以稍后再刷数据页
- 但 commit log 必须先 durable

### 11.7 日志文件

你当前默认数据文件是 `/data/ydb.sdb`。

建议第一版新增：

- `/data/ydb.wal`

只做一个 append-only 文件就够了，不必一上来就实现多段 redo 文件组。

## 12. Undo 和 Redo 的关系

事务系统里最容易混乱的问题之一，就是把 undo 和 redo 混成一件事。

你要牢牢记住：

- undo 解决的是：
  - 事务回滚
  - consistent read 看到旧版本
- redo 解决的是：
  - 崩溃后把已经发生但尚未刷盘的修改重放出来

### 12.1 一次 `UPDATE` 应该写什么

一次更新通常要写两类日志：

1. undo 记录
   - 保存旧版本
2. redo 记录
   - 记录“undo 页发生了什么变化”
   - 记录“数据页发生了什么变化”

所以不要把“有 undo 了”误解成“不需要 WAL”。

### 12.2 undo 页本身也要受 redo 保护

如果崩溃时：

- 主记录已经写了新值
- 但 undo 页还没持久化

那 recovery 时你就既无法 rollback，也无法 consistent read。

因此：

- undo page 也是普通脏页
- 对 undo page 的修改也必须写 redo

## 13. 崩溃恢复应该怎么做

MySQL/InnoDB 的恢复思路可以概括为：

1. 重启
2. redo roll-forward
3. rollback 未完成事务
4. 后台 purge 清理 delete-mark 和历史版本

你的项目第一版也建议走这个套路。

### 13.1 恢复所需持久化信息

至少要持久化：

- redo 日志
- undo 页面
- checkpoint 信息
- 数据页中的 `page_lsn`
- 事务结束记录（commit / abort）

### 13.2 恢复阶段

建议拆成三个阶段：

#### 阶段 A：加载 checkpoint

- 找到最近一次 checkpoint LSN
- 从该位置开始扫描 WAL

#### 阶段 B：redo

- 顺序扫描日志
- 对每条日志检查目标页的 `page_lsn`
- 若页上 LSN 小于日志 LSN，则重放

#### 阶段 C：undo loser transactions

- 扫描日志构建事务表
- 找出没有 `COMMIT` / `ABORT` 结束记录的事务
- 根据 undo 链执行回滚

### 13.3 你的第一版可以接受的简化

如果你想先尽快做出可运行版本，可以接受：

- 只有一个 WAL 文件
- 只有一个 checkpoint 文件
- 恢复时从 checkpoint 之后线性扫描 WAL
- 不实现 fuzzy checkpoint 的复杂脏页表

但不能省略：

- page LSN
- commit log durable
- loser transaction rollback

## 14. 隔离级别应该怎么落地

### 14.1 推荐的实现顺序

不要四个隔离级别一起上。

建议顺序：

1. `READ COMMITTED`
2. `REPEATABLE READ`
3. 锁定读 `SELECT ... FOR UPDATE / FOR SHARE`
4. `SERIALIZABLE`
5. 最后再考虑是否严格支持 `READ UNCOMMITTED`

原因是：

- `READ COMMITTED` 和 `REPEATABLE READ` 已经能把 MVCC 核心跑通
- `SERIALIZABLE` 需要更强的锁语义
- `READ UNCOMMITTED` 价值最低，却会让代码路径变脏

### 14.2 `READ COMMITTED`

规则：

- 每条语句创建一个新的 `ReadView`
- 普通 `SELECT` 做 consistent read
- `UPDATE` / `DELETE` / locking read 获取行锁
- gap lock 默认关闭，除了唯一性检查和外键检查这类特殊场景

对你这个项目来说，`READ COMMITTED` 是最容易先落地的。

### 14.3 `REPEATABLE READ`

规则：

- 第一次 consistent read 创建 `ReadView`
- 整个事务复用同一个 `ReadView`
- 普通 `SELECT` 在同一事务里结果可重复

这也是 InnoDB 默认隔离级别。

但是要注意：

- 仅有快照复用，还不等于完全防幻读
- 真正接近 InnoDB 的 phantom prevention，需要 next-key / gap locking 配合索引扫描

### 14.4 `READ UNCOMMITTED`

如果你要严格贴近 MySQL，这一级别意味着：

- 普通 `SELECT` 可以看到未提交版本
- 读出来的可能是脏数据

但从工程角度，我不建议你第一版就实现真正的 dirty read。

第一版可接受方案有两个：

1. 暂时不支持 `READ UNCOMMITTED`
2. 暂时把它等同于 `READ COMMITTED`，并在文档里明确注明“未完全兼容 MySQL”

如果是课程项目或个人数据库，第二种更稳。

### 14.5 `SERIALIZABLE`

这一层不要仅靠 MVCC 解决。

在 InnoDB 里，`SERIALIZABLE` 会把普通 `SELECT` 也提升成更强的锁定语义。

对 `YourSQL` 来说，第一版想做 `SERIALIZABLE`，至少需要：

- 记录锁
- 范围锁 / gap lock / next-key lock
- 等待和冲突处理

所以它应该排在 MVCC、undo、redo 之后。

## 15. 锁管理器应该怎么写

MVCC 不等于不要锁。

你现在的 `LockManager` 还是空的，而 mysql 风格事务系统没有锁是跑不起来的。

### 15.1 第一版需要的锁

至少要先支持：

- record S / X lock
- table intention lock

实现上可先用：

```cpp
struct LockDataId {
    entry_id table_id;
    RID rid;
};
```

### 15.2 第二版再加的锁

如果要逼近 InnoDB 的 `REPEATABLE READ` 范围行为，需要再加：

- gap lock
- next-key lock
- insert intention lock

这些锁应该挂在“索引键范围”上，而不是直接挂在行上。

### 15.3 没有索引时怎么办

你当前虽然有 `index/` 模块，但事务和执行器还没有把索引扫描完整纳入并发控制。

因此第一版可以这样处理：

- 点查更新：走 record lock
- 全表扫描更新：对命中的每条记录加 record X lock
- 范围幻读问题：先不承诺完全等价 InnoDB

这是工程上可以接受的。

但要在文档里写清楚：

- 第一版提供的是“mysql 风格 MVCC + 基础行锁”
- 不是完整的 InnoDB next-key locking

## 16. Purge / 垃圾回收

delete-mark 和 update undo 会不断留下历史垃圾。

如果没有 purge：

- 表会越来越大
- undo 链越来越长
- consistent read 越来越慢

### 16.1 purge 的目标

- 清理已经没有活跃 read view 需要的 update undo
- 物理删除已经 delete-mark 且对所有活跃事务都不可见的主记录
- 回收可重用 slot

### 16.2 什么时候可以 purge

需要一个“全局最老活跃视图边界”。

可以由 `TransactionManager` 提供：

- 所有活跃事务的最小 `up_limit_id_` 或等价边界

当某条 undo 或某个 delete-mark 版本早于这个全局边界，并且不再被任何活跃 `ReadView` 需要时，才能回收。

### 16.3 `MetaPage::UpdateTableRows()` 需要重新定义

当前 `meta_page_->UpdateTableRows()` 的含义更接近“立即可见的物理行数变化”。

但引入 MVCC 后：

- `DELETE` 不是立刻物理删除
- 未提交 `INSERT` 也不能直接算用户可见行数

所以建议：

- `MetaItem::num_rows_` 改成统计信息，不再做强一致语义
- 真正精确行数通过扫描或后续统计模块维护

否则你后面会不断遇到“元数据行数和查询结果对不上”的问题。

## 17. 对现有文件的具体改造建议

### 17.1 必改文件

- `src/include/common/type.h`
  - 补充固定宽度事务类型、LSN、undo ID
- `src/include/buffer/page.h`
  - 增加 `page_lsn`
- `src/include/storage/page/tuple.h`
  - 从纯 payload 容器扩展为可承载记录头的结构，或者新增 `Record` 类型
- `src/include/storage/page/table_page.h`
  - 移除 slot `deleted`
  - 新增 record header 读写接口
- `src/storage/page/table_page.cpp`
  - 重写 insert / update / delete / read
- `src/include/executor/executor_context.h`
  - 注入 `TransactionManager`、`LockManager`、`LogManager`
- `src/buffer/table_iterator.cpp`
  - 增加 MVCC 可见性逻辑
- `src/executor/executor_seq_scan.cpp`
  - 扫描时返回“对当前事务可见的版本”
- `src/executor/executor_insert.cpp`
  - 插入必须写 undo / redo

### 17.2 新增文件建议

- `src/include/transaction/isolation_level.h`
- `src/include/transaction/visibility.h`
- `src/transaction/transaction_manager.cpp`
- `src/transaction/read_view.cpp`
- `src/transaction/visibility.cpp`
- `src/include/log/log_record.h`
- `src/include/log/log_manager.h`
- `src/log/log_manager.cpp`
- `src/include/recovery/recovery_manager.h`
- `src/recovery/recovery_manager.cpp`

### 17.3 Session 层

还需要把 `ClientSession` 真的用起来。

建议 `ClientSession` 至少维护：

- 当前事务指针
- 当前隔离级别
- autocommit 状态

否则你没办法支持：

- `BEGIN`
- `COMMIT`
- `ROLLBACK`
- `SET TRANSACTION ISOLATION LEVEL ...`

## 18. 建议的分阶段开发顺序

### 阶段 1：先把事务外壳搭起来

目标：

- `Transaction`
- `TransactionManager`
- `ReadView`
- `ClientSession`
- `ExecutorContext` 注入事务上下文

完成标志：

- SQL 执行时能拿到当前事务
- 可以区分 autocommit 和显式事务

### 阶段 2：改表页格式，接入当前版本头

目标：

- `RecordHeader`
- stable `RID`
- slot 不再保存 deleted 语义
- `TablePage` 能读写当前版本头

完成标志：

- 表页里能正确存取 `trx_id` / `roll_ptr` / `delete_mark`

### 阶段 3：实现 undo 链和 consistent read

目标：

- undo page / undo record
- `ReadView::IsVisible`
- 扫描器支持从 undo 回溯旧版本
- `READ COMMITTED` / `REPEATABLE READ` 的普通 `SELECT`

完成标志：

- 一个事务更新后，另一个事务能基于 snapshot 看到旧版本

### 阶段 4：实现写路径与回滚

目标：

- `INSERT` / `UPDATE` / `DELETE`
- rollback
- 行锁

完成标志：

- 写写冲突能被阻塞或失败
- rollback 后主记录和 undo 链一致

### 阶段 5：接入 WAL / redo / recovery

目标：

- `page_lsn`
- redo log file
- commit flush
- crash recovery

完成标志：

- 进程异常退出后，重启能恢复到已提交状态
- 未提交事务会被回滚

### 阶段 6：purge 与更强隔离级别

目标：

- 后台 purge
- gap / next-key locking
- `SERIALIZABLE`

完成标志：

- delete-mark 和旧 undo 能被安全回收
- 范围更新 / 锁定读具备更接近 InnoDB 的行为

## 19. 你这个项目里最容易踩的坑

### 19.1 把 MVCC 元数据继续塞在 slot 里

这会很快把页管理和事务语义搅在一起，后面几乎没法维护。

### 19.2 `UPDATE` 只写 undo，不改当前记录头

这样 consistent read 虽然可能能跑，但当前版本链会断掉。

### 19.3 undo 不保存旧的 `roll_ptr` / `trx_id`

那你只能回滚一次，不能连续回溯多版本。

### 19.4 undo 有了，但不给 undo 页写 redo

一旦崩溃，你会丢历史版本，恢复路径直接断。

### 19.5 commit 只改内存状态，不刷 WAL

那系统就没有 durability。

### 19.6 以为只有 MVCC 就够了，不需要锁

没有 record lock：

- 两个事务会互相覆盖更新
- delete / update 竞态会非常混乱

### 19.7 在 `REPEATABLE READ` 下只复用 snapshot，不做范围锁

这样你只能得到“快照重复读”，但不等价于 InnoDB 默认的范围行为。

这不是不能接受，但必须明说。

## 20. 测试应该怎么设计

事务模块如果没有回归测试，后面几乎必炸。

建议至少补下面这些测试：

### 20.1 可见性测试

- T1 `BEGIN`
- T2 `INSERT`
- T1 `SELECT` 看不到
- T2 `COMMIT`
- `READ COMMITTED` 下 T1 下一条 `SELECT` 能看到
- `REPEATABLE READ` 下 T1 仍看不到

### 20.2 更新旧版本测试

- T1 读一行
- T2 更新并提交
- T1 在不同隔离级别下看到旧值或新值

### 20.3 delete-mark 测试

- T2 `DELETE`
- T1 基于旧 snapshot 仍可见
- purge 前物理记录仍在

### 20.4 rollback 测试

- `INSERT` 后 `ROLLBACK`
- `UPDATE` 后 `ROLLBACK`
- `DELETE` 后 `ROLLBACK`

### 20.5 崩溃恢复测试

- 写入后 commit，但不刷数据页，只刷 WAL
- 模拟进程崩溃
- 重启 recovery 后数据仍在

- 写入后未 commit
- 崩溃重启
- 事务应被回滚

## 21. 我对第一版范围的建议

如果你问我“现在最值得做的最小闭环是什么”，我的建议是：

1. 先做 `READ COMMITTED` + `REPEATABLE READ`
2. 先做 record lock，不急着做 gap / next-key lock
3. 先做完整 before-image undo
4. 先做单文件 append-only WAL
5. 先做 redo + loser transaction undo recovery

不要第一版就追求：

- 完整 InnoDB 二级索引 MVCC
- 完整 next-key locking
- 完整 `READ UNCOMMITTED`
- 高性能 partial undo
- 复杂 checkpoint / log group

先把“正确性闭环”打通，比一开始追求高度仿真更重要。

## 22. 这份方案和你现有 PostgreSQL 文档的关系

仓库里已经有这些文档：

- `doc/README.md`
- `doc/MVCC_EXPLAIN.md`
- `doc/POSTGRES_MVCC_AND_ISOLATION.md`

它们主要是 PostgreSQL 风格思路。

这份文档和它们的关系是：

- 不是替代 MVCC 原理文档
- 而是给“mysql 似的实现路线”单独立一个方案

如果你后面决定真的按 InnoDB 路线做，那么事务实现应以本文件为准，而不是继续把 PostgreSQL 的 `xmin/xmax/t_ctid` 往现有代码里硬塞。

## 23. 参考资料

下面这些结论主要参考 MySQL 官方文档，尤其是 InnoDB 的 multi-versioning、consistent read、isolation level、locking、redo、undo 和 recovery 章节：

- MySQL 8.4 Reference Manual, InnoDB Multi-Versioning:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-multi-versioning.html
- MySQL 8.4 Reference Manual, Consistent Nonlocking Reads:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-consistent-read.html
- MySQL 8.4 Reference Manual, Transaction Isolation Levels:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-transaction-isolation-levels.html
- MySQL 8.4 Reference Manual, InnoDB Locking:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-locking.html
- MySQL 8.4 Reference Manual, Redo Log:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-redo-log.html
- MySQL 8.4 Reference Manual, Undo Logs:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-undo-logs.html
- MySQL 8.4 Reference Manual, InnoDB Recovery:
  - https://dev.mysql.com/doc/refman/8.4/en/innodb-recovery.html
- MySQL Server Doxygen, `ReadView`:
  - https://dev.mysql.com/doc/dev/mysql-server/latest/classReadView.html

## 24. 最后给你的实现建议

如果你准备真正开始写代码，我建议你下一步严格按这个顺序推进：

1. 先补 `Transaction` / `TransactionManager` / `ClientSession`
2. 重写 `TablePage`，引入 `RecordHeader`
3. 实现 undo 和 `ReadView::IsVisible`
4. 让 `SeqScan` 跑通 consistent read
5. 再做 `UPDATE` / `DELETE`
6. 最后接 WAL / recovery

原因很简单：

- 如果你先写 WAL，但没有稳定的数据版本模型，日志格式会反复推翻
- 如果你先写锁，但没有 snapshot / undo，读路径还是错的
- 如果你先写复杂隔离级别，但没有最基础的 consistent read，最终只是“有锁无 MVCC”

先把版本链和读语义做对，整个事务模块才有根。
