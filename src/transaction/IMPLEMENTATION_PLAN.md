# PostgreSQL MVCC 实现方案

## 1. 概述

本方案基于 PostgreSQL 的 MVCC 机制，为 YourSQL 提供完整的多版本并发控制实现。方案遵循 PostgreSQL 的核心设计原则，包括 tuple header 版本元数据、UPDATE 生成新版本、snapshot 可见性判断等。

## 2. 核心模块设计

### 2.1 事务管理器 (TransactionManager)

**职责**：
- 分配全局唯一的事务 ID (TransactionId)
- 管理事务状态表 (IN_PROGRESS, COMMITTED, ABORTED)
- 创建和管理 snapshot
- 维护活跃事务列表

**核心数据结构**：
```
TransactionManager:
  - next_txn_id: atomic<TransactionId>
  - txn_state_table: map<TransactionId, TransactionState>
  - active_txns: set<TransactionId>
  - mutex: 保护并发访问
```

**核心接口**：
- `Begin()`: 开启新事务，分配 txn_id，加入活跃列表
- `Commit(txn)`: 提交事务，更新状态表，从活跃列表移除
- `Abort(txn)`: 回滚事务，更新状态表，从活跃列表移除
- `GetTransactionState(txn_id)`: 查询事务状态
- `CreateSnapshot()`: 创建当前时刻的快照

### 2.2 快照 (Snapshot)

**职责**：
- 记录事务开始时的全局事务状态
- 提供版本可见性判断逻辑

**核心数据结构**：
```
Snapshot:
  - xmin: 最小活跃事务 ID
  - xmax: 下一个将要分配的事务 ID
  - xip: 快照时刻的活跃事务 ID 列表
```

**可见性判断规则**：
```
IsVisible(xid, state):
  1. 如果 xid < xmin，且 state == COMMITTED，可见
  2. 如果 xid >= xmax，不可见
  3. 如果 xid 在 xip 中，不可见
  4. 如果 state == ABORTED，不可见
  5. 如果 state == COMMITTED，可见
  6. 否则不可见
```

### 2.3 事务对象 (Transaction)

**职责**：
- 封装单个事务的上下文信息
- 持有事务的 snapshot
- 记录事务状态

**核心数据结构**：
```
Transaction:
  - txn_id: TransactionId
  - state: TransactionState
  - snapshot: Snapshot
  - isolation_level: IsolationLevel (可选)
```

## 3. 存储层改造

### 3.1 Tuple Header

**设计**：
每个 tuple 前面添加固定大小的 header，包含 MVCC 元数据。

**核心字段**：
```
TupleHeader:
  - xmin: TransactionId (创建该版本的事务 ID)
  - xmax: TransactionId (删除该版本的事务 ID，0 表示未删除)
  - t_ctid: ItemPointer (指向新版本的位置)
  - t_infomask: uint16 (标志位)
```

**标志位定义**：
```
t_infomask:
  - HEAP_XMIN_COMMITTED: xmin 已提交
  - HEAP_XMIN_ABORTED: xmin 已回滚
  - HEAP_XMAX_COMMITTED: xmax 已提交
  - HEAP_XMAX_ABORTED: xmax 已回滚
  - HEAP_XMAX_INVALID: xmax 无效
  - HEAP_UPDATED: 该 tuple 被更新过
```

### 3.2 HeapTuple

**设计**：
封装 tuple header + 用户数据的完整结构。

**核心结构**：
```
HeapTuple:
  - header: TupleHeader*
  - data: char*
  - len: size_t
```

**核心方法**：
- `GetXmin()`: 获取创建事务 ID
- `GetXmax()`: 获取删除事务 ID
- `SetXmin(xid)`: 设置创建事务 ID
- `SetXmax(xid)`: 设置删除事务 ID
- `GetCtid()`: 获取新版本指针
- `SetCtid(ctid)`: 设置新版本指针
- `SetHintBits(flags)`: 设置提示位

### 3.3 TablePage 改造

**当前问题**：
- Slot 中有 `deleted` 标记，与 MVCC 冲突
- 没有版本链支持

**改造方案**：
```
Slot:
  - offset: uint16 (tuple 在页面中的偏移)
  - length: uint16 (tuple 总长度，包含 header)
  移除 deleted 字段
```

**页面布局**：
```
[PageHeader | Slot Array | Free Space | Tuple Data (含 TupleHeader)]
```

**核心方法**：
- `InsertTuple(tuple, txn_id)`: 插入新 tuple，设置 xmin
- `GetTuple(slot_id)`: 获取 tuple，返回 HeapTuple
- `UpdateTupleCtid(old_slot, new_ctid)`: 更新版本链指针

## 4. 可见性判断模块

### 4.1 VisibilityChecker

**职责**：
- 根据 snapshot 和 tuple header 判断版本是否可见
- 实现 PostgreSQL 的可见性规则

**核心接口**：
```
IsTupleVisible(tuple, snapshot, txn_manager):
  1. 获取 tuple 的 xmin 和 xmax
  2. 检查 xmin 可见性
  3. 检查 xmax 可见性
  4. 返回最终判断结果
```

**详细逻辑**：
```
IsTupleVisible:
  xmin = tuple.GetXmin()
  xmax = tuple.GetXmax()

  // 检查创建事务
  if xmin == INVALID_TXN_ID:
    return false

  xmin_state = txn_manager.GetTransactionState(xmin)
  if xmin_state == ABORTED:
    return false

  if xmin_state == IN_PROGRESS:
    if xmin in snapshot.xip:
      return false

  if not snapshot.IsVisible(xmin, xmin_state):
    return false

  // 检查删除事务
  if xmax == INVALID_TXN_ID:
    return true

  xmax_state = txn_manager.GetTransactionState(xmax)
  if xmax_state == ABORTED:
    return true

  if xmax_state == IN_PROGRESS:
    if xmax in snapshot.xip:
      return true

  if not snapshot.IsVisible(xmax, xmax_state):
    return true

  return false
```

## 5. DML 操作实现

### 5.1 INSERT

**流程**：
1. 分配新的 tuple 空间
2. 设置 tuple header:
   - xmin = 当前事务 ID
   - xmax = INVALID_TXN_ID
   - t_ctid = 自身位置
3. 写入用户数据
4. 插入到 TablePage

**伪代码**：
```
Insert(table, tuple, txn):
  heap_tuple = CreateHeapTuple(tuple)
  heap_tuple.SetXmin(txn.GetTransactionId())
  heap_tuple.SetXmax(INVALID_TXN_ID)
  heap_tuple.SetCtid(INVALID_CTID)

  page = table.GetInsertPage()
  slot_id = page.InsertTuple(heap_tuple)

  // 更新 ctid 指向自身
  ctid = MakeCtid(page.GetPageId(), slot_id)
  heap_tuple.SetCtid(ctid)
```

### 5.2 DELETE

**流程**：
1. 找到要删除的 tuple
2. 检查可见性
3. 设置 xmax = 当前事务 ID
4. 设置 HEAP_UPDATED 标志

**伪代码**：
```
Delete(table, key, txn, snapshot):
  tuple = table.FindTuple(key)

  if not IsTupleVisible(tuple, snapshot, txn_manager):
    return false

  if tuple.GetXmax() != INVALID_TXN_ID:
    // 已被其他事务删除或更新
    return false

  tuple.SetXmax(txn.GetTransactionId())
  tuple.SetHintBits(HEAP_UPDATED)

  return true
```

### 5.3 UPDATE

**流程**：
1. 找到旧版本 tuple
2. 检查可见性
3. 标记旧版本：设置 xmax = 当前事务 ID
4. 插入新版本：设置 xmin = 当前事务 ID
5. 建立版本链：旧版本的 t_ctid 指向新版本

**伪代码**：
```
Update(table, key, new_data, txn, snapshot):
  old_tuple = table.FindTuple(key)

  if not IsTupleVisible(old_tuple, snapshot, txn_manager):
    return false

  if old_tuple.GetXmax() != INVALID_TXN_ID:
    // 已被其他事务删除或更新
    return false

  // 标记旧版本
  old_tuple.SetXmax(txn.GetTransactionId())
  old_tuple.SetHintBits(HEAP_UPDATED)

  // 插入新版本
  new_tuple = CreateHeapTuple(new_data)
  new_tuple.SetXmin(txn.GetTransactionId())
  new_tuple.SetXmax(INVALID_TXN_ID)

  page = table.GetInsertPage()
  new_slot_id = page.InsertTuple(new_tuple)
  new_ctid = MakeCtid(page.GetPageId(), new_slot_id)

  // 建立版本链
  old_tuple.SetCtid(new_ctid)
  new_tuple.SetCtid(new_ctid)

  return true
```

### 5.4 SELECT

**流程**：
1. 获取事务的 snapshot
2. 扫描 table，对每个 tuple 进行可见性判断
3. 只返回可见的 tuple
4. 如果遇到版本链，沿着 t_ctid 查找可见版本

**伪代码**：
```
Select(table, txn):
  snapshot = txn.GetSnapshot()
  results = []

  for page in table.GetPages():
    for slot_id in page.GetSlots():
      tuple = page.GetTuple(slot_id)

      if IsTupleVisible(tuple, snapshot, txn_manager):
        results.append(tuple)

  return results
```

## 6. 事务提交与回滚

### 6.1 Commit

**流程**：
1. 更新事务状态表：state = COMMITTED
2. 从活跃事务列表移除
3. 设置 tuple 的提示位（可选优化）

**伪代码**：
```
Commit(txn):
  txn_id = txn.GetTransactionId()

  lock(mutex):
    txn_state_table[txn_id] = COMMITTED
    active_txns.remove(txn_id)

  // 可选：设置提示位
  for tuple in txn.GetModifiedTuples():
    if tuple.GetXmin() == txn_id:
      tuple.SetHintBits(HEAP_XMIN_COMMITTED)
    if tuple.GetXmax() == txn_id:
      tuple.SetHintBits(HEAP_XMAX_COMMITTED)
```

### 6.2 Abort

**流程**：
1. 更新事务状态表：state = ABORTED
2. 从活跃事务列表移除
3. 设置 tuple 的提示位（可选优化）

**伪代码**：
```
Abort(txn):
  txn_id = txn.GetTransactionId()

  lock(mutex):
    txn_state_table[txn_id] = ABORTED
    active_txns.remove(txn_id)

  // 可选：设置提示位
  for tuple in txn.GetModifiedTuples():
    if tuple.GetXmin() == txn_id:
      tuple.SetHintBits(HEAP_XMIN_ABORTED)
    if tuple.GetXmax() == txn_id:
      tuple.SetHintBits(HEAP_XMAX_ABORTED)
```

## 7. Vacuum 机制

### 7.1 Page Prune

**职责**：
- 清理页面内的死 tuple
- 回收空间供新 tuple 使用

**触发时机**：
- 页面空间不足时
- 手动触发

**流程**：
```
PrunePage(page):
  for slot_id in page.GetSlots():
    tuple = page.GetTuple(slot_id)

    if IsDeadTuple(tuple):
      page.RemoveTuple(slot_id)
      page.CompactSpace()
```

**死 tuple 判断**：
```
IsDeadTuple(tuple):
  xmin = tuple.GetXmin()
  xmax = tuple.GetXmax()

  xmin_state = txn_manager.GetTransactionState(xmin)
  if xmin_state == ABORTED:
    return true

  if xmax == INVALID_TXN_ID:
    return false

  xmax_state = txn_manager.GetTransactionState(xmax)
  if xmax_state == COMMITTED:
    // 检查是否所有活跃事务都看不到这个版本
    if xmax < GetOldestActiveTransaction():
      return true

  return false
```

### 7.2 Vacuum

**职责**：
- 全表扫描，清理死 tuple
- 更新统计信息
- 回收空间

**流程**：
```
Vacuum(table):
  for page in table.GetPages():
    PrunePage(page)

  table.UpdateStatistics()
```

## 8. 隔离级别支持

### 8.1 Read Committed

**实现**：
- 每条 SQL 语句执行前创建新 snapshot
- 可以看到其他已提交事务的修改

**伪代码**：
```
ExecuteStatement(sql, txn):
  snapshot = txn_manager.CreateSnapshot()
  txn.SetSnapshot(snapshot)

  result = Execute(sql, txn)
  return result
```

### 8.2 Repeatable Read

**实现**：
- 事务开始时创建 snapshot
- 整个事务期间使用同一个 snapshot
- 不会看到事务开始后其他事务的修改

**伪代码**：
```
Begin():
  txn = txn_manager.Begin()
  snapshot = txn_manager.CreateSnapshot()
  txn.SetSnapshot(snapshot)
  return txn

ExecuteStatement(sql, txn):
  // 使用事务开始时的 snapshot
  result = Execute(sql, txn)
  return result
```

### 8.3 Serializable (可选)

**实现**：
- 基于 Repeatable Read
- 额外检测读写冲突
- 使用谓词锁或 SSI (Serializable Snapshot Isolation)

## 9. 优化策略

### 9.1 提示位 (Hint Bits)

**目的**：
- 避免重复查询事务状态表
- 加速可见性判断

**实现**：
- 在 t_infomask 中设置 COMMITTED/ABORTED 标志
- 第一次查询后缓存结果

### 9.2 HOT Update (可选)

**目的**：
- 减少索引更新开销
- 提高 UPDATE 性能

**条件**：
- UPDATE 不修改索引列
- 新版本可以放在同一页面

### 9.3 版本链优化

**策略**：
- 限制版本链长度
- 及时 prune 死版本
- 考虑使用 undo log (可选)

## 10. 实现顺序建议

按照以下顺序逐步实现，每个阶段完成后进行测试：

1. **阶段 1：事务管理器基础**
   - TransactionManager
   - Transaction
   - Snapshot
   - 事务状态表

2. **阶段 2：存储层改造**
   - TupleHeader
   - HeapTuple
   - TablePage 改造

3. **阶段 3：可见性判断**
   - VisibilityChecker
   - 可见性规则实现

4. **阶段 4：INSERT 支持**
   - 实现 INSERT 逻辑
   - 测试基本插入

5. **阶段 5：SELECT 支持**
   - 改造 SeqScan
   - 实现 snapshot 过滤
   - 测试可见性

6. **阶段 6：DELETE 支持**
   - 实现 DELETE 逻辑
   - 测试删除可见性

7. **阶段 7：UPDATE 支持**
   - 实现版本链
   - 测试 UPDATE 逻辑

8. **阶段 8：事务提交回滚**
   - 完善 Commit/Abort
   - 测试事务隔离

9. **阶段 9：Vacuum**
   - 实现 Page Prune
   - 实现 Vacuum

10. **阶段 10：隔离级别**
    - 实现 Read Committed
    - 实现 Repeatable Read

## 11. 测试策略

### 11.1 单元测试

- 事务管理器测试
- Snapshot 可见性测试
- Tuple Header 操作测试
- 可见性判断测试

### 11.2 集成测试

- 并发 INSERT 测试
- 并发 UPDATE 测试
- 并发 DELETE 测试
- 事务隔离级别测试
- Vacuum 测试

### 11.3 性能测试

- 大量并发事务
- 长事务场景
- 版本链长度测试
- Vacuum 性能测试

## 12. 注意事项

1. **事务 ID 回卷**：TransactionId 是 uint32，需要考虑回卷问题
2. **死锁检测**：UPDATE 可能导致死锁，需要检测机制
3. **内存管理**：旧版本会占用大量空间，需要及时 vacuum
4. **并发控制**：事务状态表需要加锁保护
5. **崩溃恢复**：第一版可以不实现，但要预留接口

## 13. 参考资料

- PostgreSQL 官方文档：MVCC 章节
- PostgreSQL 源码：src/backend/access/heap/
- 《PostgreSQL 数据库内核分析》
- 本目录下的 MVCC_EXPLAIN.md 和 POSTGRES_MVCC_AND_ISOLATION.md
