# PostgreSQL 风格 MVCC 开发方案

## 1. 目标

本文档用于定义如何在 `YourSQL` 中实现一套尽量贴近 PostgreSQL 的 MVCC 机制。

这里的目标不是泛泛地做一个“多版本存储”，而是明确采用 PostgreSQL 那套核心模型：

- 版本元数据放在 tuple header 中
- `UPDATE` 不做原地覆盖，而是生成新版本
- `DELETE` 不立即物理删除，而是通过事务元数据控制可见性
- 读路径通过 `xmin/xmax` 和 snapshot 判断版本是否可见
- 旧版本不会立刻回收，而是交给后续的 prune / vacuum

这个方案是基于当前仓库现状写的：

- 目前没有事务管理器
- 当前存储层 tuple 只有用户数据，没有 MVCC header
- page slot 仍然使用 `deleted`
- 扫描器是物理扫描，没有 snapshot visibility
- 当前 update 路径是原地更新，这和 MVCC 模型冲突

## 2. 范围

### 本期纳入范围

- 事务管理器
- snapshot 结构
- 带 MVCC 元数据的 tuple header
- heap table 上的版本链
- `INSERT`、`DELETE`、`UPDATE` 的 MVCC 语义
- `SELECT` 的 snapshot 可见性判断
- 内存中的事务状态表
- 基础的 page prune / vacuum
- 配套回归测试

### 本期不纳入范围

- WAL / 崩溃恢复
- 落盘版事务状态页，类似 PostgreSQL 的 `pg_xact`
- Serializable 隔离级别
- HOT 优化
- 索引 vacuum
- 分布式事务

## 3. 设计原则

### 3.1 PostgreSQL 核心语义要保持

实现过程中，下面这些 PostgreSQL 的核心特征不能丢：

- tuple header 中记录创建事务和删除事务
- `UPDATE` 的本质是 delete + insert
- 旧版本通过 `t_ctid` 指向新版本
- 可见性由事务状态和 snapshot 共同决定
- 旧版本只有在所有活跃事务都不再需要时才能回收

### 3.2 可以结合当前工程做简化，但不能改模型

当前仓库还比较早期，允许做工程层面的简化，但不能把 PostgreSQL 风格 MVCC 改造成别的东西。

可以接受的简化：

- 事务状态表先放内存里
- 第一版不引入 commit timestamp，xid 状态足够支持 snapshot visibility
- 第一版可以先支持 autocommit
- vacuum 可以先做成手动触发

不能接受的简化：

- 把 MVCC 元数据只放在 slot 里
- update 继续做原地覆盖
- 读路径没有 snapshot 判断

## 4. 当前仓库的结构性缺口

下面这些现有结构和 PostgreSQL 风格 MVCC 是冲突的，不能直接套壳：

- `src/include/storage/page/table_page.h`
  - `Slot` 里有 `deleted`
  - page API 没有 tuple header 的访问能力
- `src/storage/page/table_page.cpp`
  - `DeleteTuple` 直接改物理删除标记
  - `updateTuple` 直接原地改 tuple 数据
- `src/include/storage/page/tuple.h`
  - 当前 `Tuple` 更像 payload 容器，没有 heap tuple header
- `src/include/executor/executor_context.h`
  - 没有 transaction、snapshot、transaction manager
- `src/buffer/table_iterator.cpp`
  - 当前只是物理扫描，没有 MVCC 过滤
- `src/executor/executor_seq_scan.cpp`
  - 返回的是物理 tuple，没有可见性判断

这几个点属于结构性阻塞项。MVCC 不应该绕着它们打补丁，而是要正面重构。

## 5. 总体架构

事务子系统建议作为一个独立模块引入：

- `transaction/transaction.h`
- `transaction/transaction_manager.h`
- `transaction/snapshot.h`
- `transaction/visibility.h`
- 如果回滚通过写集驱动，还需要 `transaction/undo_record.h`

存储层需要改成接近 heap table 的模型：

- tuple header 单独定义
- tuple payload 继续沿用 `Row` 的列序列化逻辑
- `TablePage` 负责存放 line pointer 和 heap tuple

执行链路应当变成：

1. executor 开启事务，或者从 session 中拿当前事务
2. transaction manager 分配 xid，并生成 snapshot
3. 扫描器从 page 中取出 heap tuple
4. visibility 层根据 snapshot 判断可见性
5. update/delete 修改的是 tuple header，而不是物理删除位
6. commit / abort 更新事务状态

## 6. 关键数据结构

### 6.1 事务标识

建议先引入显式的 xid 类型：

```cpp
using txn_id_t = uint32_t;
using command_id_t = uint32_t;

constexpr txn_id_t INVALID_TXN_ID = 0;
```

第一版用 `txn_id_t` 足够，不必一开始就引入更复杂的时间戳模型。

### 6.2 Snapshot

snapshot 建议尽量按 PostgreSQL 的语义来定义：

```cpp
struct Snapshot {
    txn_id_t xmin;
    txn_id_t xmax;
    std::vector<txn_id_t> xip;
};
```

含义：

- `xmin`：创建 snapshot 时系统中仍然活跃的最小事务 id
- `xmax`：创建 snapshot 时“下一个待分配 xid”，也就是可见性上界
- `xip`：创建 snapshot 时仍然处于运行中的事务集合

可见性规则：

- 已提交事务如果 `xid < xmin`，则对当前 snapshot 可见
- `xid >= xmax` 一律不可见
- 出现在 `xip` 中的事务说明拍快照时还没结束，对当前 snapshot 不可见
- 其他情况再结合事务状态判断

### 6.3 Transaction 对象

```cpp
enum class TxnState : uint8_t {
    IN_PROGRESS,
    COMMITTED,
    ABORTED
};

struct WriteRecord {
    enum class Type : uint8_t {
        INSERT,
        DELETE,
        UPDATE_OLD,
        UPDATE_NEW
    };

    Type type;
    RID rid;
    RID related_rid;
};

class Transaction {
public:
    txn_id_t txn_id_{INVALID_TXN_ID};
    command_id_t command_id_{0};
    TxnState state_{TxnState::IN_PROGRESS};
    Snapshot snapshot_{};
    std::vector<WriteRecord> write_set_;
};
```

这里的 `command_id_` 很重要。它不仅是为了兼容 PostgreSQL 的模型，也能保证“同一个事务内自己能看到自己的 insert，但看不到自己已经 delete 掉的版本”。

### 6.4 TransactionManager

```cpp
class TransactionManager {
public:
    auto Begin() -> std::shared_ptr<Transaction>;
    auto Commit(const std::shared_ptr<Transaction> &txn) -> void;
    auto Abort(const std::shared_ptr<Transaction> &txn) -> void;

    auto GetSnapshot() -> Snapshot;
    auto GetState(txn_id_t xid) const -> TxnState;
    auto IsCommitted(txn_id_t xid) const -> bool;
    auto IsAborted(txn_id_t xid) const -> bool;
    auto IsInProgress(txn_id_t xid) const -> bool;
    auto OldestActiveXmin() const -> txn_id_t;
};
```

第一版可以把事务状态全部放在内存里：

```cpp
std::atomic<txn_id_t> next_xid_;
std::unordered_map<txn_id_t, TxnState> txn_state_;
std::unordered_map<txn_id_t, std::weak_ptr<Transaction>> active_txns_;
std::mutex mutex_;
```

核心职责：

- 分配 xid
- 维护事务状态
- 生成 snapshot
- 提供事务状态查询
- 计算 vacuum 所需的 `OldestActiveXmin`

### 6.5 TupleHeader

tuple header 应该变成物理存储的一部分：

```cpp
enum TupleInfomask : uint16_t {
    HEAP_XMIN_COMMITTED = 1 << 0,
    HEAP_XMIN_INVALID   = 1 << 1,
    HEAP_XMAX_COMMITTED = 1 << 2,
    HEAP_XMAX_INVALID   = 1 << 3,
    HEAP_UPDATED        = 1 << 4,
    HEAP_DELETED        = 1 << 5
};

struct TupleHeader {
    txn_id_t xmin;
    txn_id_t xmax;
    command_id_t cmin;
    command_id_t cmax;
    uint16_t infomask;
    RID t_ctid;
};
```

语义如下：

- `xmin`：插入这个版本的事务
- `xmax`：删除这个版本，或者用新版本替换它的事务
- `cmin`：同一事务内插入该 tuple 时的命令号
- `cmax`：同一事务内删除或更新该 tuple 时的命令号
- `t_ctid`：最新版本时指向自己；发生 update 后，旧版本的 `t_ctid` 指向新版本

### 6.6 HeapTuple

当前的 `Tuple` 不应该继续承担“完整物理 tuple”这个职责。建议拆成两个概念：

- `Row`：逻辑行对象，给执行器和列序列化使用
- `HeapTuple`：真正落在 page 上的物理 tuple

建议结构：

```cpp
class HeapTuple {
public:
    TupleHeader header_{};
    Schema schema_{};
    char *data_{nullptr};
    uint16_t data_size_{0};

    auto Serialize() const -> std::vector<char>;
    auto Deserialize(const char *src, uint16_t size, const Schema &schema) -> void;
};
```

第一阶段可以暂时保留现有 `Tuple` 给 executor 输出使用，但存储层应该逐步切到 `HeapTuple`。

### 6.7 Slot / line pointer

`Slot.deleted` 必须废掉，因为 tuple 是否存活应该由 tuple header 决定，而不是 slot。

建议改成：

```cpp
enum class ItemState : uint8_t {
    UNUSED,
    NORMAL,
    REDIRECT,
    DEAD
};

struct Slot {
    uint16_t offset;
    uint16_t size;
    ItemState state;
};
```

语义：

- `NORMAL`：slot 指向一个真实 tuple
- `REDIRECT`：后续如果要做 redirect 或优化链，可以支持
- `DEAD`：这个 slot 里的 tuple 已经可回收

第一版不一定马上要支持 `REDIRECT`，但 `NORMAL/DEAD` 至少要有。

## 7. 物理存储布局

page 布局建议仍然保持当前方向：

```text
+---------------------------------------------------------------+
| page header | tuple data area 向前增长 | slots 向后增长       |
+---------------------------------------------------------------+
```

tuple 的字节布局改成：

```text
+----------------------+------------------------------+
| TupleHeader          | row payload                  |
+----------------------+------------------------------+
```

其中 row payload 仍然可以复用现在 `Row` 的序列化结果。

### 7.1 TableHeader

page header 当前结构可以先保留：

```cpp
struct TableHeader {
    uint16_t version;
    uint32_t num_rows;
    page_id_t page_id;
    page_id_t next_page_id;
};
```

MVCC 元数据不应该放在 page header 里，page header 仍然只负责页级别的组织信息。

## 8. 可见性规则

### 8.1 核心原则

一个 tuple 对 snapshot `S` 可见，当且仅当：

1. 它的插入事务 `xmin` 对 `S` 可见
2. 它的删除事务 `xmax` 对 `S` 不可见

### 8.2 事务对 snapshot 的可见性

建议抽一个工具函数：

```cpp
auto TxnVisibleInSnapshot(txn_id_t xid,
                          const Snapshot &snapshot,
                          const TransactionManager &txn_mgr) -> bool;
```

规则如下：

- `INVALID_TXN_ID` 不可见
- 如果 `xid < snapshot.xmin`，则只有事务已提交时才可见
- 如果 `xid >= snapshot.xmax`，不可见
- 如果 `xid` 存在于 `snapshot.xip` 中，说明拍快照时该事务还在运行，不可见
- 其他情况则只有事务已提交才可见

### 8.3 Tuple 可见性

建议再封一层：

```cpp
auto HeapTupleVisible(const TupleHeader &header,
                      const Transaction &txn,
                      const TransactionManager &txn_mgr) -> bool;
```

判断逻辑参考 PostgreSQL：

- 如果 `xmin` 是当前事务：
  - 只要当前事务还没把它删掉，这个版本对自己可见
- 如果 `xmin` 属于其他进行中的事务：
  - 不可见
- 如果 `xmin` 属于 aborted 事务：
  - 不可见
- 如果 `xmin` 属于已提交事务：
  - 再根据 snapshot 判断是否可见

然后再判断 `xmax`：

- `xmax == INVALID_TXN_ID`
  - 说明还没有被删掉或更新掉，对当前事务可见
- `xmax` 是当前事务
  - 说明自己已经把这个旧版本删掉了，对自己不可见
- `xmax` 是其他进行中的事务
  - 旧版本暂时仍然可见
- `xmax` 属于 aborted 事务
  - 旧版本仍然可见
- `xmax` 属于已提交事务，且对当前 snapshot 可见
  - 旧版本不可见

### 8.4 同事务自可见语义

第一版至少要保证以下行为：

- 一个事务能看到自己插入的数据
- 一个事务看不到自己已经删除的数据
- update 之后，这个事务能看到新版本，看不到旧版本

这也是为什么 `cmin/cmax` 一开始就应该预留好。

## 9. DML 语义

### 9.1 INSERT

执行流程：

1. 将 `Row` 序列化为 payload
2. 组装 `HeapTuple`
3. 填写 header：
   - `xmin = current_xid`
   - `xmax = INVALID_TXN_ID`
   - `cmin = 当前命令号`
   - `cmax = 0`
   - `t_ctid = self`，在 rid 分配完成后补齐
4. 将 tuple 插入 `TablePage`
5. 在事务的 `write_set_` 中追加一条 `INSERT`

### 9.2 DELETE

执行流程：

1. 定位当前可见版本
2. 检查并发写冲突
3. 更新旧版本 header：
   - `xmax = current_xid`
   - `cmax = 当前命令号`
   - 设置 delete 相关 infomask
4. 在 `write_set_` 中记录一条 `DELETE`

这里不做物理删除。

### 9.3 UPDATE

update 必须彻底禁止原地覆盖。

执行流程：

1. 定位当前可见版本 `old_rid`
2. 构造 `new_tuple`
3. 插入 `new_tuple`，其 header 为：
   - `xmin = current_xid`
   - `xmax = INVALID_TXN_ID`
   - `cmin = 当前命令号`
   - `t_ctid = new_rid`
4. 回写旧版本 header：
   - `xmax = current_xid`
   - `cmax = 当前命令号`
   - `t_ctid = new_rid`
   - 设置 `HEAP_UPDATED`
5. 写集里分别记录 `UPDATE_OLD` 和 `UPDATE_NEW`

这个过程就是 PostgreSQL 风格的版本链。

### 9.4 写写冲突

在 `UPDATE` 或 `DELETE` 之前，如果目标 tuple 满足以下情况，就要认为发生并发冲突：

- `xmax` 属于另一个进行中的事务
- `xmax` 属于一个对当前 snapshot 已可见的已提交事务

第一版建议不要做等待锁，直接失败返回冲突错误即可。

建议行为：

- 检测到并发 update/delete，直接返回 conflict
- 暂时不做锁等待

## 10. Abort 与回滚

PostgreSQL 的本质是“tuple header + transaction status”共同决定可见性。你这里第一版也可以走这个思路。

不过为了保证回滚行为更清晰，建议仍然保留写集来回放恢复。

### 10.1 回滚 INSERT

insert 产生的新 tuple 不需要立刻物理删掉：

- 事务状态置为 `ABORTED`
- 这个 tuple 因为 `xmin` 属于 aborted 事务，所以以后永远不可见
- 后面由 vacuum 再回收

### 10.2 回滚 DELETE

如果当前事务给一个旧版本写了 `xmax`，随后 abort：

- 把旧 tuple 的 `xmax` 清回 `INVALID_TXN_ID`
- 清掉 delete 相关的 infomask

### 10.3 回滚 UPDATE

需要做两件事：

- 把旧版本的 `xmax` 清掉，并将 `t_ctid` 改回指向自己
- 新版本物理上可以留着，但因为其 `xmin` 属于 aborted 事务，所以不可见

这些回滚动作可以通过 `write_set_` 驱动完成。

## 11. Vacuum 与 Prune

### 11.1 为什么必须有 vacuum

MVCC 会留下很多历史版本。如果没有 vacuum：

- page 空间会被不断吃掉
- 扫描成本会持续升高
- aborted insert 产生的垃圾永远存在

### 11.2 最小回收规则

一个 tuple 版本可以被回收，当满足以下条件之一：

- 插入它的事务已经 abort
- 删除或替换它的事务已经提交，并且没有活跃 snapshot 还可能看到这个旧版本

这里需要 transaction manager 提供：

```cpp
oldest_xmin = txn_mgr.OldestActiveXmin();
```

如果某个旧版本的删除事务已经提交，而且该事务 xid 小于 `oldest_xmin`，就说明所有活跃事务都不再需要它了，可以进入可回收状态。

### 11.3 第一版的回收策略

建议先做 page-local prune：

- 扫描一页上的所有 slot
- 如果 tuple 可回收，则把 slot 标记为 `DEAD`
- 暂时不急着做页面 compaction

也就是说，第一版只要能把 dead tuple 标出来就够了，后面再考虑真正整理 page。

## 12. 模块与文件规划

### 12.1 新增文件

建议新增：

- `src/include/transaction/transaction.h`
- `src/include/transaction/transaction_manager.h`
- `src/include/transaction/snapshot.h`
- `src/include/transaction/visibility.h`
- `src/transaction/transaction_manager.cpp`
- `src/transaction/visibility.cpp`
- `src/include/storage/page/tuple_header.h`
- `src/include/storage/page/heap_tuple.h`
- `src/storage/page/heap_tuple.cpp`
- `src/transaction/CMakeLists.txt`

### 12.2 需要修改的文件

- `src/CMakeLists.txt`
  - 添加 `add_subdirectory(transaction)`
- `src/include/executor/executor_context.h`
  - 接入 transaction manager 和当前事务
- `src/include/client/client_context.h`
  - 增加当前事务或 autocommit 状态
- `src/include/client/client_session.h`
  - 如果要做 session 级事务，需要接入事务拥有权
- `src/include/storage/page/table_page.h`
  - 改 slot 定义，新增 tuple header 读写接口
- `src/storage/page/table_page.cpp`
  - 改成读写 heap tuple
- `src/include/storage/page/tuple.h`
  - 降级为 executor 输出结构，或逐步从存储语义中剥离
- `src/storage/page/row.cpp`
  - 保留 payload 序列化职责，不再承担 MVCC 结构
- `src/buffer/table_iterator.cpp`
  - 扫描时加入可见性过滤
- `src/include/buffer/table_iterator.h`
  - 向 iterator 传入事务上下文
- `src/executor/executor_seq_scan.cpp`
  - 调用 visibility 层进行过滤
- `src/executor/executor_insert.cpp`
  - 构造带 header 的 heap tuple

### 12.3 后续如果加 update/delete executor

后面还会新增：

- `src/include/executor/executor_update.h`
- `src/executor/executor_update.cpp`
- `src/include/executor/executor_delete.h`
- `src/executor/executor_delete.cpp`

## 13. 接口草案

### 13.1 ExecutorContext

建议改成：

```cpp
class ExecutorContext {
public:
    std::shared_ptr<Catalog> catalog_;
    std::shared_ptr<BufferManager> buffer_manager_;
    std::shared_ptr<MetaPage> meta_page_;
    std::shared_ptr<TransactionManager> txn_mgr_;
    std::shared_ptr<Transaction> txn_;
};
```

### 13.2 TablePage

建议接口逐步演进成：

```cpp
class TablePage {
public:
    auto InsertHeapTuple(const HeapTuple &tuple, RID *rid) -> bool;
    auto FetchHeapTuple(const RID &rid, HeapTuple *tuple, Slot *slot) -> bool;
    auto UpdateTupleHeader(const RID &rid, const TupleHeader &header) -> bool;
    auto MarkDelete(const RID &rid, txn_id_t xid, command_id_t cid) -> bool;
    auto UpdateVersion(const RID &old_rid,
                       const HeapTuple &new_tuple,
                       RID *new_rid,
                       txn_id_t xid,
                       command_id_t cid) -> bool;
    auto Prune(const TransactionManager &txn_mgr) -> void;
};
```

### 13.3 Visibility

建议统一通过这个接口判定：

```cpp
auto HeapTupleVisible(const HeapTuple &tuple,
                      const Transaction &txn,
                      const TransactionManager &txn_mgr) -> bool;
```

## 14. 分阶段实施计划

### 阶段 1：事务基础设施

交付内容：

- xid 分配
- 事务状态表
- snapshot 生成
- `ExecutorContext` 接线

验收标准：

- 一条语句能够运行在一个事务对象里
- `SeqScan` 可以拿到 snapshot

### 阶段 2：物理 tuple 重构

交付内容：

- `TupleHeader`
- `HeapTuple`
- 新 slot 结构
- `TablePage` 支持 heap tuple 读写

验收标准：

- insert/fetch 可以处理带 header 的 tuple
- 原有 `Row` 序列化仍可复用

### 阶段 3：MVCC 读路径

交付内容：

- visibility 规则
- MVCC 感知的扫描器

验收标准：

- tuple 是否返回完全取决于 snapshot 和事务状态

### 阶段 4：MVCC 写路径

交付内容：

- 带 xid 的 insert
- 通过 `xmax` 实现 delete
- 基于 `t_ctid` 的 update 版本链

验收标准：

- heap table 写路径中不再出现原地 update

### 阶段 5：回滚

交付内容：

- 写集记录
- insert/delete/update 的 abort 逻辑

验收标准：

- abort 后对应写入不可见

### 阶段 6：vacuum / prune

交付内容：

- 基于 `OldestActiveXmin` 的回收判定
- dead slot 标记

验收标准：

- 活跃读事务结束后，旧版本能进入可回收状态

## 15. 测试计划

### 15.1 事务管理器单测

- xid 分配单调递增
- snapshot 能正确捕获活跃事务
- commit 状态迁移正确
- abort 状态迁移正确

### 15.2 可见性单测

- 已提交 insert 对后续事务可见
- aborted insert 不可见
- 其他进行中事务的 insert 不可见
- 已提交 delete 对后续 snapshot 生效
- aborted delete 不影响旧版本可见性
- 自己插入的数据自己可见
- 自己删除的数据自己不可见
- update 后自己看到新版本，看不到旧版本

### 15.3 存储层测试

- tuple header 在 page roundtrip 后不丢
- update 会产生第二个物理 tuple
- 旧 tuple 的 `t_ctid` 正确指向新 tuple
- prune 只会在安全条件下把 slot 标成 dead

### 15.4 执行器测试

- `SeqScan` 只返回当前事务可见版本
- autocommit insert 后 select 能看到结果
- update 后 select 返回新值
- rollback update 后旧值重新可见

## 16. 已知风险

### 16.1 当前扫描器本身有边界问题

现有 iterator 逻辑在翻页和最后一行处理上需要重新检查。否则后面排错时很难分辨到底是扫描 bug，还是 MVCC 可见性 bug。

### 16.2 现有 `Tuple` 的内存所有权比较脆弱

当前大量使用裸 `char *`。如果在重构成 `HeapTuple` 的过程中继续叠加手动内存管理，后续很容易出现重复释放或者浅拷贝问题。

### 16.3 现有 meta row count 语义会失真

目前 `meta_page_->UpdateTableRows()` 的含义接近“当前表实际行数”。但引入 MVCC 后：

- insert 会立刻新增一个物理版本
- delete 不会立刻删物理版本
- update 会新增一个物理版本，同时让旧版本失活

所以元数据计数必须重新定义，否则统计值会越来越不可信。至少要区分：

- 物理 tuple 数
- 逻辑可见行数估计
- vacuum 后 live row 数

## 17. 第一阶段推荐目标

第一批值得合并的最小里程碑建议是：

- 支持 autocommit 事务
- 每条语句生成 snapshot
- 引入 `xmin/xmax/cmin/cmax/t_ctid`
- `SeqScan` 支持 MVCC 可见性判断
- insert 走 MVCC
- delete 走 `xmax`
- update 改成版本链，而不是原地改
- abort 通过事务状态和写集实现

做到这里，已经可以说你的存储层和读写路径具备 PostgreSQL 风格 MVCC 的核心语义。

## 18. 实施过程中的硬约束

下面这些约束建议视为强制规则：

1. 不要再把 `deleted` 当作行可见性的事实来源。
2. 不允许 `UPDATE` 做原地覆盖。
3. 不允许跳过 snapshot 直接读物理 tuple。
4. 不允许把 MVCC 元数据只塞在 slot 里。
5. 不允许在活跃 snapshot 仍可能访问旧版本时提前 vacuum。

## 19. 推荐开发顺序

建议严格按下面顺序推进：

1. 新增 transaction 模块，并接入 `ExecutorContext`
2. 引入 `TupleHeader` 和 `HeapTuple`
3. 重构 `TablePage` 的存储格式和 slot 结构
4. 实现 visibility 逻辑
5. 改 `SeqScan`，让扫描走 MVCC 过滤
6. 改 insert 路径
7. 实现 delete
8. 把 update 改成版本链
9. 加 abort
10. 加 prune / vacuum
11. 每个阶段配套补测试

不要一上来先改 update，否则代码会非常乱。

## 20. 后续演进方向

如果这个项目继续往真正数据库内核发展，后续可以逐步补齐：

- 落盘事务状态存储
- WAL
- 崩溃恢复
- 索引与 MVCC 的协同
- vacuum 调度
- HOT update

但这些都不应该阻塞本文档描述的第一版 PostgreSQL 风格 MVCC 落地。
