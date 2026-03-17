B+ 树核心概念回顾
B+ 树的关键特性：
1. 所有数据都存在叶子节点
2. 叶子节点通过链表连接（你的 next_page_id_）
3. 内部节点只存 key 和子节点指针，用于导航
4. 内部节点的第一个 key 可以为空（或最小值），对应最左子树

BPlusIndexManager 实现思路

核心操作：

1. FindLeafPage - 从根节点向下查找到叶子节点
   - 从 root 开始
   - 在 internal page 用 LookupChild(key) 找下一层
   - 重复直到叶子层
2. Insert - 插入 key-value
   - 用 FindLeafPage 找到目标叶子
   - 调用 leaf 的 Insert
   - 如果叶子满了，调用 SplitLeaf
   - 把中间 key 插入父节点 InsertInfoParent
3. SplitLeaf - 分裂叶子节点
   - 创建新叶子页
   - 把一半数据移到新页
   - 返回新页的第一个 key（作为分隔 key）
   - 更新 next_page_id 链表
4. InsertInfoParent - 向父节点插入分裂信息
   - 找到父节点
   - 调用 InsertAfter(old_child, middle_key, new_child)
   - 如果父节点也满了，递归分裂
5. Delete - 删除操作
   - 找到叶子节点
   - 删除 key-value
   - （可选）处理合并/重分配
6. Get - 查询操作
   - 找到叶子节点
   - 调用 leaf 的 Lookup

BPlusIndexIterator 实现思路

Iterator 用于范围扫描，沿着叶子链表遍历：

1. 构造函数 - 初始化到某个叶子的某个位置
   - 保存 buffer_manager
   - 调用 LoadLeaf 加载页面
   - 设置 index
2. operator* - 返回当前 key-value
   - 从当前 leaf page 读取 array_[index_]
3. operator++ - 移动到下一个元素
   - index_++
   - 如果超出当前页，通过 next_page_id_ 加载下一页
   - 重置 index = 0
4. IsEnd - 判断是否结束
   - is_end_ 标志