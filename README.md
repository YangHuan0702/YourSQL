# YourDB 🚀

> 一个从零实现的 **单机 OLTP 数据库内核**

---

## 🧭 项目定位

**YourDB 是一个学习型但不玩具化的 OLTP 数据库引擎。**

它关注的是：

* SQL 是如何被解析和执行的
* 数据是如何组织、缓存并落盘的
* 事务、日志、恢复这些“数据库灵魂组件”是如何协同工作的

而不是：

* 分布式系统
* 云原生 / 微服务

> **YourDB 的目标是“可解释性”，而不是“炫技”。**

---

## 🎯 设计目标

* 清晰的模块边界
* 可逐步演进的架构
* 每一阶段都能独立运行
* 尽量避免“为了通用而通用”的设计

---

## 🧱 系统架构（逻辑视图）

```
SQL
 │
 ▼
Parser
 │
 ▼
Executor
 │
 ▼
Storage Engine
 ├── Buffer Pool
 ├── Page / File
 ├── Index (B+Tree)
 ├── WAL
 └── Recovery
```

---

## 🛠️ 当前功能

* [ ] 基础 SQL 支持（CREATE / INSERT / SELECT）
* [ ] 行存储（Heap Table）
* [ ] 基础执行器
* [ ] WHERE 条件过滤
* [ ] 索引
* [ ] WAL
* [ ] Crash Recovery

> 功能以 **“能跑 + 可扩展”** 为第一优先级

---

## 🗺️ 开发路线图

### Phase 1：最小可用数据库（MVP）

* 表 / Schema 管理
* Heap Table
* 顺序扫描

目标：

> **SQL 能执行，数据能查出来**

---

### Phase 2：存储引擎

* Page 抽象
* Buffer Pool
* 磁盘文件管理

目标：

> **数据真正落盘，不靠内存苟活**

---

### Phase 3：索引

* B+Tree
* 点查 / 范围查

目标：

> **理解“为什么数据库能快”**

---

### Phase 4：事务与恢复

* WAL
* 崩溃恢复
* 单线程事务模型

目标：

> **数据库开始“值得信任”**

---

## 🧠 设计原则

* Simple > Clever
* Correct > Fast
* 清晰 > 抽象
* 可验证 > 想当然

---

## 🧪 示例

```sql
CREATE TABLE users (
  id INT,
  name TEXT
);

INSERT INTO users VALUES (1, 'Alice');
INSERT INTO users VALUES (2, 'Bob');

SELECT * FROM users;
```

输出：

```
1 | Alice
2 | Bob
```

---

## 📚 参考资料

* *Database System Concepts*
* *Architecture of a Database System*
* SQLite / PostgreSQL（思想参考，不直接照抄）

---

## 🧘 项目态度

> YourDB 是一个 **长期演进项目**。

* 不追热点
* 不和工业数据库对标
* 不焦虑进度

**慢一点，但每一步都算数。**
---

