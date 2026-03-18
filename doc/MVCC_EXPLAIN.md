# MVCC 原理说明与示例

## 1. 这篇文档是干什么的

这篇文档不是实现清单，也不是模块拆分说明，而是专门用来回答下面这些问题：

- MVCC 到底在解决什么问题
- PostgreSQL 风格的 MVCC 和“给行加个删除标记”有什么本质区别
- 为什么 `UPDATE` 不能原地修改
- 为什么读取一条记录时，不能只看“它现在长什么样”，而必须结合事务和 snapshot
- `xmin/xmax/t_ctid` 这些字段到底在表达什么
- 一个版本为什么有时候可见，有时候又不可见

如果你后面在实现时对某个行为拿不准，比如“这个旧版本现在到底该不该返回给扫描器”，优先回来看这篇文档。

## 2. 什么是 MVCC

MVCC 的全称是 Multi-Version Concurrency Control，多版本并发控制。

它的核心思想非常简单：

> 不要让一个事务直接覆盖另一个事务可能还需要看到的数据版本。

换句话说，数据库中的一行数据，在不同时间点、不同事务看来，可以对应不同的“版本”。

举个最简单的例子。

表里原来有一行：

```text
id = 1, name = 'Alice', balance = 100
```

事务 T1 正在读这行，事务 T2 想把 `balance` 改成 `200`。

如果不用 MVCC，而是直接原地改，那么就会有两个问题：

1. T1 可能读到一半时数据被改掉，出现非一致结果。
2. 为了避免这个问题，只能强依赖锁，把读写强行串起来，并发性能会很差。

MVCC 的思路是：

- T2 不去覆盖旧版本
- T2 生成一个新版本，值是 `balance = 200`
- 旧版本 `balance = 100` 先保留
- T1 按照自己的 snapshot 继续看到旧版本
- 后来的事务根据 snapshot 看到新版本

所以，MVCC 的本质不是“并发时能看到多个值”这么简单，而是：

> 通过保留历史版本，让读操作尽量不阻塞写，写操作也尽量不阻塞读。

## 3. MVCC 想解决哪些问题

### 3.1 读写冲突

最典型的问题是：

- 一个事务正在读
- 另一个事务正在写

如果所有读都要等写结束，或者所有写都要等读结束，系统吞吐会非常差。

MVCC 的目标之一，就是让大部分普通 `SELECT` 不必去等待写事务。

### 3.2 一致性读

事务执行期间，用户通常希望看到的是“某个时刻的一致数据视图”，而不是中间被别人改来改去的数据。

例如：

```sql
BEGIN;
SELECT balance FROM account WHERE id = 1;
-- 此时另一个事务更新了 balance
SELECT balance FROM account WHERE id = 1;
COMMIT;
```

在某些隔离级别下，用户希望两次 `SELECT` 看到同一个版本，这就需要 snapshot。

### 3.3 避免脏读

如果事务 T2 还没提交就把数据改了，T1 绝不能把这个未提交的数据当成真实结果读出来。

MVCC 会通过事务状态和版本元数据控制：

- 别人的未提交版本不可见
- 已回滚事务产生的版本不可见

## 4. PostgreSQL 风格 MVCC 的核心思路

PostgreSQL 风格 MVCC 的重点不是“维护一个版本列表”这么宽泛，而是以下三个非常具体的原则：

1. 每个物理 tuple 自带版本元数据
2. `UPDATE` 不原地修改，而是产生一个新 tuple
3. 读路径通过 snapshot 判断某个 tuple 是否可见

### 4.1 一个物理 tuple 代表一个版本

在 PostgreSQL 风格设计里，一条物理 tuple 不是“某行的唯一存储”，而是“某行在某一时刻的一个版本”。

也就是说，你逻辑上看到的一行：

```text
id = 1, balance = 100
```

可能在存储层其实长这样：

```text
版本 V1: id = 1, balance = 80
版本 V2: id = 1, balance = 100
版本 V3: id = 1, balance = 200
```

其中：

- 某些事务只能看到 V1
- 某些事务能看到 V2
- 更晚的事务才看到 V3

### 4.2 `UPDATE` = 删除旧版本 + 插入新版本

这点是理解 PostgreSQL MVCC 的关键。

很多人一开始会自然地想：

> update 就是把原来的 tuple 内容改掉。

这在 MVCC 里不对。

正确做法是：

- 旧版本保留
- 在旧版本上记录“我已经被哪个事务替换掉了”
- 再插入一个新版本

所以：

```sql
UPDATE account SET balance = 200 WHERE id = 1;
```

存储层真正发生的事情更接近：

```text
旧版本: balance = 100, 被事务 T2 标记为过期
新版本: balance = 200, 由事务 T2 创建
```

### 4.3 `DELETE` 不是物理删除

同理，`DELETE` 也不是马上把 tuple 从页面里抹掉。

`DELETE` 的本质是：

- 给旧版本写上 `xmax`
- 表示“这个版本从某个事务开始不再有效”

只有等到确认没有任何活跃事务还可能看到这个版本时，vacuum 才可以真正把它回收。

## 5. 为什么 PostgreSQL 不把版本信息只放在 slot 里

你现在这个项目里，slot 类似：

```cpp
struct Slot {
    uint16_t offset;
    uint16_t size;
    bool deleted;
};
```

这种结构只适合非常简单的存储模型，不适合 PostgreSQL 风格 MVCC。

原因有三个。

### 5.1 可见性属于 tuple 版本，不属于 page 管理信息

slot 的职责应该是：

- 指向 tuple 在页内的偏移
- 管理 slot 的生死状态

而不是承载：

- 谁创建了这个版本
- 谁删除了这个版本
- 这个版本是不是对当前事务可见

这些信息本质上是 tuple 版本级别的语义，不是 slot 级别的语义。

### 5.2 一个逻辑行会有多个物理版本

如果把版本信息塞进 slot，就会越来越别扭：

- 一个逻辑行可能会对应多个物理 tuple
- 每个 tuple 都需要自己的 `xmin/xmax`
- update 后旧版本和新版本都要有完整事务信息

这时候版本元数据就必须跟着 tuple 本身走。

### 5.3 vacuum 和版本链都会要求 tuple 自带元数据

后面你做：

- update 版本链
- prune
- vacuum
- hint bit

都会直接依赖 tuple header 里的字段。

所以 PostgreSQL 的路线是：

> slot 管页面定位，tuple header 管版本语义。

## 6. Tuple Header 中每个字段是什么意思

如果你要按 PostgreSQL 风格做，最核心的 header 至少包含这些字段：

```cpp
struct TupleHeader {
    txn_id_t xmin;
    txn_id_t xmax;
    command_id_t cmin;
    command_id_t cmax;
    uint16_t infomask;
    RID t_ctid;
};
```

下面逐个解释。

### 6.1 `xmin`

表示“创建这个版本的事务”。

例如：

- T10 插入了一条 tuple
- 那这个 tuple 的 `xmin = 10`

如果这个事务后来 abort 了，那么这个版本永远不可见。

### 6.2 `xmax`

表示“让这个版本失效的事务”。

注意，这个“失效”可能来自两种操作：

- `DELETE`
- `UPDATE`

为什么 `UPDATE` 也会写 `xmax`？

因为 update 后旧版本已经不再是当前版本了，它被新版本替代了。对旧版本来说，本质上就是“我被结束了”。

### 6.3 `cmin`

表示当前事务内部，是第几条命令创建了这个版本。

这个字段的价值在于：

- 同一个事务内可能有多条语句
- 需要精确处理“自己是否能看到自己刚刚写入的数据”

第一版即使暂时没完全用到，也应该先预留。

### 6.4 `cmax`

表示当前事务内部，是第几条命令让这个版本失效。

这个字段主要用于：

- 自事务 delete/update 可见性
- 更细粒度的命令级判断

### 6.5 `infomask`

这是状态位集合，用来缓存一些判断结果，典型用途包括：

- `xmin` 已提交
- `xmin` 无效
- `xmax` 已提交
- `xmax` 无效
- 这个 tuple 是 update 产生的旧版本
- 这个 tuple 是 delete 标记的

它的作用是减少反复查事务状态表的成本。

第一版可以先只保留字段，不必一开始把 hint bit 优化做满。

### 6.6 `t_ctid`

这是 PostgreSQL 风格中很关键的一个字段。

它表示“这个版本对应的当前 tuple 标识”。

简化理解：

- 如果当前 tuple 是最新版本，`t_ctid` 指向自己
- 如果当前 tuple 被 update 了，`t_ctid` 指向新版本

它的作用之一就是把旧版本和新版本串起来。

## 7. Snapshot 到底是什么

很多人第一次接触 MVCC 时，会把 snapshot 理解成“复制一份表数据”。这不对。

snapshot 不是数据副本，而是一组规则，用来回答：

> 当前事务在判断可见性时，哪些事务算已经结束，哪些事务算还没结束？

建议的结构：

```cpp
struct Snapshot {
    txn_id_t xmin;
    txn_id_t xmax;
    std::vector<txn_id_t> xip;
};
```

### 7.1 `xmin`

当前系统中，拍 snapshot 时仍在运行的最小事务 id。

如果一个事务 id 比它还小，并且已经提交，一般可以认为对当前 snapshot 可见。

### 7.2 `xmax`

拍 snapshot 时系统“下一个准备分配的 xid”。

凡是 `xid >= xmax` 的事务，在这个 snapshot 看来都属于“未来事务”，所以不可见。

### 7.3 `xip`

记录拍 snapshot 时仍然活跃的事务列表。

如果一个 tuple 的 `xmin` 落在这个列表里，就说明这个版本在拍快照时还没有完成提交，因此不可见。

## 8. 如何判断一个 tuple 是否可见

MVCC 读路径的本质，就是在扫描每个物理 tuple 时，做一次“可见性判定”。

直觉上可以分两步：

1. 看创建事务 `xmin` 是否对当前 snapshot 可见
2. 看删除事务 `xmax` 是否对当前 snapshot 可见

只有“创建可见，删除不可见”，这个 tuple 才能返回。

## 9. 一个完整的可见性例子

我们用一个时间线来说明。

初始时表里有一条记录：

```text
R1: id = 1, balance = 100
```

假设它的 tuple header 是：

```text
xmin = 5
xmax = 0
t_ctid = self
```

说明：

- 它是事务 T5 插入的
- 它还没有被删除或更新

现在发生如下事务序列。

### 场景一：已提交插入对后续事务可见

```text
T10: INSERT INTO account VALUES (2, 300);
T10: COMMIT;

T11: SELECT * FROM account WHERE id = 2;
```

对 `id = 2` 那条 tuple 来说：

```text
xmin = 10
xmax = 0
```

T11 拿到 snapshot 后，如果发现：

- T10 已提交
- T10 不在 snapshot 的活跃事务集合里

那么这个版本可见。

### 场景二：未提交插入对其他事务不可见

```text
T20: INSERT INTO account VALUES (3, 500);
-- 尚未提交

T21: SELECT * FROM account WHERE id = 3;
```

这时 `id = 3` 的 tuple：

```text
xmin = 20
xmax = 0
```

但 T21 拍 snapshot 时，T20 仍在运行，因此 T20 会出现在 `xip` 中。

于是这个版本不可见。

结论：

- 自己插入的数据自己可以看到
- 别人未提交插入的数据不能看到

### 场景三：DELETE 之后旧版本何时不可见

假设现在有旧版本：

```text
R1: id = 1, balance = 100
xmin = 5
xmax = 0
```

事务 T30 执行：

```sql
DELETE FROM account WHERE id = 1;
```

此时不是物理删，而是变成：

```text
R1:
xmin = 5
xmax = 30
```

这代表：

- 这个版本是 T5 创建的
- 它在 T30 看来已经被删除了

如果此时 T30 还没提交：

- T30 自己再查 `id = 1`，看不到这条记录
- 别的事务如果 snapshot 看不到 T30 提交，则仍然能看到旧版本

等 T30 提交后：

- 更晚开启的事务就看不到这个版本了

### 场景四：UPDATE 产生版本链

这是最关键的例子。

初始：

```text
R1: id = 1, balance = 100
xmin = 5
xmax = 0
t_ctid = R1
```

事务 T40 执行：

```sql
UPDATE account SET balance = 200 WHERE id = 1;
```

正确的 MVCC 行为不是原地改，而是：

1. 插入一个新版本 `R2`
2. 回写旧版本 `R1`

更新后：

```text
R1: id = 1, balance = 100
xmin = 5
xmax = 40
t_ctid = R2

R2: id = 1, balance = 200
xmin = 40
xmax = 0
t_ctid = R2
```

现在分别看不同事务看到什么。

#### T40 自己看

T40 是执行 update 的事务。

它应该看到：

```text
id = 1, balance = 200
```

因为：

- 旧版本 `R1` 的 `xmax = 40`，表示自己已经把它结束掉了
- 新版本 `R2` 的 `xmin = 40`，是自己刚刚创建的

#### 一个更早开始的旧事务看

假设事务 T39 在 T40 update 之前就已经拿到了 snapshot。

那么对 T39 来说：

- R2 的 `xmin = 40` 属于“未来事务”或“当时未提交事务”，不可见
- R1 的 `xmax = 40` 对 T39 的 snapshot 不可见，因此旧版本仍然可见

所以 T39 继续看到：

```text
id = 1, balance = 100
```

#### 一个更晚开始的新事务看

如果 T40 提交后，事务 T41 再开始：

- R2 的 `xmin = 40` 已提交，对 T41 可见
- R1 的 `xmax = 40` 已提交，对 T41 也可见，因此旧版本不可见

所以 T41 看到：

```text
id = 1, balance = 200
```

这个例子就是 MVCC 的精髓：

> 同一时刻页面里同时存在两个版本，不同事务依据自己的 snapshot 看到不同结果。

## 10. 为什么 `UPDATE` 一定不能原地修改

这个点非常重要，单独强调一下。

如果你把：

```text
balance = 100
```

直接改成：

```text
balance = 200
```

那之前所有还应该看到旧版本的事务都会出错。

例如：

```text
T1 先开始，拿到 snapshot
T2 后开始，执行 UPDATE 并提交
T1 再读一次
```

如果 T2 是原地更新，那么 T1 第二次读取时只能看到 `200`，这就破坏了一致性读。

所以只要你要的是 PostgreSQL 风格 MVCC，下面这条就是硬规则：

> update 不允许覆盖旧版本，只能追加新版本。

## 11. 为什么 `DELETE` 不能立刻物理删除

原因和 update 一样。

假设：

```text
T1 已经开始，snapshot 中还能看到某条旧记录
T2 执行 DELETE 并提交
```

如果 T2 立刻把数据从页里抹掉，那么 T1 后续就再也无法读到本该属于它 snapshot 的旧版本了。

这会直接破坏 MVCC 的可见性语义。

所以 delete 只能先做：

- 标记版本被某事务删除

而不能立刻做：

- 物理删除 tuple 数据

## 12. 为什么需要事务状态表

光有 `xmin/xmax` 还不够。

例如你看到一个 tuple：

```text
xmin = 40
```

那你还得知道：

- 事务 40 现在是 `IN_PROGRESS`、`COMMITTED` 还是 `ABORTED`

否则根本没法判断这个版本可不可见。

所以系统必须维护一张事务状态表。

第一版可以是内存结构：

```cpp
std::unordered_map<txn_id_t, TxnState> txn_state_;
```

作用是：

- 判断某个 `xmin/xmax` 对应的事务状态
- 给 snapshot 可见性判断提供依据
- 给 abort / vacuum 提供信息

## 13. 为什么需要 `t_ctid`

很多人一开始会觉得 `xmin/xmax` 已经够了，`t_ctid` 好像多余。

实际上它很有价值。

### 13.1 它能把旧版本指向新版本

发生 update 后：

```text
R1.t_ctid = R2
```

这表示：

- R1 已经不是最新版本
- R2 才是它的后继版本

### 13.2 有利于后续做链式处理

有了 `t_ctid`，后续你做：

- 追踪版本链
- prune
- 某些 update 优化

都会更自然。

### 13.3 它让“这个版本现在对应谁”变得明确

即使你不在第一版就大量用它，也应该从一开始把这个字段建好。

## 14. Snapshot 不是“谁最新就看谁”

这是初学 MVCC 时最容易犯的一个理解错误。

很多人会自然写出类似逻辑：

> 如果一个逻辑行有多个版本，就返回最新那个。

这不对。

正确规则不是“最新”，而是“对当前 snapshot 可见”。

例如：

```text
旧版本 R1: xmin = 5, xmax = 40
新版本 R2: xmin = 40, xmax = 0
```

如果当前事务的 snapshot 看不到 T40 提交，那么：

- R2 不可见
- R1 仍然可见

也就是说，虽然物理上 R2 更新，但逻辑上当前事务仍然要返回 R1。

所以读路径一定要牢记：

> 不是选最新版本，而是选当前 snapshot 下的可见版本。

## 15. Read Committed 和 Repeatable Read 的差别

你后面实现时即使第一版先只做一种，也最好知道差异。

### 15.1 Read Committed

每条语句开始时拿一次新的 snapshot。

这意味着：

- 同一个事务里的两条 `SELECT`
- 如果中间别的事务提交了更新
- 第二条 `SELECT` 可能看到新版本

例子：

```text
T1: BEGIN
T1: SELECT balance -> 100

T2: UPDATE balance = 200; COMMIT

T1: SELECT balance -> 200
T1: COMMIT
```

这是 Read Committed 下可以接受的。

### 15.2 Repeatable Read

整个事务共用同一个 snapshot。

这意味着：

- 事务第一次拍到什么视图
- 后面整个事务期间都按这个视图读

例子：

```text
T1: BEGIN
T1: SELECT balance -> 100

T2: UPDATE balance = 200; COMMIT

T1: SELECT balance -> 100
T1: COMMIT
```

这时候 T1 会一直看到旧版本。

对于你当前这个项目来说，第一版如果先做：

- autocommit
- 每条语句一个事务

那行为会更接近简化版的 Read Committed。

## 16. 一个更完整的时间线例子

下面给一个综合例子，把 insert / update / delete / snapshot 串起来。

### 初始状态

表中只有一条记录：

```text
R1: id = 1, name = 'Alice', balance = 100
xmin = 1
xmax = 0
t_ctid = R1
```

### 时间线

#### 时刻 A：T10 开始

T10 拍 snapshot：

```text
xmin = 10
xmax = 11
xip = [10]
```

它执行：

```sql
SELECT balance FROM account WHERE id = 1;
```

看到的是：

```text
100
```

#### 时刻 B：T11 开始并更新

T11 执行：

```sql
UPDATE account SET balance = 200 WHERE id = 1;
```

页面中变成：

```text
R1: balance = 100, xmin = 1, xmax = 11, t_ctid = R2
R2: balance = 200, xmin = 11, xmax = 0, t_ctid = R2
```

但这时 T11 尚未提交。

#### 时刻 C：T10 再次读取

T10 如果使用之前的 snapshot 来判断：

- R2 的 `xmin = 11`，对 T10 不可见
- R1 的 `xmax = 11`，对 T10 也不可见

所以 T10 仍然看到：

```text
100
```

#### 时刻 D：T11 提交

事务状态表中：

```text
T11 -> COMMITTED
```

#### 时刻 E：T12 开始

T12 拿到新 snapshot 后再查：

- R2 的 `xmin = 11` 已提交，可见
- R1 的 `xmax = 11` 已提交，旧版本不可见

所以 T12 看到：

```text
200
```

#### 时刻 F：T13 删除

T13 执行：

```sql
DELETE FROM account WHERE id = 1;
```

这时 R2 被标记：

```text
R2: balance = 200, xmin = 11, xmax = 13, t_ctid = R2
```

如果 T13 尚未提交：

- T13 自己看不到这行
- 其他事务若 snapshot 看不到 T13 提交，仍然可能看到 `200`

T13 提交后：

- 更晚事务就看不到任何可见版本了

这就是整套 MVCC 行为。

## 17. Abort 为什么不会破坏 MVCC

假设事务 T20 插入了一个新版本，但最后 abort 了。

例如：

```text
R3: balance = 300
xmin = 20
xmax = 0
```

只要事务状态表中：

```text
T20 -> ABORTED
```

那么这个版本在可见性判断时就会失败：

- 因为创建事务未提交，且已 abort
- 所以这个版本不可见

这就是为什么即使 abort 后暂时不立即物理删掉 tuple，也不会影响正确性。

对于 update 和 delete 的 abort 也是一样：

- 要么通过写集把 `xmax` 清回去
- 要么依赖事务状态把对应操作判为无效

## 18. Vacuum 在 MVCC 中扮演什么角色

MVCC 会留下历史版本，这是它换来高并发读写的代价。

因此系统还需要一个清理阶段。

vacuum 的职责不是“让数据变可见”。

可见性在事务提交那一刻就决定了。

vacuum 的职责是：

- 找出已经不可能再被任何活跃事务看到的旧版本
- 回收它们占用的物理空间

例如：

```text
R1: xmin = 5, xmax = 40
```

如果：

- T40 已提交
- 当前系统中所有活跃事务的 `xmin` 都大于 40

那就说明没有任何活跃事务还会看到这个旧版本了，它就可以被标记为 dead，等待回收。

## 19. 对你当前项目最重要的几个认知

如果结合 `YourSQL` 当前代码结构，最关键的认知有这几个：

### 19.1 `deleted` 这个字段不能继续当真相来源

因为在 MVCC 里，“这条记录是否应该返回”不是一个布尔值问题，而是：

- 对哪个事务？
- 在哪个 snapshot 下？

所以它不能只靠 `deleted = true/false` 决定。

### 19.2 `updateTuple()` 这种原地改法必须废掉

只要目标是 PostgreSQL 风格 MVCC，原地 update 就是错误实现。

### 19.3 扫描器不能只按物理顺序吐 tuple

扫描器必须在返回 tuple 之前做可见性判断。

也就是说，扫描器以后应该做的是：

1. 读物理 tuple
2. 取 header
3. 调 visibility 逻辑
4. 只有可见版本才交给 executor

### 19.4 版本元数据应该跟 tuple 走

所以你后面一定会自然引出：

- `TupleHeader`
- `HeapTuple`
- `Snapshot`
- `TransactionManager`

这是正常演进，不是多此一举。

## 20. 一句话总结

如果要用一句话概括 PostgreSQL 风格 MVCC，可以记成：

> 每次写入都不是“修改当前值”，而是在存储层制造一个新的版本；每次读取都不是“拿最新值”，而是根据当前事务的 snapshot 从多个版本中挑出可见的那个版本。

你后面只要围绕这句话去审视设计，很多实现细节就不会跑偏。

## 21. 建议配合阅读顺序

建议这样看：

1. 先看本文件，建立对 MVCC 行为的直觉
2. 再看 [README.md](/Users/yanghuan/core/project/cpp-proj/YourSQL/src/transaction/README.md)，对照具体开发方案
3. 真正实现时，先落 transaction manager 和 snapshot，再做 tuple header 和 scan visibility

如果后面你需要，我还可以继续补两类文档：

- 一篇“可见性判定流程图版”，专门把 `xmin/xmax` 判定画成决策树
- 一篇“PostgreSQL MVCC 到你当前代码结构的映射文档”，逐文件说明该怎么改
