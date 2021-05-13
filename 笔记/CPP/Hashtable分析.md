---
title: Hashtable分析
categories: CPP
tags:
 - CPP
---

## _Hashtable
STL中并没有提供直接提供hashtable，而是通过将其封装为unordered_map和unordered_set来供外部使用，也可以说它们是由_Hashtable来实现的。
其_Hashtable的结构如下，其与传统的hash list有相同点但也存在些许不同。
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200831203225910.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzM3NjU0NzA0,size_16,color_FFFFFF,t_70#pic_center)**同**：

1. _M_buckets为nodes指针数组，其保存了node的地址。而nodes中主要保存了2个元素，分别为next指针和value。
2. 增删改查过程基本相似，主要分为两个步骤。首先计算hash(key)，获取的bucket_index，也就是插入的链表位置。之后便是插入进入链表
3. 同样面临rehash问题

**异**

1. bucket中的list中包含头节点，也就是forword list中的begin_before节点。引入此节点后其插入删除操作将得到统一。
2. 传统hash链表结构的链表最后一个节点会指向nullptr，STL中的则不同，其会指向next_bucket的链表的第一个node，参照上图中的红色线，我们可以发现所有的节点相连为一个forward list结构，这种结构将很方便地完成迭代器的遍历(仅单向遍历，所以unorder_set/map 返回的是LegacyForwardIterator)。但是这样结构也对判断是否到达list 尾部增加了额外计算量，我们无法像之前通过node->next == nullptr来判断其是否为last节点。
3. list中包含头节点，其由对应的buckets[]所指向，在图中为绿色箭头所指向的node，我们可以看到其指向了previous bucket的last node，通过这种方法可以很轻易地实现头节点并节省一个节点的空间。但是此方法也会造成当我们修改last node是我们还需要修改bucket[]中存储的数据。
4. _M_before_begin是一个特殊节点，用于解决第一个node没有previous begin的问题。

`note:`图中节点排列顺序按照bucket顺序进行连接是为了构图简单，实质上bucket list并不存在明显的先后顺序，其顺序通常由构造顺序所决定。

#### _HashTable 声明
在源码中hashtable有完整的注释，其注解内容如下。
```cpp
  /**
   *  Primary class template _Hashtable.
   *
   *  @ingroup hashtable-detail
   *
   *  @tparam _Value  CopyConstructible type.
   *
   *  @tparam _Key    CopyConstructible type.
   *
   *  @tparam _Alloc  An allocator type
   *  ([lib.allocator.requirements]) whose _Alloc::value_type is
   *  _Value.  As a conforming extension, we allow for
   *  _Alloc::value_type != _Value.
   *
   *  @tparam _ExtractKey  Function object that takes an object of type
   *  _Value and returns a value of type _Key.
   *
   *  @tparam _Equal  Function object that takes two objects of type k
   *  and returns a bool-like value that is true if the two objects
   *  are considered equal.
   *
   *  @tparam _H1  The hash function. A unary function object with
   *  argument type _Key and result type size_t. Return values should
   *  be distributed over the entire range [0, numeric_limits<size_t>:::max()].
   *
   *  @tparam _H2  The range-hashing function (in the terminology of
   *  Tavori and Dreizin).  A binary function object whose argument
   *  types and result type are all size_t.  Given arguments r and N,
   *  the return value is in the range [0, N).
   *
   *  @tparam _Hash  The ranged hash function (Tavori and Dreizin). A
   *  binary function whose argument types are _Key and size_t and
   *  whose result type is size_t.  Given arguments k and N, the
   *  return value is in the range [0, N).  Default: hash(k, N) =
   *  h2(h1(k), N).  If _Hash is anything other than the default, _H1
   *  and _H2 are ignored.
   *
   *  @tparam _RehashPolicy  Policy class with three members, all of
   *  which govern the bucket count. _M_next_bkt(n) returns a bucket
   *  count no smaller than n.  _M_bkt_for_elements(n) returns a
   *  bucket count appropriate for an element count of n.
   *  _M_need_rehash(n_bkt, n_elt, n_ins) determines whether, if the
   *  current bucket count is n_bkt and the current element count is
   *  n_elt, we need to increase the bucket count.  If so, returns
   *  make_pair(true, n), where n is the new bucket count.  If not,
   *  returns make_pair(false, <anything>)
   *
   *  @tparam _Traits  Compile-time class with three boolean
   *  std::integral_constant members:  __cache_hash_code, __constant_iterators,
   *   __unique_keys.
   *
   *  Each _Hashtable data structure has:
   *
   *  - _Bucket[]       _M_buckets
   *  - _Hash_node_base _M_before_begin
   *  - size_type       _M_bucket_count
   *  - size_type       _M_element_count
   *
   *  with _Bucket being _Hash_node* and _Hash_node containing:
   *
   *  - _Hash_node*   _M_next
   *  - Tp            _M_value
   *  - size_t        _M_hash_code if cache_hash_code is true
   *
   *  In terms of Standard containers the hashtable is like the aggregation of:
   *
   *  - std::forward_list<_Node> containing the elements
   *  - std::vector<std::forward_list<_Node>::iterator> representing the buckets
   *
   *  The non-empty buckets contain the node before the first node in the
   *  bucket. This design makes it possible to implement something like a
   *  std::forward_list::insert_after on container insertion and
   *  std::forward_list::erase_after on container erase
   *  calls. _M_before_begin is equivalent to
   *  std::forward_list::before_begin. Empty buckets contain
   *  nullptr.  Note that one of the non-empty buckets contains
   *  &_M_before_begin which is not a dereferenceable node so the
   *  node pointer in a bucket shall never be dereferenced, only its
   *  next node can be.
   *
   *  Walking through a bucket's nodes requires a check on the hash code to
   *  see if each node is still in the bucket. Such a design assumes a
   *  quite efficient hash functor and is one of the reasons it is
   *  highly advisable to set __cache_hash_code to true.
   *
   *  The container iterators are simply built from nodes. This way
   *  incrementing the iterator is perfectly efficient independent of
   *  how many empty buckets there are in the container.
   *
   *  On insert we compute the element's hash code and use it to find the
   *  bucket index. If the element must be inserted in an empty bucket
   *  we add it at the beginning of the singly linked list and make the
   *  bucket point to _M_before_begin. The bucket that used to point to
   *  _M_before_begin, if any, is updated to point to its new before
   *  begin node.
   *
   *  On erase, the simple iterator design requires using the hash
   *  functor to get the index of the bucket to update. For this
   *  reason, when __cache_hash_code is set to false the hash functor must
   *  not throw and this is enforced by a static assertion.
   *
   *  Functionality is implemented by decomposition into base classes,
   *  where the derived _Hashtable class is used in _Map_base,
   *  _Insert, _Rehash_base, and _Equality base classes to access the
   *  "this" pointer. _Hashtable_base is used in the base classes as a
   *  non-recursive, fully-completed-type so that detailed nested type
   *  information, such as iterator type and node type, can be
   *  used. This is similar to the "Curiously Recurring Template
   *  Pattern" (CRTP) technique, but uses a reconstructed, not
   *  explicitly passed, template pattern.
   *
   *  Base class templates are: 
   *    - __detail::_Hashtable_base
   *    - __detail::_Map_base
   *    - __detail::_Insert
   *    - __detail::_Rehash_base
   *    - __detail::_Equality
   */
  template<typename _Key, typename _Value, typename _Alloc,
	   typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash,
	   typename _RehashPolicy, typename _Traits>
    class _Hashtable
    : public __detail::_Hashtable_base<_Key, _Value, _ExtractKey, _Equal,
				       _H1, _H2, _Hash, _Traits>,
      public __detail::_Map_base<_Key, _Value, _Alloc, _ExtractKey, _Equal,
				 _H1, _H2, _Hash, _RehashPolicy, _Traits>,
      public __detail::_Insert<_Key, _Value, _Alloc, _ExtractKey, _Equal,
			       _H1, _H2, _Hash, _RehashPolicy, _Traits>,
      public __detail::_Rehash_base<_Key, _Value, _Alloc, _ExtractKey, _Equal,
				    _H1, _H2, _Hash, _RehashPolicy, _Traits>,
      public __detail::_Equality<_Key, _Value, _Alloc, _ExtractKey, _Equal,
				 _H1, _H2, _Hash, _RehashPolicy, _Traits>,
      private __detail::_Hashtable_alloc<
	__alloc_rebind<_Alloc,
		       __detail::_Hash_node<_Value,
					    _Traits::__hash_cached::value>>>
```
#### 数据成员
```cpp
      __bucket_type*		_M_buckets		= &_M_single_bucket;   // __bucket_type 实质上为node**
      size_type			_M_bucket_count		= 1;
      __node_base		_M_before_begin;
      size_type			_M_element_count	= 0;
      _RehashPolicy		_M_rehash_policy;


      // A single bucket used when only need for 1 bucket. Especially
      // interesting in move semantic to leave hashtable with only 1 buckets
      // which is not allocated so that we can have those operations noexcept
      // qualified.
      // Note that we can't leave hashtable with 0 bucket without adding
      // numerous checks in the code to avoid 0 modulus.

      __bucket_type		_M_single_bucket	= nullptr;
```
常见的有`bucket`数组`_M_buckets`用于索引链表。`_M_bucket_count`用于记录bucket大小，`_M_before_begin`便是first bucket的before begin，`_M_element_count`用于记录数据个数，_RehashPolicy用于对rehash行为进行管理。对于hashtable有一个负载的概念，也就是load = _M_element_count / _M_bucket_count。load表示平均bucket的list长度。当load较大时，其性能会出现明显下降，此时我们需要扩充bucket大小，并进行重新插入，也就是rehash操作。
这里有一个特殊的变量_M_single_bucket，此变量仅用于bucket_count == 1时，这样在进行移动语义操作时我们可以不必再分配bucket空间。

---
hashtable的node与forward list中的node相似，都是仅包含next而不包含previous。其定义如下
```cpp
  /* @chapter _Hash_node  */
  struct _Hash_node_base			// node基类，其包含_M_nxt即next指针
  {
    _Hash_node_base* _M_nxt;

    _Hash_node_base() noexcept : _M_nxt() { }

    _Hash_node_base(_Hash_node_base* __next) noexcept : _M_nxt(__next) { }
  };

    /**
   *  struct _Hash_node_value_base
   *
   *  Node type with the value to store.
   */
  template<typename _Value>
    struct _Hash_node_value_base : _Hash_node_base			// node中包含value
    {
      typedef _Value value_type;	

      __gnu_cxx::__aligned_buffer<_Value> _M_storage;

      _Value*
      _M_valptr() noexcept
      { return _M_storage._M_ptr(); }

      const _Value*
      _M_valptr() const noexcept
      { return _M_storage._M_ptr(); }

      _Value&
      _M_v() noexcept
      { return *_M_valptr(); }

      const _Value&
      _M_v() const noexcept
      { return *_M_valptr(); }
    };

  /**
   *  Primary template struct _Hash_node.
   *  _Hash_node 的基本模板结构
   */
  template<typename _Value, bool _Cache_hash_code>
    struct _Hash_node;
/*
 * 这里使用了偏特化技术，创建了两种hashnode，一种为带有hashcode cache的node，其_Cache_hash_code为true，另一种则不是。
*/
  /**
   *  Specialization for nodes with caches, struct _Hash_node.
   * 
   *  Base class is __detail::_Hash_node_value_base.
   */
  template<typename _Value>
    struct _Hash_node<_Value, true> : _Hash_node_value_base<_Value>    
    {
      std::size_t  _M_hash_code;        // 用于存储hashcode，由此可见hashcode的类型为size_t
      _Hash_node*
      _M_next() const noexcept
      { return static_cast<_Hash_node*>(this->_M_nxt); }
    };

  /**
   *  Specialization for nodes without caches, struct _Hash_node.
   *
   *  Base class is __detail::_Hash_node_value_base.
   */
  template<typename _Value>
    struct _Hash_node<_Value, false> : _Hash_node_value_base<_Value>
    {
      _Hash_node*						// 不缓存，其未定义任何额外数据
      _M_next() const noexcept
      { return static_cast<_Hash_node*>(this->_M_nxt); }
    };
```
hashcode是否需要缓存是根据需求而言，当我们缓存hashcode时会造成node节点需要额外的size_t存储hashcode，造成存储密度降低，但是我们在rehash的过程中却不必再对其hash值进行任何计算。相反则是存储密度高，计算性能的浪费。对于某些特殊的变量，如string。其hash计算过程较为复杂，如果我们不存储器hash值将会造成很大计算浪费。
这里看一下string的hash计算过程
```cpp
  template<>
    struct hash<string>          // basic_string.h 中特化了一个hash<T>，此函数用于hash过程
    : public __hash_base<size_t, string>
    {
      size_t
      operator()(const string& __s) const noexcept
      { return std::_Hash_impl::hash(__s.data(), __s.length()); }  // 传递字符串数组，length
    };

  struct _Hash_impl
  {
    static size_t									// hash函数调用如下，其会传递如下内容
    hash(const void* __ptr, size_t __clength,
	 size_t __seed = static_cast<size_t>(0xc70f6907UL))  // 此处增加了一个seed，但是是写死的
    { return _Hash_bytes(__ptr, __clength, __seed); }    

    template<typename _Tp>
      static size_t
      hash(const _Tp& __val)
      { return hash(&__val, sizeof(__val)); }

    template<typename _Tp>
      static size_t
      __hash_combine(const _Tp& __val, size_t __hash)		
      { return hash(&__val, sizeof(__val), __hash); }
  };


  // Hash function implementation for the nontrivial specialization.
  // All of them are based on a primitive that hashes a pointer to a
  // byte array. The actual hash algorithm is not guaranteed to stay
  // the same from release to release -- it may be updated or tuned to
  // improve hash quality or speed.
  /*
   * 所有的hash操作都指向一个原语，即hash一个字节数组。我在这里未找到相应的源代码可能是由编译器通过汇编来实现此函数
  */
  size_t
  _Hash_bytes(const void* __ptr, size_t __len, size_t __seed);
```
通过上面的代码不难看出，hash string时我们需要对其保存的所有数据进行hash操作，如果string很大，将会产生很大的计算量。
#### construct
默认构造函数如下
```cpp
_Hashtable() = default;
```
此构造函数传入__bucket_hint，作用于最后的bucket_size大小。
```cpp
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _Hashtable(size_type __bucket_hint,
	       const _H1& __h1, const _H2& __h2, const _Hash& __h,
	       const _Equal& __eq, const _ExtractKey& __exk,
	       const allocator_type& __a)
      : _Hashtable(__h1, __h2, __h, __eq, __exk, __a)
    {
      auto __bkt = _M_rehash_policy._M_next_bkt(__bucket_hint);         // 访问_M_rehash_policy，获取bucket的合适大小
      if (__bkt > _M_bucket_count)
	{
	  _M_buckets = _M_allocate_buckets(__bkt);                          // 如果__bkt大于_M_bucket_count（初始化时为1），则重新分配bucket的空间
	  _M_bucket_count = __bkt;
	}
    }
```
此构造函数传入InputIterator __f, __t。用于在构造时通过其他容器初始化
```cpp

  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    template<typename _InputIterator>
      _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, _Traits>::
      _Hashtable(_InputIterator __f, _InputIterator __l,
		 size_type __bucket_hint,
		 const _H1& __h1, const _H2& __h2, const _Hash& __h,
		 const _Equal& __eq, const _ExtractKey& __exk,
		 const allocator_type& __a)
	: _Hashtable(__h1, __h2, __h, __eq, __exk, __a)
      {
	auto __nb_elems = __detail::__distance_fw(__f, __l);                 // 获取迭代器之间的数据个数
	auto __bkt_count =
	  _M_rehash_policy._M_next_bkt(                                      // 
	    std::max(_M_rehash_policy._M_bkt_for_elements(__nb_elems),       // _M_bkt_for_elements 用于获取元素数量对应的bucket数量  
		     __bucket_hint));                                            // __bucket_hint 传入的bucket数量   

	if (__bkt_count > _M_bucket_count)                                   // 数量不够分配bucket
	  {
	    _M_buckets = _M_allocate_buckets(__bkt_count);
	    _M_bucket_count = __bkt_count;
	  }

	for (; __f != __l; ++__f)
	  this->insert(*__f);                                                // 调用insert方法循环插入迭代器中的数据
      }
```
_M_allocate_bucket进行bucket内存分配，其需要处理当bkt_count为1时的特殊情况，其源码如下
```cpp
    __bucket_type*
    _M_allocate_buckets(size_type __n)
    {
if (__builtin_expect(__n == 1, false))                                 // __n == 1，其_M_single_bucket将指向single bucket，此时不必对bucket进行内存分配
    {
    _M_single_bucket = nullptr;
    return &_M_single_bucket;
    }

return __hashtable_alloc::_M_allocate_buckets(__n);                   // 此时需要进行内存分配
    }

  template<typename _NodeAlloc>
    typename _Hashtable_alloc<_NodeAlloc>::__bucket_type*
    _Hashtable_alloc<_NodeAlloc>::_M_allocate_buckets(std::size_t __n)
    {
      __bucket_alloc_type __alloc(_M_node_allocator());         // 初始化allocator

      auto __ptr = __bucket_alloc_traits::allocate(__alloc, __n);   // 分配内存
      __bucket_type* __p = std::__addressof(*__ptr);                // 
      __builtin_memset(__p, 0, __n * sizeof(__bucket_type));        // 初始化bucket中的内部元素
      return __p;
    }
```

#### insert
其实hashtable中可以存在不同的key，或者相同的key，其会导致插入时行为的不同。因此hashtable提供了两个接口。
`_M_insert_unique_node`用于hashtable中不存在相同key时进行插入
```cpp
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_insert_unique_node(size_type __bkt, __hash_code __code,
			  __node_type* __node)
    -> iterator
    {
      const __rehash_state& __saved_state = _M_rehash_policy._M_state();
      std::pair<bool, std::size_t> __do_rehash
	= _M_rehash_policy._M_need_rehash(_M_bucket_count, _M_element_count, 1);            // 通过 policy 判断是否需要rehash

      __try
	{
	  if (__do_rehash.first)
	    {
	      _M_rehash(__do_rehash.second, __saved_state);                                 // rehash一下
	      __bkt = _M_bucket_index(this->_M_extract()(__node->_M_v()), __code);          // 重新计算__bkt(bucket的位置)
	    }

	  this->_M_store_code(__node, __code);                                              // 存储 __code

	  // Always insert at the beginning of the bucket.
	  _M_insert_bucket_begin(__bkt, __node);                                            // 插入到bucket的首位置，可就是头插法
	  ++_M_element_count;                                                               // 元素数量 + 1
	  return iterator(__node);
	}
      __catch(...)
	{
	  this->_M_deallocate_node(__node);                                                 // 删除元素
	  __throw_exception_again;
	}
    }
```
`_M_insert_bucket_begin`需要分两种情况处理，当插入bucket存在元素时，我们直接进行头插法处理即可。但是如果不存在元素时，我们需要为其分配previous begin。这里STL的处理是：变更first bucket 为当前 current bucket。为了完成变更我们需要进行如下步骤，移动_M_before_begin至current bucket，将previous first bucket的previous begin变更为current bucket的last node，也就是insered node。
```cpp
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    void
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_insert_bucket_begin(size_type __bkt, __node_type* __node)
    {
      if (_M_buckets[__bkt])                                        // 此bucket后面存在链表元素
	{
	  // Bucket is not empty, we just need to insert the new node
	  // after the bucket before begin.                             // before begin 就是单链表的头节点, hashtable中
	  __node->_M_nxt = _M_buckets[__bkt]->_M_nxt;                   // 插入头部
	  _M_buckets[__bkt]->_M_nxt = __node;                           
	}
      else
	{
	  // The bucket is empty, the new node is inserted at the       // bucket 为空
	  // beginning of the singly-linked list and the bucket will
	  // contain _M_before_begin pointer.                           // bucket不仅会包含新的节点还会包含_M_before_begin pointer
	  __node->_M_nxt = _M_before_begin._M_nxt;                      // 链接 previous first bucket list  
	  _M_before_begin._M_nxt = __node;                              // current bucket 获得 _M_before_begin
	  if (__node->_M_nxt)
	    // We must update former begin bucket that is pointing to   // 我们必须更新previous first bucket所对应的bucket[ ]元素值 ，这个将指向__node 本身
	    // _M_before_begin.
	    _M_buckets[_M_bucket_index(__node->_M_next())] = __node;    // 
	  _M_buckets[__bkt] = &_M_before_begin;                         // current bucket 拥有 _M_before_begin
	}
    }
```
` _M_insert_multi_node`用于对存在重复key的hashtable进行插入时使用，其会将新节点插入到相等元素的前头，如果不存在相等元素则插入到list头部。
```cpp
  // Insert node, in bucket bkt if no rehash (assumes no element with its key
  // already present). Take ownership of the node, deallocate it on exception.
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_insert_multi_node(__node_type* __hint, __hash_code __code,
			 __node_type* __node)
    -> iterator
    {
      const __rehash_state& __saved_state = _M_rehash_policy._M_state();		   // 判断是否需要rehash
      std::pair<bool, std::size_t> __do_rehash
	= _M_rehash_policy._M_need_rehash(_M_bucket_count, _M_element_count, 1);

      __try
	{
	  if (__do_rehash.first)
	    _M_rehash(__do_rehash.second, __saved_state);

	  this->_M_store_code(__node, __code);											
	  const key_type& __k = this->_M_extract()(__node->_M_v());                     // __node 中存储key
	  size_type __bkt = _M_bucket_index(__k, __code);                               // __bkt 获取bucket的index

	  // Find the node before an equivalent one or use hint if it exists and
	  // if it is equivalent.
	  __node_base* __prev
	    = __builtin_expect(__hint != nullptr, false)								
	      && this->_M_equals(__k, __code, __hint)                                   // __hint存在 && hint->key 与 node->key 相同直接使用hint
		? __hint                                                                    
		: _M_find_before_node(__bkt, __k, __code);                                  // 否则自己查找prev_node
	  if (__prev)                                                                   // 存在相等的节点
	    {
	      // Insert after the node before the equivalent one.                       // 插入到相等node之前的节点之后
	      __node->_M_nxt = __prev->_M_nxt;
	      __prev->_M_nxt = __node;
	      if (__builtin_expect(__prev == __hint, false))
	      	// hint might be the last bucket node, in this case we need to          // hint可能是最后一个bucket节点
	      	// update next bucket.
	      	if (__node->_M_nxt
	      	    && !this->_M_equals(__k, __code, __node->_M_next()))
	      	  {
	      	    size_type __next_bkt = _M_bucket_index(__node->_M_next());
	      	    if (__next_bkt != __bkt)
	      	      _M_buckets[__next_bkt] = __node;                                  // 此时我们需要更改previous begin
	      	  }
	    }
	  else
	    // The inserted node has no equivalent in the                               
	    // hashtable. We must insert the new node at the
	    // beginning of the bucket to preserve equivalent
	    // elements' relative positions.
        /* 插入节点不存在相等对象，我们必须将新的节点插入到bucket的开头，来保证相等元素的相对位置（其实放末尾也可以，但是修改last node 将会导致next bucket的previous begin变动），较麻烦*/
        /* 这里只要不插入到中间就行，因为插入到中间可以会分隔两个相等元素，multiple hashtable中相等元素必相连*/
	    _M_insert_bucket_begin(__bkt, __node);
	  ++_M_element_count;
	  return iterator(__node);
	}
      __catch(...)
	{
	  this->_M_deallocate_node(__node);
	  __throw_exception_again;
	}
    }
```
`_M_find_before_node`就是传统的list查找算法，不过需要注意的是last node的next并不是nullptr，所以对last node的判断需要计算其next的bucket index，如果next的bucket index不等于current bucket index，表示其为last node。
这里我们也能看到存储hashcode的重要性。
```cpp
  // Find the node whose key compares equal to k in the bucket n.
  // Return nullptr if no node is found.
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_find_before_node(size_type __n, const key_type& __k,
			__hash_code __code) const
    -> __node_base*
    {
      __node_base* __prev_p = _M_buckets[__n];
      if (!__prev_p)								// bucket不存在list
	return nullptr;

      for (__node_type* __p = static_cast<__node_type*>(__prev_p->_M_nxt);;
	   __p = __p->_M_next())
	{
	  if (this->_M_equals(__k, __code, __p))			// key相等返回prev_p
	    return __prev_p;

	  if (!__p->_M_nxt || _M_bucket_index(__p->_M_next()) != __n)		// 判断是否到达last node
	    break;
	  __prev_p = __p;												    // 更新__prev_p
	}
      return nullptr;
    }
```
#### find
find用于查找key所对应的节点，
```cpp
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    find(const key_type& __k)
    -> iterator
    {
      __hash_code __code = this->_M_hash_code(__k);             // 获取到key所对应的hash code
      std::size_t __n = _M_bucket_index(__k, __code);           // 获取key所对应bucket index
      __node_type* __p = _M_find_node(__n, __k, __code);        // 在此list下查找node
      return __p ? iterator(__p) : end();
    }

      __node_type*
      _M_find_node(size_type __bkt, const key_type& __key,
		   __hash_code __c) const
      {
	__node_base* __before_n = _M_find_before_node(__bkt, __key, __c);   // 查找相等节点的前一个节点，复用接口
	if (__before_n)
	  return static_cast<__node_type*>(__before_n->_M_nxt);             // 前一个节点存在，返回的为相等节点，即__before_n->next
	return nullptr;
      }
```

#### erase
erase会根据hashtable的是否包含重复key划分为两个函数，当其包含重复key时，__unique_key()返回false_type，否则返回true_type。这样可以在运行时决定调用哪个函数。
```cpp
    size_type
    erase(const key_type& __k)
    { return _M_erase(__unique_keys(), __k); }
```
`_M_erase(std::true_type, const key_type& __k)`删除唯一key的hashtable 元素

```cpp
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_erase(std::true_type, const key_type& __k)
    -> size_type
    {
      __hash_code __code = this->_M_hash_code(__k);                             // 计算__k 的hash_code
      std::size_t __bkt = _M_bucket_index(__k, __code);                         // 计算bucket index

      // Look for the node before the first matching node.
      __node_base* __prev_n = _M_find_before_node(__bkt, __k, __code);          // 获取前一个节点
      if (!__prev_n)
	return 0;

      // We found a matching node, erase it.
      __node_type* __n = static_cast<__node_type*>(__prev_n->_M_nxt);           // 找到了相应的节点
      _M_erase(__bkt, __prev_n, __n);                                           // 删除此节点
      return 1;
    }
```
`_M_erase(__bkt, __prev_n, __n)`调用并不是简单的list节点删除方法

对于节点删除函数，其存在如下需要注意的问题

* 删除last node节点，此时我们需要修改next bucket的previous begin
* 删除节点时致使整个list清空，此时我们需要将previous bucket 与 next bucket 进行链接
* 清空的list中存在_M_before_begin，此时需要让_M_before_begin成为next bucket的before begin
```cpp
      template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_erase(size_type __bkt, __node_base* __prev_n, __node_type* __n)
    -> iterator
    {
      if (__prev_n == _M_buckets[__bkt])                                        // 删除的节点为首节点
	_M_remove_bucket_begin(__bkt, __n->_M_next(),                                 // 摘除(此函数并不执行析构)first node 到 __n 之间的所有的节点，这样的删除方式可能会导致整个list清空
	   __n->_M_nxt ? _M_bucket_index(__n->_M_next()) : 0);                        
      else if (__n->_M_nxt)                                                     // __n->_M_nxt存在表示其之后仍存在元素
	{
	  size_type __next_bkt = _M_bucket_index(__n->_M_next());
	  if (__next_bkt != __bkt)                                                    // __n为bucket最后一个节点，__n将会是next bucket的previous begin
	    _M_buckets[__next_bkt] = __prev_n;                                        // 修改next bucket的previous begin
	}

      __prev_n->_M_nxt = __n->_M_nxt;                                           // 进行挂载
      iterator __result(__n->_M_next());                                        // 
      this->_M_deallocate_node(__n);                                            // 释放此节点
      --_M_element_count;

      return __result;
    }


  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    void
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_remove_bucket_begin(size_type __bkt, __node_type* __next,			// 此函数用于摘除[first node，__next)之间所有的节点之前所需做的处理
			   size_type __next_bkt)
    {
      if (!__next || __next_bkt != __bkt)					// 意味着current list已经被全部清空
	{
	  // Bucket is now empty
	  // First update next bucket if any
	  if (__next)
	    _M_buckets[__next_bkt] = _M_buckets[__bkt];			// next bucket的previous begin 指定为当前的bucket的previous begin

	  // Second update before begin node if necessary
	  if (&_M_before_begin == _M_buckets[__bkt])			// 如果当前的bucket拥有_M_before_begin，重新设置_M_before_begin
	    _M_before_begin._M_nxt = __next;				   
	  _M_buckets[__bkt] = nullptr;
	}
    }
```
`_M_erase(std::false_type, const key_type& __k)`用于存在重复key的hashtable的删除。
```cpp
  // erase hashtable that has multple key
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    auto
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_erase(std::false_type, const key_type& __k)
    -> size_type
    {
      __hash_code __code = this->_M_hash_code(__k);
      std::size_t __bkt = _M_bucket_index(__k, __code);

      // Look for the node before the first matching node.
      __node_base* __prev_n = _M_find_before_node(__bkt, __k, __code);			// 查找相等key的previous node
      if (!__prev_n)
	return 0;

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 526. Is it undefined if a function in the standard changes
      // in parameters?
      // We use one loop to find all matching nodes and another to deallocate
      // them so that the key stays valid during the first loop. It might be
      // invalidated indirectly when destroying nodes.
      __node_type* __n = static_cast<__node_type*>(__prev_n->_M_nxt);
      __node_type* __n_last = __n;                      // 用于标记最后一个相等的node
      std::size_t __n_last_bkt = __bkt;
      do
	{
	  __n_last = __n_last->_M_next();                     
	  if (!__n_last)								    // 达到末尾
	    break;
	  __n_last_bkt = _M_bucket_index(__n_last);			
	}
      while (__n_last_bkt == __bkt && this->_M_equals(__k, __code, __n_last));   // __n_last_bkt == __bkt表示__n_last 还位于此bucket， _M_equals == true 表示__n_last 与 __k相等
// 删除中默认相等元素会相连，如果不相连会导致删除操作异常
      // Deallocate nodes.
      size_type __result = 0;
      do
	{
	  __node_type* __p = __n->_M_next();
	  this->_M_deallocate_node(__n);          // 先释放节点
	  __n = __p;
	  ++__result;
	  --_M_element_count;
	}
      while (__n != __n_last);

      if (__prev_n == _M_buckets[__bkt])    //  删除头部连续节点需要特殊处理，因为此时存在全部删除的可能
	_M_remove_bucket_begin(__bkt, __n_last, __n_last_bkt);    // 
      else if (__n_last && __n_last_bkt != __bkt)  // 删除了last node
	_M_buckets[__n_last_bkt] = __prev_n;      
      __prev_n->_M_nxt = __n_last;
      return __result;
    }
```

#### rehash
当负载过大时我们需要进行rehash操作来扩充bucket的大小，其只要分为两个步骤
1. 重新分配bucket数组，对其进行扩充
2. 将old_bucket中的节点移动到new_bucket当中

rehash同样也需要考虑unique key 和 multiple key的区别，其调用如下
```cpp
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    void
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_rehash(size_type __n, const __rehash_state& __state)
    {
      __try
	{
	  _M_rehash_aux(__n, __unique_keys());
	}
      __catch(...)
	{
	  // A failure here means that buckets allocation failed.  We only
	  // have to restore hash policy previous state.
	  _M_rehash_policy._M_reset(__state);
	  __throw_exception_again;
	}
    }
```
对于unique key，其调用如下函数，可以看到其处理过程与unique insert过程相似
```cpp

  // Rehash when there is no equivalent elements.
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    void
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_rehash_aux(size_type __n, std::true_type)
    {
      __bucket_type* __new_buckets = _M_allocate_buckets(__n);           // 分配新的bucket，其大小为__n
      __node_type* __p = _M_begin();                                     // 返回第一个元素所对应的迭代器
      _M_before_begin._M_nxt = nullptr;                     
      std::size_t __bbegin_bkt = 0;                                      
      while (__p)                                                        // 当前元素非空
	{
	  __node_type* __next = __p->_M_next();                                // 
	  std::size_t __bkt = __hash_code_base::_M_bucket_index(__p, __n);     // 获取__p所对应的_M_bucket_index
	  if (!__new_buckets[__bkt])                                           // 插入bucket为nullptr
	    {                                                                  
	      __p->_M_nxt = _M_before_begin._M_nxt;                            // 将_M_before_begin 移动到 __p之前
	      _M_before_begin._M_nxt = __p;                                    
	      __new_buckets[__bkt] = &_M_before_begin;                          
	      if (__p->_M_nxt)                                                 // __p->_M_nxt存在，意味着_M_before_begin是从一个已经存在的bucket处调用而来，所以此处需要修改原位置的bucket内容
		__new_buckets[__bbegin_bkt] = __p;                                   
	      __bbegin_bkt = __bkt;                                            // 
	    }
	  else                                                                // 插入位置 bucket 存在元素，直接头插即可
	    {
	      __p->_M_nxt = __new_buckets[__bkt]->_M_nxt; 
	      __new_buckets[__bkt]->_M_nxt = __p;
	    }
	  __p = __next;
	}

      _M_deallocate_buckets();
      _M_bucket_count = __n;                                           
      _M_buckets = __new_buckets;                                      // 设置新的bucket
    }
```
而对于multiple key的hash table，其保证rehash之后原先相等的一连串node其相对位置不发生改变。
```cpp
      // Rehash when there can be equivalent elements, preserve their relative
  // order.
  template<typename _Key, typename _Value,
	   typename _Alloc, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   typename _Traits>
    void
    _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, _Traits>::
    _M_rehash_aux(size_type __n, std::false_type)
    {
      __bucket_type* __new_buckets = _M_allocate_buckets(__n);          

      __node_type* __p = _M_begin();
      _M_before_begin._M_nxt = nullptr;
      std::size_t __bbegin_bkt = 0;
      std::size_t __prev_bkt = 0;
      __node_type* __prev_p = nullptr;
      bool __check_bucket = false;

      while (__p)
	{
	  __node_type* __next = __p->_M_next();     
	  std::size_t __bkt = __hash_code_base::_M_bucket_index(__p, __n);

	  if (__prev_p && __prev_bkt == __bkt)          // 前一个元素存在，且插入bucket与前一次插入时相同
	    {
	      // Previous insert was already in this bucket, we insert after
	      // the previously inserted one to preserve equivalent elements
	      // relative order.
        /*之前插入的节点也位于此bucket，我们将其插入到前一个节点之后来保存相等元素之间的相对位置*/
	      __p->_M_nxt = __prev_p->_M_nxt;
	      __prev_p->_M_nxt = __p;
        /* 当向一个node之后插入元素时，我们需要检查我们是否改变了bucket的最后一个节点 */
        /* 如果改变，我们需要更新next bucket的before begin。 */
        /* 我们在完成这一系列想等元素的移动之后再进行检查，这样便可以限制检查的次数 */
	      // Inserting after a node in a bucket require to check that we
	      // haven't change the bucket last node, in this case next
	      // bucket containing its before begin node must be updated. We
	      // schedule a check as soon as we move out of the sequence of
	      // equivalent nodes to limit the number of checks.
	      __check_bucket = true;
	    }
	  else
	    {
	      if (__check_bucket)     // 检测是否需要更新next_bicket 的 previous begin
		{
		  // Check if we shall update the next bucket because of
		  // insertions into __prev_bkt bucket.
		  if (__prev_p->_M_nxt)         
		    {
		      std::size_t __next_bkt
			= __hash_code_base::_M_bucket_index(__prev_p->_M_next(),
							    __n);
		      if (__next_bkt != __prev_bkt)
			__new_buckets[__next_bkt] = __prev_p;       // 更新 next_buckt 的 previous begin
		    }
		  __check_bucket = false;
		}

	      if (!__new_buckets[__bkt])                // 插入位置为空
		{
		  __p->_M_nxt = _M_before_begin._M_nxt;
		  _M_before_begin._M_nxt = __p;
		  __new_buckets[__bkt] = &_M_before_begin;
		  if (__p->_M_nxt)
		    __new_buckets[__bbegin_bkt] = __p;
		  __bbegin_bkt = __bkt;
		}
	      else                                      // 插入位置不为空
		{
		  __p->_M_nxt = __new_buckets[__bkt]->_M_nxt;
		  __new_buckets[__bkt]->_M_nxt = __p;
		}
	    }
	  __prev_p = __p;
	  __prev_bkt = __bkt;
	  __p = __next;
	}

      if (__check_bucket && __prev_p->_M_nxt)     // 最后再次检查 __check_bucket
	{
	  std::size_t __next_bkt
	    = __hash_code_base::_M_bucket_index(__prev_p->_M_next(), __n);
	  if (__next_bkt != __prev_bkt)
	    __new_buckets[__next_bkt] = __prev_p;
	}

      _M_deallocate_buckets();                    // 释放buckets
      _M_bucket_count = __n;
      _M_buckets = __new_buckets;
    }
```
之前的代码在进行rehash时会牵扯到`_RehashPolicy		_M_rehash_policy`，此对象用于控制rehash行为。其为一个模板对象，在定义模板时传入，我们可以通过传入自己实现的rehash policy来自定义rehash行为。STL中有预定义的rehash policy，即`__detail::_Prime_rehash_policy`。
_Prime_rehash_policy定义如下，可以看到默认负载因子为1
```cpp
  /// Default value for rehash policy.  Bucket size is (usually) the
  /// smallest prime that keeps the load factor small enough.
  struct _Prime_rehash_policy
  {
    using __has_load_factor = std::true_type;

    _Prime_rehash_policy(float __z = 1.0) noexcept			// 构造函数，默认load_factor == 1
    : _M_max_load_factor(__z), _M_next_resize(0) { }

    float
    max_load_factor() const noexcept						
    { return _M_max_load_factor; }

    // Return a bucket size no smaller than n.           返回一个不小于__n的bucket大小
    std::size_t
    _M_next_bkt(std::size_t __n) const;

    // Return a bucket count appropriate for n elements   返回一个对于__n适当的bucket大小
    std::size_t
    _M_bkt_for_elements(std::size_t __n) const
    { return __builtin_ceil(__n / (long double)_M_max_load_factor); }   // 其返回的bucket将导致插入后直接进入满负载状态

    // __n_bkt is current bucket count, __n_elt is current element count,
    // and __n_ins is number of elements to be inserted.  Do we need to
    // increase bucket count?  If so, return make_pair(true, n), where n
    // is the new bucket count.  If not, return make_pair(false, 0).
    /* __n_bkt指代当前bucket数量， __n_elt指当前元素数量，__n值将要被插入的元素数量。
	   我们是否需要增大bucket数量？如果需要返回make_pair(true, n)，其中n表示新的bucket数量，
	   否则，返回make_pair(false, 0)
	*/
    std::pair<bool, std::size_t>
    _M_need_rehash(std::size_t __n_bkt, std::size_t __n_elt,
		   std::size_t __n_ins) const;

    typedef std::size_t _State;

    _State
    _M_state() const
    { return _M_next_resize; }

    void
    _M_reset() noexcept
    { _M_next_resize = 0; }

    void
    _M_reset(_State __state)
    { _M_next_resize = __state; }

    static const std::size_t _S_growth_factor = 2;			// 定义了增长因子

    float		_M_max_load_factor;
    mutable std::size_t	_M_next_resize;
  };
```
` _M_next_bkt(std::size_t __n) ` 定义如下
```cpp
    // Return a bucket size no smaller than n (as long as n is not above the
    // highest power of 2).
    std::size_t
    _M_next_bkt(std::size_t __n) noexcept
    {
      const auto __max_width = std::min<size_t>(sizeof(size_t), 8);
      const auto __max_bkt = size_t(1) << (__max_width * __CHAR_BIT__ - 1);
      std::size_t __res = __clp2(__n);

      if (__res == __n)
	__res <<= 1;

      if (__res == 0)
	__res = __max_bkt;

      if (__res == __max_bkt)
	// Set next resize to the max value so that we never try to rehash again
	// as we already reach the biggest possible bucket number.
	// Note that it might result in max_load_factor not being respected.
	_M_next_resize = std::size_t(-1);
      else
	_M_next_resize
	  = __builtin_ceil(__res * (long double)_M_max_load_factor);

      return __res;
    }
```
`_M_need_rehash(std::size_t __n_bkt, std::size_t __n_elt, std::size_t __n_ins)`函数定义如下

```cpp
  // Finds the smallest prime p such that alpha p > __n_elt + __n_ins.
  // If p > __n_bkt, return make_pair(true, p); otherwise return
  // make_pair(false, 0).  In principle this isn't very different from 
  // _M_bkt_for_elements.

  // The only tricky part is that we're caching the element count at
  // which we need to rehash, so we don't have to do a floating-point
  // multiply for every insertion.

  /*
  * 查找最小的p使得 p > __n_elt + __n_ins
  * 如果p > __n_bkt，返回make_pair(true, p)，否则返回make_pair(false, 0).
  * 原则上其与_M_bkt_for_elements相差不大
  * 
  * 唯一困难的一部分是我们缓存了需要重新计算的元素个数，所以我们在插入时不必
  * 每次都进行一次浮点数乘法运算
  */
  inline std::pair<bool, std::size_t>
  _Prime_rehash_policy::
  _M_need_rehash(std::size_t __n_bkt, std::size_t __n_elt,
		 std::size_t __n_ins) const
  {
    if (__n_elt + __n_ins > _M_next_resize)     // 我们缓存的_M_next_resize大小
      {
	float __min_bkts = ((float(__n_ins) + float(__n_elt))
			    / _M_max_load_factor);                // 获取最小的bucket数量
	if (__min_bkts > __n_bkt)                     // 最小需求大于当前bucket数量
	  {
	    __min_bkts = std::max(__min_bkts, _M_growth_factor * __n_bkt);    // 增长方式按照_M_growth_factor的指数倍进行增长，默认为2
	    return std::make_pair(true,
				  _M_next_bkt(__builtin_ceil(__min_bkts)));
	  }
	else        
	  {
	    _M_next_resize = static_cast<std::size_t>
	      (__builtin_ceil(__n_bkt * _M_max_load_factor));       // 缓存_M_next_resize
	    return std::make_pair(false, 0);
	  }
      }
    else
      return std::make_pair(false, 0);
  }
```

## 总结

hashlist个人感觉像按照其hash值进行折叠了的list，这种结构的引入优势在于简化了迭代器的设计，其迭代器的实现非常简单，并且迭代器的next的查找非常高效，可以很好地与当前的algorithm进行整合。
但是也带了其他问题
* last node判断复杂 >>>> 为了判断此节点是否为last node，我们需要计算next节点的bucket index，判断其是否与当前bucket一致。如果不缓存hashcode，将导致每次都重新计算hashcode，此时其计算性能将严重降低。
* last node删除>>>>last node不光作用于当前list，其还为next bucket的previous begin，我们还需要进行额外的处理工作。