# PostgreSQL 中的 MVCC 与隔离级别原理说明

## 1. 为什么要写这篇文档

很多人第一次接触 PostgreSQL 的 MVCC 时，都会立刻想到一个问题：

> 既然一个数据会保留多个版本，那数据库的隔离级别到底是怎么实现的？

进一步还会有两个常见疑问：

1. PostgreSQL 的隔离级别，和教科书里讲的隔离级别是不是一回事？
2. PostgreSQL 既然主要靠 MVCC 做并发控制，那是不是就不需要锁了？

答案都不是一句话能说清的。

这篇文档的目标就是把下面几件事讲明白：

- PostgreSQL 的 MVCC 到底是怎么工作的
- tuple header 里的 `xmin/xmax`、事务状态表、snapshot 各自负责什么
- PostgreSQL 的隔离级别是如何建立在 MVCC 之上的
- PostgreSQL 的隔离级别和很多人“教材式理解”的差异在哪里
- 为什么 PostgreSQL 既有 MVCC，又仍然需要锁与 Serializable 机制

如果你接下来准备在 `YourSQL` 里实现 PostgreSQL 风格 MVCC，那么这篇文档的定位就是“原理地图”。

## 2. 先给一个总览

可以先记住下面这句话：

> PostgreSQL 的 MVCC 负责解决“读到哪个版本”的问题，而隔离级别决定“一个事务在整个执行过程中采用什么样的 snapshot 规则，以及遇到读写冲突时该不该失败”。

这句话可以拆成两层：

### 2.1 MVCC 解决的是版本可见性

同一条逻辑记录，在页面里可能同时有多个物理版本。

读取时，PostgreSQL 不是简单返回“最新版本”，而是：

- 看当前事务的 snapshot
- 看 tuple 的 `xmin/xmax`
- 看这些事务现在是已提交、未提交还是已回滚

然后判断某个版本对当前事务是否可见。

### 2.2 隔离级别决定的是 snapshot 的使用方式

不同隔离级别最大的差异，不是“锁强不强”，而是：

- snapshot 是每条语句都重新拿一次
- 还是整个事务从头到尾共用一个
- 还是在快照读之外，还要检测更高级别的冲突

所以 PostgreSQL 的隔离级别，确实和很多人一开始理解的不完全一样。

## 3. PostgreSQL MVCC 的核心思想

PostgreSQL 的基本思路是：

- 每个 tuple 都有自己的事务可见性元数据
- `UPDATE` 不原地修改旧 tuple
- `DELETE` 不立刻物理删除 tuple
- 事务读取时通过 snapshot 判断当前版本是否可见
- 垃圾版本之后再由 vacuum 回收

换句话说：

> PostgreSQL 不是用“让所有人围着同一份最新数据加锁”来实现并发，而是通过“保留旧版本 + 快照读”来尽量减少读写互相等待。

## 4. PostgreSQL 中的一条记录实际上是什么

在 PostgreSQL 里，一条“行”不是一个抽象概念，而是一个 heap tuple。

一个 heap tuple 通常包含两部分：

1. tuple header
2. 用户数据

用户看到的是：

```text
id = 1, balance = 100
```

但 PostgreSQL 存储层看到的是更接近这样的结构：

```text
TupleHeader {
    xmin
    xmax
    ctid
    infomask
    ...
}
payload {
    id = 1
    balance = 100
}
```

其中真正和 MVCC 最相关的是：

- `xmin`
- `xmax`
- `ctid`
- 一些事务状态相关的 hint bits

## 5. `xmin`、`xmax`、`ctid` 分别是什么

### 5.1 `xmin`

`xmin` 表示“插入这个 tuple 版本的事务 ID”。

举例：

- 事务 T100 执行 `INSERT`
- 新生成的 tuple，其 `xmin = 100`

含义是：

- 这个版本是事务 100 创建的

### 5.2 `xmax`

`xmax` 表示“删除这个 tuple 或者把它更新掉的事务 ID”。

这里要特别注意：

- `DELETE` 会写 `xmax`
- `UPDATE` 也会写 `xmax`

原因很简单：

- 对旧版本来说，被 update 掉，本质上就等价于“这个版本结束了”

### 5.3 `ctid`

`ctid` 是 tuple 当前在物理存储中的位置标识。

在发生 update 时，它还有一个特殊用途：

- 旧版本的 `ctid` 会指向新版本

这就让旧版本和新版本之间形成一条版本链。

简化理解：

- 最新版本：`ctid` 指向自己
- 被更新的旧版本：`ctid` 指向它的新版本

## 6. PostgreSQL 为什么不原地做 UPDATE

这是 PostgreSQL MVCC 最核心的一条。

假设一条记录最初是：

```text
id = 1, balance = 100
```

事务 T1 已经开始读取，事务 T2 要执行：

```sql
UPDATE account SET balance = 200 WHERE id = 1;
```

如果 T2 直接把原来的 tuple 改成 `200`，就会有一个严重问题：

- T1 明明是一个更早开始的事务
- 它本应还能看到旧版本 `100`
- 但因为数据被覆盖，它被迫读到 `200`

这样就破坏了 snapshot 读的一致性。

所以 PostgreSQL 的做法是：

1. 保留旧版本
2. 新写一个新版本
3. 旧版本 `xmax = T2`
4. 新版本 `xmin = T2`
5. 旧版本 `ctid` 指向新版本

于是：

- 老事务还能看到旧版本
- 新事务看到新版本

## 7. DELETE 为什么也不立刻物理删除

原因和 update 类似。

假设事务 T1 在某个 snapshot 下，本来还应该能看到一条记录。

这时事务 T2 执行了 `DELETE`。

如果 T2 立刻把物理 tuple 从页面里抹掉，那么：

- T1 后续再读时，就找不到这条它本该还能看到的数据了

所以 PostgreSQL 的做法不是立即删除，而是：

- 给 tuple 写上 `xmax = T2`

然后：

- 对某些事务，这个版本已经不可见
- 对另一些更早的事务，它仍然可见

等确定没有任何活跃事务还需要它之后，vacuum 才真正回收空间。

## 8. PostgreSQL 中 Snapshot 到底是什么

snapshot 不是数据副本，也不是“把整张表复制一份”。

snapshot 本质上是一组规则，告诉当前事务：

> 在判断可见性时，哪些事务算已经完成，哪些事务算还没完成。

可以把 snapshot 简化理解为三部分：

- `xmin`
- `xmax`
- 活跃事务集合 `xip`

含义是：

- 所有比 `xmin` 更老并且已提交的事务，一般都可以视为可见
- 所有 `>= xmax` 的事务，一定不可见，因为它们属于 snapshot 之后的“未来事务”
- 活跃事务集合里的事务，在 snapshot 生成时还没结束，所以不可见

这就是 PostgreSQL 快照读的基础。

## 9. PostgreSQL 是如何判断 tuple 可见性的

粗略地说，读取一个 tuple 时，PostgreSQL 会做下面两步判断：

1. 看 `xmin` 对当前 snapshot 是否可见
2. 看 `xmax` 对当前 snapshot 是否可见

只有：

- 插入事务可见
- 删除事务不可见

这个 tuple 才会返回给当前查询。

### 9.1 一个简化例子

假设一条 tuple：

```text
xmin = 10
xmax = 20
value = 100
```

这表示：

- 它由事务 10 创建
- 它被事务 20 删除或替换

那么：

- 如果当前事务的 snapshot 看得到 T10 提交，但看不到 T20 提交，这条 tuple 可见
- 如果当前事务既看得到 T10，也看得到 T20，那么这条 tuple 不可见

### 9.2 自事务例外

还有一类特殊情况是：

- 这个 tuple 是当前事务自己插入的
- 或者当前事务自己把它删掉了

这时 PostgreSQL 还会结合命令号 `cmin/cmax` 来处理“自己是否能看到自己刚写的数据”。

所以真正的可见性判断比上面的两步还复杂，但核心思想就是：

> 版本是否返回，不看“它最新不最新”，只看它对当前 snapshot 是否可见。

## 10. 一个完整的 UPDATE 版本链例子

假设最开始有一条记录：

```text
R1: balance = 100
xmin = 5
xmax = 0
ctid = R1
```

现在事务 T40 执行：

```sql
UPDATE account SET balance = 200 WHERE id = 1;
```

PostgreSQL 风格下的结果不是改写 `R1`，而是：

```text
R1: balance = 100, xmin = 5,  xmax = 40, ctid = R2
R2: balance = 200, xmin = 40, xmax = 0,  ctid = R2
```

现在来看不同事务的结果。

### 10.1 T40 自己看到什么

T40 会看到：

```text
balance = 200
```

因为：

- 新版本 `R2` 是自己插入的
- 旧版本 `R1` 是自己结束掉的

### 10.2 更早启动的事务看到什么

假设事务 T39 在 T40 update 之前就拿到了 snapshot。

它看到：

```text
balance = 100
```

因为对它来说：

- `R2.xmin = 40` 不可见
- `R1.xmax = 40` 也不可见

所以旧版本仍然成立。

### 10.3 更晚启动的事务看到什么

等 T40 提交以后，一个更晚启动的事务 T41 会看到：

```text
balance = 200
```

因为：

- 新版本的插入事务已提交，可见
- 旧版本的失效事务也已提交，可见，所以旧版本不再返回

这就是 PostgreSQL MVCC 最核心的行为。

## 11. PostgreSQL 的 MVCC 是否意味着“不需要锁”

不是。

这是一个非常常见的误解。

正确理解是：

- MVCC 主要降低的是读写之间的阻塞
- 但 PostgreSQL 仍然需要锁来处理很多问题

### 11.1 为什么还需要锁

即使有 MVCC，下面这些场景仍然需要锁或类似机制：

- 两个事务同时 update 同一行
- 两个事务同时 delete 同一行
- `SELECT FOR UPDATE`
- DDL 和 DML 之间的并发控制
- Serializable 隔离级别下的冲突检测

### 11.2 PostgreSQL 中锁和 MVCC 是互补关系

可以这么理解：

- MVCC 解决“读哪个版本”
- 锁解决“哪些操作不能同时做”

不是二选一，而是两套机制配合。

## 12. PostgreSQL 中的隔离级别到底有哪些

SQL 标准通常会列四个隔离级别：

1. Read Uncommitted
2. Read Committed
3. Repeatable Read
4. Serializable

但 PostgreSQL 实际上有一个很重要的特点：

> PostgreSQL 不真正区分 Read Uncommitted 和 Read Committed。

在 PostgreSQL 中：

- `READ UNCOMMITTED` 会被当成 `READ COMMITTED`

原因很简单：

- PostgreSQL 的 MVCC 本身就不允许脏读
- 所以它没有去实现真正意义上的“读未提交”

因此，实际可用的核心隔离级别可以理解为三个：

1. Read Committed
2. Repeatable Read
3. Serializable

## 13. PostgreSQL 的 Read Committed 是怎么工作的

Read Committed 是 PostgreSQL 的默认隔离级别。

它的关键规则是：

> 每条语句开始时获取一个新的 snapshot。

注意，是“每条语句”，不是“整个事务”。

### 13.1 这意味着什么

在同一个事务中，两条 `SELECT` 之间，如果别的事务提交了更新，那么第二条 `SELECT` 可以看到新结果。

例子：

```sql
-- T1
BEGIN;
SELECT balance FROM account WHERE id = 1;  -- 看到 100

-- T2
UPDATE account SET balance = 200 WHERE id = 1;
COMMIT;

-- T1
SELECT balance FROM account WHERE id = 1;  -- 在 Read Committed 下可能看到 200
COMMIT;
```

为什么？

因为 T1 的第二条 `SELECT` 会重新拿一次 snapshot。

### 13.2 它能防止什么

Read Committed 可以防止：

- 脏读

也就是说，别的事务未提交的数据你看不到。

### 13.3 它不能防止什么

它不能保证：

- 可重复读

因为同一个事务中，不同语句拿到的 snapshot 可能不一样。

所以：

- 同一行两次读到不同值
- 一次查询前后集合变了

这些都是可能发生的。

## 14. PostgreSQL 的 Repeatable Read 是怎么工作的

PostgreSQL 的 Repeatable Read 和很多教材里讲的“Repeatable Read”很像，但实现方式更接近“事务级 snapshot”。

它的关键规则是：

> 整个事务只拿一次 snapshot，后续所有普通读取都基于这个 snapshot。

### 14.1 这意味着什么

事务开始后，你看到的版本集合基本固定了。

例子：

```sql
-- T1
BEGIN ISOLATION LEVEL REPEATABLE READ;
SELECT balance FROM account WHERE id = 1;  -- 看到 100

-- T2
UPDATE account SET balance = 200 WHERE id = 1;
COMMIT;

-- T1
SELECT balance FROM account WHERE id = 1;  -- 仍然看到 100
COMMIT;
```

因为 T1 整个事务都使用第一次建立的 snapshot。

### 14.2 这和很多数据库的 Repeatable Read 有何不同

这里有一个非常重要的点：

> PostgreSQL 的 Repeatable Read 比很多人印象中的“Repeatable Read”更强。

原因是：

- 它基于真正的事务级 snapshot
- 因此可以避免传统意义上的幻读

也就是说，在 PostgreSQL 里：

- Repeatable Read 通常已经不会出现你在一些教材里看到的那种“幻读”现象

这也是很多人第一次学 PostgreSQL 隔离级别时会困惑的地方。

## 15. PostgreSQL 的 Repeatable Read 为什么能避免很多教材里的幻读

教材里经常这样定义幻读：

> 同一个事务中，两次相同条件的查询，第二次突然多出或少了一些行。

在一些依赖锁实现的数据库里，Repeatable Read 可能仍然挡不住这种情况，所以还要靠更强的范围锁。

但 PostgreSQL 的 Repeatable Read 基于事务级 snapshot：

- 你事务开始时能看到哪些行
- 之后整个事务通常就固定看到这些版本

因此对于普通快照读来说：

- 新插入但在你事务开始后才提交的行
- 对你来说是不可见的

所以很多人熟悉的“幻读”现象，在 PostgreSQL 的 Repeatable Read 下不会以同样方式出现。

这就是为什么：

> PostgreSQL 的隔离级别名称和 SQL 标准一致，但其底层行为可能和你从别的数据库或教材中形成的印象不同。

## 16. 那 PostgreSQL 的 Serializable 又是做什么的

如果 Repeatable Read 已经很强了，那还要 Serializable 干什么？

因为还有一类问题，叫写偏斜（write skew）或更复杂的序列化异常。

这些问题并不是简单的“我两次读到不同结果”，而是：

- 两个事务分别读取一组条件
- 各自基于读到的旧视图做决定
- 最后虽然每个事务单看都合法，但组合起来违反业务约束

这种问题，单靠事务级 snapshot 并不一定能阻止。

PostgreSQL 的 Serializable 使用的是：

> Serializable Snapshot Isolation，简称 SSI

也就是说：

- 它仍然建立在 snapshot 之上
- 但会额外追踪事务之间的读写依赖
- 如果发现这些依赖可能形成不可序列化的结构，就主动让某个事务失败回滚

换句话说：

> Serializable 不是“把所有事务都排队串行执行”，而是“先乐观并发执行，发现不可序列化冲突时再中止事务”。

## 17. 一个 Serializable 要解决的典型例子：写偏斜

假设有一张值班表，规则是：

> 任意时刻至少要有一名医生值班。

当前表中有两条记录：

```text
doctor A on_call = true
doctor B on_call = true
```

现在两个事务并发执行：

### T1

```sql
BEGIN ISOLATION LEVEL REPEATABLE READ;
SELECT count(*) FROM duty WHERE on_call = true;  -- 看到 2
UPDATE duty SET on_call = false WHERE doctor = 'A';
COMMIT;
```

### T2

```sql
BEGIN ISOLATION LEVEL REPEATABLE READ;
SELECT count(*) FROM duty WHERE on_call = true;  -- 看到 2
UPDATE duty SET on_call = false WHERE doctor = 'B';
COMMIT;
```

这两个事务都基于同一个逻辑：

- “现在还有另一个人在值班，所以我下班没问题”

结果是：

- 两个事务都成功提交
- 最终没人值班

这就是典型的写偏斜。

它不是普通的脏读或不可重复读，而是：

- 每个事务都基于一个合法 snapshot
- 但组合后的结果不等价于任何串行顺序

这正是 Serializable 要解决的问题。

在 PostgreSQL 的 Serializable 下，系统会追踪这类读写依赖，并在必要时让某个事务报错回滚。

## 18. PostgreSQL 的隔离级别和“教材式理解”的主要差异

这是你最关心的点之一，单独列出来。

### 18.1 PostgreSQL 没有真正的 Read Uncommitted

教材里有四级隔离，但 PostgreSQL 实际上不会让你读到未提交数据。

所以：

- `READ UNCOMMITTED` 在 PostgreSQL 中等价于 `READ COMMITTED`

### 18.2 PostgreSQL 的 Repeatable Read 比很多教材印象更强

很多教材里，Repeatable Read 仍可能发生幻读。

但 PostgreSQL 的 Repeatable Read 基于事务级 snapshot，对普通查询来说通常已经不会出现那种经典幻读。

### 18.3 Serializable 不是“加更多锁直到全串行”

很多人直觉上以为：

- Serializable 就是把锁加得特别狠

但 PostgreSQL 的 Serializable 更像：

- snapshot isolation + 冲突图检测 + 必要时主动回滚

所以它是一种更聪明的并发控制，而不只是“最重的锁”。

### 18.4 MVCC 不是隔离级别本身

MVCC 只是底层机制之一。

更准确地说：

- MVCC 提供多版本和快照读能力
- 隔离级别定义 snapshot 获取时机和冲突处理策略

所以不能把“有 MVCC”和“隔离级别高”画等号。

## 19. PostgreSQL 中锁在隔离级别里还扮演什么角色

即使在 MVCC 和 snapshot 机制下，锁依然非常关键。

### 19.1 行级写冲突

两个事务同时 update 同一行，不可能都无限并发下去。

PostgreSQL 需要通过行级冲突控制来决定：

- 谁先改
- 谁等待
- 谁失败

### 19.2 显式锁语义

例如：

```sql
SELECT ... FOR UPDATE
SELECT ... FOR SHARE
```

这些语句就是明确要求锁语义，而不仅仅是快照读。

### 19.3 Serializable 的依赖追踪

Serializable 虽然不是简单靠锁实现，但仍然会使用谓词锁等机制来帮助检测危险结构。

所以你不能理解成：

- 有了 MVCC，就不要锁了

而应该理解成：

- PostgreSQL 用 MVCC 处理版本可见性，用锁和 SSI 处理更高层的并发正确性

## 20. 一个从 Read Committed 到 Serializable 的直观对比

我们用同一个场景做对比。

初始数据：

```text
id = 1, balance = 100
```

### 20.1 Read Committed

```sql
-- T1
BEGIN;
SELECT balance FROM account WHERE id = 1;  -- 100

-- T2
UPDATE account SET balance = 200 WHERE id = 1;
COMMIT;

-- T1
SELECT balance FROM account WHERE id = 1;  -- 200
COMMIT;
```

特点：

- 第二条语句重新取 snapshot
- 所以能看到别人的新提交

### 20.2 Repeatable Read

```sql
-- T1
BEGIN ISOLATION LEVEL REPEATABLE READ;
SELECT balance FROM account WHERE id = 1;  -- 100

-- T2
UPDATE account SET balance = 200 WHERE id = 1;
COMMIT;

-- T1
SELECT balance FROM account WHERE id = 1;  -- 仍然 100
COMMIT;
```

特点：

- 整个事务一个 snapshot
- 所以视图稳定

### 20.3 Serializable

如果只是上面这个简单场景，Serializable 和 Repeatable Read 看起来可能没区别。

但在存在更复杂跨行约束时，Serializable 会额外做冲突检测。

也就是说：

- Repeatable Read 可能让两个事务都提交
- Serializable 会发现这种组合不可串行化，于是让其中一个失败

## 21. PostgreSQL 的 MVCC 对实现者意味着什么

如果你要在 `YourSQL` 里实现 PostgreSQL 风格 MVCC，那么需要牢牢记住：

### 21.1 不是“给每行加个删除标记”就结束了

真正要实现的是：

- tuple header
- transaction status
- snapshot
- visibility check
- update version chain
- vacuum

这是一个系统，不是一个字段。

### 21.2 不是“返回最新版本”，而是“返回可见版本”

这点在实现扫描器时最容易写错。

### 21.3 隔离级别的关键不是“锁强度”

对于 PostgreSQL 风格设计，更关键的是：

- snapshot 是语句级还是事务级
- 是否需要额外做 Serializable 冲突检测

### 21.4 MVCC 和锁都要有

只实现 MVCC，不处理写写冲突和更高层并发问题，是不完整的。

## 22. 如果把 PostgreSQL 的隔离级别压缩成一句话

可以这样记：

- `Read Committed`：每条语句一个新 snapshot，只保证不读到未提交数据
- `Repeatable Read`：整个事务一个 snapshot，保证事务内看到稳定视图
- `Serializable`：在事务级 snapshot 基础上，再检测并阻止不可串行化结果

## 23. 如果把 PostgreSQL 的 MVCC 压缩成一句话

可以这样记：

> PostgreSQL 通过给每个 tuple 保存创建事务和失效事务信息，再结合事务 snapshot 决定当前查询能看到哪个版本，从而让大部分读写可以并发进行而不互相阻塞。

## 24. 对你当前疑问的直接回答

你提的问题其实可以直接回答成下面三句：

1. PostgreSQL 当然有隔离级别，而且隔离级别和 MVCC 是紧密结合的。
2. PostgreSQL 的隔离级别名称和标准一致，但底层行为和很多教材或其他数据库中的直觉理解并不完全一样，尤其是 `Read Uncommitted` 和 `Repeatable Read`。
3. PostgreSQL 的 MVCC 主要负责“版本可见性”，隔离级别主要负责“snapshot 何时获取、是否允许更高级并发异常、发生危险冲突时要不要中止事务”。

## 25. 建议和本目录其他文档一起阅读

建议阅读顺序：

1. 先看本文件，建立 PostgreSQL MVCC 和隔离级别的整体认知
2. 再看 [MVCC_EXPLAIN.md](/Users/yanghuan/core/project/cpp-proj/YourSQL/src/transaction/MVCC_EXPLAIN.md)，巩固 MVCC 自身原理
3. 最后看 [README.md](/Users/yanghuan/core/project/cpp-proj/YourSQL/src/transaction/README.md)，对照你当前项目的落地方案

如果你后面还需要，我可以继续补两类文档：

- 一份“PostgreSQL tuple 可见性判断流程”文档，把 `xmin/xmax` 判定拆成接近源码思路的决策流程
- 一份“隔离级别异常案例”文档，把脏读、不可重复读、幻读、写偏斜分别配成时间线示意
