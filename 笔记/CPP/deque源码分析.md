---
title: deque源码分析
categories: CPP
tags:
 - CPP
---



deque在实现时使用一连串固定大小的数组和一个书签来实现，其结构图如下：![在这里插入图片描述](https://img-blog.csdnimg.cn/20200815165935150.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzM3NjU0NzA0,size_16,color_FFFFFF,t_70#pic_center)
假设此时我们存储的数据类型为T，那么Nodes数组中存储的类型便为T*，即T[]的位置。而T[]则是实际存储数据的内存位置。这种结构在empty()时，其Nodes下不会挂载任何数组，当我们需要插入元素时，会在Nodes的中间开始挂载而不是两端进行。这样便保证了deque可以进行两端的任意插入操作。插入操作时，会先判断当前数组是否还有剩余空间，如果当前数组还剩余空间则在数组空间内placement construct，否则我们需要申请一个新的数组空间并挂载，然后在新的数组空间进行插入操作。

## Deque::iterator
Deque的迭代器中存储了节点的位置信息，利用这些位置信息，迭代器可以找到其previous和next，并且Deque的迭代器是一个随机访问迭代器，所以他还能够在O(1)的时间内找到相对于此节点的任意一个节点。
其包含如下数据。
```cpp
// typedef _Tp*					    _Elt_pointer;
// typedef _Tp**					_Map_pointer;
      _Elt_pointer _M_cur;
      _Elt_pointer _M_first;
      _Elt_pointer _M_last;
      _Map_pointer _M_node;
```
`_M_node` 指向上图的某一数组在nodes中对应的元素位置，用于获取当前指向的数组的具体位置。`_M_first`指向数组中第一个元素，`_M_last`指向数组中最后一个元素，`_M_cur`指向当前的元素。
以iterator的移动操作为例进行分析。当需要对迭代器进行移动时，我们需要考虑两种情况。1、如果移动的距离很小，使得仅在数组内部进行移动时我们仅需要移动_M_cur。2、如果移动距离过大，那么我们还需要在数组间进行移动，这时我们需要计算目标数组的位置和新的_M_cur，并且重新设置`_M_node`，`_M_last`，`_M_first`，`_M_cur`
```cpp

_Self&
      operator+=(difference_type __n) _GLIBCXX_NOEXCEPT                 // __n可以为负值，表示向左进行偏移
      {
	const difference_type __offset = __n + (_M_cur - _M_first);       // 获取从_M_first 到目标位置的全部偏移量
	if (__offset >= 0 && __offset < difference_type(_S_buffer_size()))      // 目标位置位于同一个数组， _S_buffer_size()返回数组中包含的元素个数
	  _M_cur += __n;                                                        // 直接利用_M_cur进行偏移计算
	else
	  {
	    const difference_type __node_offset =                         // 此处用于获取nodes的偏移量，公式为   偏移量 / 数组中包含元素的个数
	      __offset > 0 ? __offset / difference_type(_S_buffer_size())  // offset为正数时使用此方式
			   : -difference_type((-__offset - 1)                 
					      / _S_buffer_size()) - 1;            // 为负数时使用此
	    _M_set_node(_M_node + __node_offset);                         // 移动_M_node，其偏移量为刚才计算的__node_offset
	    _M_cur = _M_first + (__offset - __node_offset           
				 * difference_type(_S_buffer_size()));          // 设置_M_cur, _M_first 和 _M_last 在 _M_set_node时已经被设置完成
	  }
	return *this;
      }

    void
      _M_set_node(_Map_pointer __new_node) _GLIBCXX_NOEXCEPT
      {
	_M_node = __new_node;
	_M_first = *__new_node;
	_M_last = _M_first + difference_type(_S_buffer_size());
      }
```
operator*()和operator->()证明_M_cur指向的就是实际的元素
```cpp
      reference
      operator*() const _GLIBCXX_NOEXCEPT
      { return *_M_cur; }

      pointer
      operator->() const _GLIBCXX_NOEXCEPT
      { return _M_cur; }
```
## container
Deque大量操作都依赖迭代器完成，其内部也创建了两个iterator用于指示当前已经存储元素的头和尾。内部维护的数据如下。
`_M_map`指向Nodes数组，也就是索引数组。`_M_map_size`存储索引数组的大小。`_M_start`存储队列的首部元素迭代器，`_M_finish`存储的为尾部元素迭代器。
```cpp
	_Map_pointer _M_map;
	size_t _M_map_size;
	iterator _M_start;
	iterator _M_finish;
```
#### constructor
default constructor调用栈如下，在_Deque_base()构造中调用了_M_initialize_map(0)，其完成了构造过程中的主要工作。
```cpp
	  deque() : _Base() { }

      _Deque_base()
      : _M_impl()
      { _M_initialize_map(0); }
	 
	  _Deque_impl()
	   : _Tp_alloc_type(), _M_map(), _M_map_size(0),
	   _M_start(), _M_finish()
	   { }	  
```
_M_initialize_map(__n)函数意义为在deque中分配__n个元素所需的内存空间。构造时会先初始化_M_map所指向的内存空间，在构造时存在最小构造量8。初始化数组索引之后，再去分配所需的数组空间，其分配原则为只分配所需大小，使用lazy模式。
```cpp
  template<typename _Tp, typename _Alloc>
    void
    _Deque_base<_Tp, _Alloc>::
    _M_initialize_map(size_t __num_elements)
    {
      const size_t __num_nodes = (__num_elements/ __deque_buf_size(sizeof(_Tp))     // __deque_buf_size()用于获取存储数据的数组大小
				  + 1);                                                     // __num_node表示共需构造多少数组才能完全存储__num_elements数据
      
      this->_M_impl._M_map_size = std::max((size_t) _S_initial_map_size,            // enum { _S_initial_map_size = 8 }; 此为常量 8
					   size_t(__num_nodes + 2));                          // _M_map数组大小至少为8，即存在最小构造量
      this->_M_impl._M_map = _M_allocate_map(this->_M_impl._M_map_size);            // 为_M_map分配存储空间

      // For "small" maps (needing less than _M_map_size nodes), allocation
      // starts in the middle elements and grows outwards.  So nstart may be
      // the beginning of _M_map, but for small maps it may be as far in as
      // _M_map+3.

      _Map_pointer __nstart = (this->_M_impl._M_map
			       + (this->_M_impl._M_map_size - __num_nodes) / 2);          // 此计算位置的结果为_M_map的中间位置 -  __num_nodes / 2
      _Map_pointer __nfinish = __nstart + __num_nodes;                              // _nfinish同理

      __try
	{ _M_create_nodes(__nstart, __nfinish); }                                     // 创建数据存储数组
      __catch(...)
	{
	  _M_deallocate_map(this->_M_impl._M_map, this->_M_impl._M_map_size);         // 发生异常，回收_M_map分配的内存
	  this->_M_impl._M_map = _Map_pointer();
	  this->_M_impl._M_map_size = 0;
	  __throw_exception_again;
	}

      this->_M_impl._M_start._M_set_node(__nstart);                                 // 设置start迭代器
      this->_M_impl._M_finish._M_set_node(__nfinish - 1);                           // 设置finish迭代器
      this->_M_impl._M_start._M_cur = _M_impl._M_start._M_first;                    // _M_start._M_cur 为第一个数组元素
      this->_M_impl._M_finish._M_cur = (this->_M_impl._M_finish._M_first            // _M_finish._M_cur 根据元素数获取
					+ __num_elements
					% __deque_buf_size(sizeof(_Tp)));
    }
```
__deque_buf_size用于获取数组中包含的元素个数，当数据类型T的大小较小时，其包含的元素个数为 512 / sizeof(T)，即int32_t返回的元素个数为128，如果sizeof(T) == 257，则返回1。如果sizeof(T) > 512，也会返回1。
_M_create_nodes为_M_map数组索引设置其所分配的数组空间，在其内部继续调用_M_allocate_node，此函数真正分配内存空间，其大小为 sizeof(T) * __deque_buf_size。
```cpp
#ifndef _GLIBCXX_DEQUE_BUF_SIZE
#define _GLIBCXX_DEQUE_BUF_SIZE 512
#endif

  _GLIBCXX_CONSTEXPR inline size_t
  __deque_buf_size(size_t __size)
  { return (__size < _GLIBCXX_DEQUE_BUF_SIZE
	    ? size_t(_GLIBCXX_DEQUE_BUF_SIZE / __size) : size_t(1)); }

  template<typename _Tp, typename _Alloc>
    void
    _Deque_base<_Tp, _Alloc>::
    _M_create_nodes(_Map_pointer __nstart, _Map_pointer __nfinish)
    {
      _Map_pointer __cur;
      __try
	{
	  for (__cur = __nstart; __cur < __nfinish; ++__cur)
	    *__cur = this->_M_allocate_node();
	}
      __catch(...)
	{
	  _M_destroy_nodes(__nstart, __cur);
	  __throw_exception_again;
	}
    }

      _Ptr
      _M_allocate_node()
      {
	typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Traits;
	return _Traits::allocate(_M_impl, __deque_buf_size(sizeof(_Tp)));
      }
```
`template< class InputIt > deque( InputIt first, InputIt last,  const Allocator& alloc = Allocator() )`的函数实现如下，其主要功能在函数_M_range_initialize()中实现。其主要完成了两个步骤：1、对节点内存的分配任务。2、在分配的内存中进行元素构造。
分析了源码之后，也更深地理解析构函数中不要引入异常的原因：在STL的源码中我们看到了其对于构造过程中异常的处理，但是他并没有对析构过程做任何异常处理。因为STL中要求class析构中不能产生错误。
```cpp
      template<typename _InputIterator,
	       typename = std::_RequireInputIter<_InputIterator>>
	deque(_InputIterator __first, _InputIterator __last,
	      const allocator_type& __a = allocator_type())
	: _Base(__a)
	{ _M_initialize_dispatch(__first, __last, __false_type()); }



      template<typename _InputIterator>
	void
	_M_initialize_dispatch(_InputIterator __first, _InputIterator __last,
			       __false_type)
	{
	  _M_range_initialize(__first, __last,
			      std::__iterator_category(__first));
	}

  template <typename _Tp, typename _Alloc>
    template <typename _ForwardIterator>
      void
      deque<_Tp, _Alloc>::
      _M_range_initialize(_ForwardIterator __first, _ForwardIterator __last,
                          std::forward_iterator_tag)
      {
        const size_type __n = std::distance(__first, __last);       // 此函数计算两个迭代器之间的距离
        this->_M_initialize_map(__n);                               // 初始化_M_map

        _Map_pointer __cur_node;
        __try
          {
            for (__cur_node = this->_M_impl._M_start._M_node;       // 对每个数组进行初始化操作
                 __cur_node < this->_M_impl._M_finish._M_node;
                 ++__cur_node)
	      {
		_ForwardIterator __mid = __first;
		std::advance(__mid, _S_buffer_size());                         // advance 执行 __mid += _S_buffer_size()
		std::__uninitialized_copy_a(__first, __mid, *__cur_node,       // __uninitialized_copy_a 初始化一片连续内存数据
					    _M_get_Tp_allocator());
		__first = __mid;
	      }
            std::__uninitialized_copy_a(__first, __last,          // 初始化最后一个数组元素
					this->_M_impl._M_finish._M_first,
					_M_get_Tp_allocator());
          }
        __catch(...)
          {
            std::_Destroy(this->_M_impl._M_start,                 // 如果发生异常，则析构已分配的元素
			  iterator(*__cur_node, __cur_node),
			  _M_get_Tp_allocator());
            __throw_exception_again;
          }
      }

    template<typename _ForwardIterator, typename _Allocator>
    void
    _Destroy(_ForwardIterator __first, _ForwardIterator __last,
	     _Allocator& __alloc)
    {
      typedef __gnu_cxx::__alloc_traits<_Allocator> __traits;
      for (; __first != __last; ++__first)
	__traits::destroy(__alloc, std::__addressof(*__first));       // 调用分配器所对用的析构函数
    }
```
#### insert
分析`iterator insert(const_iterator __position, const value_type& __x)`，当其向头部插入时直接调用push_front，尾部时直接调用push_back。当插入位置位于内部是则调用_M_insert_aux。
```cpp
  template <typename _Tp, typename _Alloc>
    typename deque<_Tp, _Alloc>::iterator
    deque<_Tp, _Alloc>::
    insert(const_iterator __position, const value_type& __x)
    {
      if (__position._M_cur == this->_M_impl._M_start._M_cur)
	{
	  push_front(__x);
	  return this->_M_impl._M_start;
	}
      else if (__position._M_cur == this->_M_impl._M_finish._M_cur)
	{
	  push_back(__x);
	  iterator __tmp = this->_M_impl._M_finish;
	  --__tmp;
	  return __tmp;
	}
      else
	return _M_insert_aux(__position._M_const_cast(), __x);
   }
```
_M_insert_aux在插入时会先计算插入位置，当插入位置靠左时，他会在左侧进行插入。而插入位置靠右时则进行右侧插入。这样便能在插入时达到移动最小元素数量的目的。
```cpp
  template<typename _Tp, typename _Alloc>
    template<typename... _Args>
      typename deque<_Tp, _Alloc>::iterator
      deque<_Tp, _Alloc>::
      _M_insert_aux(iterator __pos, _Args&&... __args)
      {
	value_type __x_copy(std::forward<_Args>(__args)...);          // XXX copy
	difference_type __index = __pos - this->_M_impl._M_start;     // 计算插入位置左侧有多少节点
	if (static_cast<size_type>(__index) < size() / 2)             // 左侧节点少，我们则将节点插入到左侧的位置
	  {
	    push_front(_GLIBCXX_MOVE(front()));                       // 调用push_front，此时会插入一个节点。此操作目的为解决潜在的内存分配问题
	    iterator __front1 = this->_M_impl._M_start;               
	    ++__front1;
	    iterator __front2 = __front1;
	    ++__front2;
	    __pos = this->_M_impl._M_start + __index;               
	    iterator __pos1 = __pos;
	    ++__pos1;
	    _GLIBCXX_MOVE3(__front2, __pos1, __front1);               // 调用移动函数，对迭代器中的元素进行移动 
	  }
	else                                                          // 右侧节点少，则将节点插入到右侧
	  {
	    push_back(_GLIBCXX_MOVE(back()));                         
	    iterator __back1 = this->_M_impl._M_finish;
	    --__back1;
	    iterator __back2 = __back1;
	    --__back2;
	    __pos = this->_M_impl._M_start + __index;
	    _GLIBCXX_MOVE_BACKWARD3(__pos, __back2, __back1);         
	  }
	*__pos = _GLIBCXX_MOVE(__x_copy);                             // 这里直接调用移动函数
	return __pos;
      }

```
#### push_back
push_back涉及到内存的分配问题和元素重排序问题。
1、插入位置其数组空间还有剩余空间则直接在其内部进行构造即可
2、如果其数组空间已经满了，我们还需要重新分配一个数组来存储新的元素，此时又将引入新的情况
++++1、如果数组索引空间还有剩余，即插入侧数组索引还能插入数组，那么我们插入新的数组即可
++++2、如果插入侧不存在足够空间，那么造成其不足的原因可能由两个。分别是偏重和单纯的空间不足。
```cpp
  // Called only if _M_impl._M_finish._M_cur == _M_impl._M_finish._M_last - 1.
  template<typename _Tp, typename _Alloc>
    template<typename... _Args>
      void
      deque<_Tp, _Alloc>::
      _M_push_back_aux(_Args&&... __args)
      {
	_M_reserve_map_at_back();                                                   // 判断map空间是否充足，如果不足则做相应处理（移位 或者 重新分配） 
	*(this->_M_impl._M_finish._M_node + 1) = this->_M_allocate_node();          // 插入一个数组空间
	__try
	  {
	    _Alloc_traits::construct(this->_M_impl,
	                             this->_M_impl._M_finish._M_cur,               // 在数组空间中构造目标
			             std::forward<_Args>(__args)...);

	    this->_M_impl._M_finish._M_set_node(this->_M_impl._M_finish._M_node
						+ 1);
	    this->_M_impl._M_finish._M_cur = this->_M_impl._M_finish._M_first;
	  }
	__catch(...)
	  {
	    _M_deallocate_node(*(this->_M_impl._M_finish._M_node + 1));
	    __throw_exception_again;
	  }
      }


      void
      _M_reserve_map_at_back(size_type __nodes_to_add = 1)
      {
	if (__nodes_to_add + 1 > this->_M_impl._M_map_size
	    - (this->_M_impl._M_finish._M_node - this->_M_impl._M_map))
	  _M_reallocate_map(__nodes_to_add, false);
      }


  template <typename _Tp, typename _Alloc>
    void
    deque<_Tp, _Alloc>::
    _M_reallocate_map(size_type __nodes_to_add, bool __add_at_front)
    {
      const size_type __old_num_nodes
	= this->_M_impl._M_finish._M_node - this->_M_impl._M_start._M_node + 1;       // old_node_num表示已经占有的nodes数量，并不是_M_map_size，可以避免由于偏重造成的错误分配
      const size_type __new_num_nodes = __old_num_nodes + __nodes_to_add;       // 

      _Map_pointer __new_nstart;
      if (this->_M_impl._M_map_size > 2 * __new_num_nodes)                      // 出现这种情况可能由如下操作造成：假设deque中插入数据已满，那么此时start和finish将在Map的两侧，
                                                                                // 如果此时我们只调用pop_front，将只移动start，全部删除时start将位于最右侧。此时调用push_back即使有空间也无法插入
	{
	  __new_nstart = this->_M_impl._M_map + (this->_M_impl._M_map_size            // 计算start应该位于的位置
					 - __new_num_nodes) / 2
	                 + (__add_at_front ? __nodes_to_add : 0);
	  if (__new_nstart < this->_M_impl._M_start._M_node)                         
	    std::copy(this->_M_impl._M_start._M_node,                                // 这里移动的为整个数组，所以其移动损耗并不算大
		      this->_M_impl._M_finish._M_node + 1,
		      __new_nstart);
	  else
	    std::copy_backward(this->_M_impl._M_start._M_node,
			       this->_M_impl._M_finish._M_node + 1,
			       __new_nstart + __old_num_nodes);
	}
      else
	{
	  size_type __new_map_size = this->_M_impl._M_map_size                        // 此时我们需要重新分配map的空间，map的空间分配采用eager模式
	                             + std::max(this->_M_impl._M_map_size,            // 会分配远超需求的空间大小
						__nodes_to_add) + 2;

	  _Map_pointer __new_map = this->_M_allocate_map(__new_map_size);         
	  __new_nstart = __new_map + (__new_map_size - __new_num_nodes) / 2          
	                 + (__add_at_front ? __nodes_to_add : 0);
	  std::copy(this->_M_impl._M_start._M_node,                                   // 移动原先数组索引中的数组
		    this->_M_impl._M_finish._M_node + 1,
		    __new_nstart);
	  _M_deallocate_map(this->_M_impl._M_map, this->_M_impl._M_map_size);

	  this->_M_impl._M_map = __new_map;
	  this->_M_impl._M_map_size = __new_map_size;
	}

      this->_M_impl._M_start._M_set_node(__new_nstart);
      this->_M_impl._M_finish._M_set_node(__new_nstart + __old_num_nodes - 1);
    }

```
#### pop_back
pop_back操作将涉及元素的删除，其会调用元素的析构函数和释放不需要的内存空间。当其删除元素位于当前数组时，直接在此空间析构其即可。如果位于下一个数组空间，则调用_M_pop_back_aux。其会移动到下一个数组的位置，并对其最后一个元素析构，此时还会释放前一个数组空间。
```cpp
  void
      pop_back() _GLIBCXX_NOEXCEPT
      {
	__glibcxx_requires_nonempty();
	if (this->_M_impl._M_finish._M_cur
	    != this->_M_impl._M_finish._M_first)
	  {
	    --this->_M_impl._M_finish._M_cur;
	    _Alloc_traits::destroy(this->_M_impl,
				   this->_M_impl._M_finish._M_cur);
	  }
	else
	  _M_pop_back_aux();
      }


  template <typename _Tp, typename _Alloc>
    void deque<_Tp, _Alloc>::
    _M_pop_back_aux()
    {
      _M_deallocate_node(this->_M_impl._M_finish._M_first);                  // 释放当前数组空间
      this->_M_impl._M_finish._M_set_node(this->_M_impl._M_finish._M_node - 1);    // 移动到下一个数组索引
      this->_M_impl._M_finish._M_cur = this->_M_impl._M_finish._M_last - 1;  
      _Alloc_traits::destroy(_M_get_Tp_allocator(),                 //对最后一个元素进行析构
			     this->_M_impl._M_finish._M_cur);             
    }

      
      void
      _M_deallocate_node(_Ptr __p) _GLIBCXX_NOEXCEPT    // 释放数组空间
      {
	typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Traits;
	_Traits::deallocate(_M_impl, __p, __deque_buf_size(sizeof(_Tp)));
      }
```

#### erase
erase操作过程如下，与插入相似其也会计算删除区间的位置距离哪里更近，如果靠左则调用头部删除这种方式进行元素删除。
```cpp
  template <typename _Tp, typename _Alloc>
    typename deque<_Tp, _Alloc>::iterator
    deque<_Tp, _Alloc>::
    _M_erase(iterator __first, iterator __last)
    {
      if (__first == __last)
	return __first;
      else if (__first == begin() && __last == end())
	{
	  clear();                                                              // erase全部元素时直接删_除即可
	  return end();
	}
      else
	{
	  const difference_type __n = __last - __first;
	  const difference_type __elems_before = __first - begin();
	  if (static_cast<size_type>(__elems_before) <= (size() - __n) / 2)
	    {
	      if (__first != begin())
		_GLIBCXX_MOVE_BACKWARD3(begin(), __first, __last);                    // 将元素向后移动
	      _M_erase_at_begin(begin() + __n);                                 // 释放头部不再释放的元素，其中会对这些元素调用析构函数
	    }
	  else
	    {
	      if (__last != end())
		_GLIBCXX_MOVE3(__last, end(), __first);
	      _M_erase_at_end(end() - __n);
	    }
	  return begin() + __elems_before;
	}
    }

          void
      _M_erase_at_begin(iterator __pos)
      {
	_M_destroy_data(begin(), __pos, _M_get_Tp_allocator());
	_M_destroy_nodes(this->_M_impl._M_start._M_node, __pos._M_node);
	this->_M_impl._M_start = __pos;
      }
```
这里的删除操作有一些特别以至于我最开始还以为是个Bug。其删除的核心是这两步
1、_GLIBCXX_MOVE_BACKWARD3(begin(), __first, __last);
就是简单的移动操作，其会调用如下函数，这里注意他会调用类的operator=，operator=不仅仅只有复制目标数据这一简单功能，其还需要释放现有资源，所以在这一步时__first至__last的资源完成的析构。
```cpp
      __copy_move_b(_BI1 __first, _BI1 __last, _BI2 __result)
        {
	  while (__first != __last)
	    *--__result = *--__last;
	  return __result;
	}	
```
2、_M_erase_at_begin(begin() + __n);
将所有元素移动到其目标位置之后，我们还需要删除多余的元素
因此需要对begin()，__pos 的数据调用析构函数
再回收分配的数组空间
```cpp
      // Called by erase(q1, q2).
      void
      _M_erase_at_begin(iterator __pos)
      {
	_M_destroy_data(begin(), __pos, _M_get_Tp_allocator());
	_M_destroy_nodes(this->_M_impl._M_start._M_node, __pos._M_node);
	this->_M_impl._M_start = __pos;
      }
```
## 总结
1、Deque使用二级数组的方式来实现队列，第一层为数组索引，第二层才是真正存储数据的数组。这种结构能很好的处理偏重问题。当我们需要重新排序数组位置时，其实只是简单改变数组索引所对应的数组，而不用对数组内的元素做任何修改，其性能损耗并不大。

2、与链表的两端插入性能分析：
deque的数组空间是以512 byte作为单位进行分配的，如果存储的目标元素过大，即超过256 byte，那么deque数组索引下的每个数组空间实质上只能存一个元素。其每次插入和删除时都会涉及到内存空间的申请和释放，所以其性能如果不考虑_M_map的问题将与list持平，考虑的情况下还不如使用链表。
但是如果其存储小元素，则可以极大减少数据在存访过程中的内存频繁申请释放所带来的性能损耗。
不过deque的插入也就只能限于两端操作，如果对于其内部节点进行插入，其还需要对目标位置到某一端的元素进行移动。而链表则可以在O(1)完成。

3、STL要求我们custom class 的 operator= 已经处理好数据的析构工作，否则将会造成严重的资源泄露问题，因为数据的析构有的是通过operator=进行的。