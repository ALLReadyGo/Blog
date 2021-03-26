---
title: forward_list源码分析
categories: CPP
tags: 
 - CPP
---



forward_list是一个单链表的实现，相较于list，其有更高的空间存储效率，但是其迭代器只能进行单向移动。
其构造如图所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200818172645740.png#pic_center)

与list进行比较可以发现，其头部节点不再存储任何数据并且结构体中也没有开辟空间来存储forward_list的大小，所以其并没有实现size()这个成员函数，即我们无法在O(1)的时间内获取forward_list大小。
其尾部节点并没有指向头部节点，其指向的为nullptr，这样做能有效保证其迭代器的单向性。
## _Fwd_list_node
_Fwd_list_node继承自_Fwd_list_node_base，其包含元素与继承关系如下，其中其内部元素_M_storage是用于存储目标元素_Tp的内存空间
```cpp
  template<typename _Tp>
    struct _Fwd_list_node
    : public _Fwd_list_node_base
    {
      __gnu_cxx::__aligned_buffer<_Tp> _M_storage;
    };
```
_Fwd_list_node_base中存储是单链表的结构信息next
```cpp
  struct _Fwd_list_node_base
  {
    _Fwd_list_node_base() = default;

    _Fwd_list_node_base* _M_next = nullptr;
   }
```
## _Fwd_list_iterator
其包含的数据成员只有一个_Fwd_list_node_base指针，用于指向具体的节点
```cpp
template<typename _Tp>
    struct _Fwd_list_iterator
    {
    _Fwd_list_node_base* _M_node;
    }
```
其重载了*, ->,++(int), ++()之类的操作符。在->运算符定义中，其会进行强制类型转化来获取其相应的子类，
```cpp

      reference
      operator*() const noexcept
      { return *static_cast<_Node*>(this->_M_node)->_M_valptr(); }

      pointer
      operator->() const noexcept
      { return static_cast<_Node*>(this->_M_node)->_M_valptr(); }

      _Self&
      operator++() noexcept
      {
        _M_node = _M_node->_M_next;
        return *this;
      }

      _Self
      operator++(int) noexcept
      {
        _Self __tmp(*this);
        _M_node = _M_node->_M_next;
        return __tmp;
      }
```
## container
forward_list的头部节点`_M_head`为`_Fwd_list_node_base _M_head`，而不是`_Fwd_list_node<size_t>`，所以其头部不再存储任何数据，只是起到单纯的链接作用。
```cpp
_Fwd_list_node_base _M_head;
```
并且其尾部节点的next为nullptr，查看end()函数实现，其就是返回一个指向nullptr的迭代器。
```cpp
      const_iterator
      end() const noexcept
      { return const_iterator(0); }

	explicit
      _Fwd_list_iterator(_Fwd_list_node_base* __n) noexcept
      : _M_node(__n) { }
```
#### constructor
defult constructor函数调用栈如下，可以看到其结果是头节点中_M_next指向nullptr。其中头节点是forward_list的成员变量，所以会隐式构造，不用调用new之类的操作。
```cpp
      forward_list()
      noexcept(is_nothrow_default_constructible<_Node_alloc_type>::value)
      : _Base()
      { }
	
	 _Fwd_list_base()
      : _M_impl() { }
	
	_Fwd_list_impl()
        : _Node_alloc_type(), _M_head()
        { }
	
      struct _Fwd_list_node_base
      {
         _Fwd_list_node_base() = default;
         _Fwd_list_node_base* _M_next = nullptr;
      }
```
range constructor`template< class InputIt > forward_list( InputIt first, InputIt last, const Allocator& alloc = Allocator() )`，用于将两个迭代器之间的数据全部导入forward_list，其实现如下
```cpp
template<typename _InputIterator,
	       typename = std::_RequireInputIter<_InputIterator>>
        forward_list(_InputIterator __first, _InputIterator __last,
                     const _Alloc& __al = _Alloc())
	: _Base(_Node_alloc_type(__al))
        { _M_range_initialize(__first, __last); }

  template<typename _Tp, typename _Alloc>
    template<typename _InputIterator>
      void
      forward_list<_Tp, _Alloc>::
      _M_range_initialize(_InputIterator __first, _InputIterator __last)
      {
        _Node_base* __to = &this->_M_impl._M_head;			// 获取头部节点
        for (; __first != __last; ++__first)
          {
            __to->_M_next = this->_M_create_node(*__first);   // 构造新节点，并将其插入到__to节点之后
            __to = __to->_M_next;							  // 移动__to节点
          }
      }
```
#### insert_after
由于迭代器只能找到其后面的元素，所以forward_list不能实现插入到插入节点之前这一操作，所以其并没有定义insert方法，而是定义了insert_after这一操作。
```cpp
      iterator
      insert_after(const_iterator __pos, const _Tp& __val)
      { return iterator(this->_M_insert_after(__pos, __val)); }   // _M_insert_after实现真正的插入操作

  template<typename _Tp, typename _Alloc>
    template<typename... _Args>
      _Fwd_list_node_base*
      _Fwd_list_base<_Tp, _Alloc>::
      _M_insert_after(const_iterator __pos, _Args&&... __args)   
      {
        _Fwd_list_node_base* __to
	  = const_cast<_Fwd_list_node_base*>(__pos._M_node);		        // 直接获取迭代器中的指针
	_Node* __thing = _M_create_node(std::forward<_Args>(__args)...);	// 构造新节点
        __thing->_M_next = __to->_M_next;								// 对其进行链接
        __to->_M_next = __thing;						
        return __to->_M_next;
      }


        template<typename... _Args>
        _Node*
        _M_create_node(_Args&&... __args)
        {
          _Node* __node = this->_M_get_node();							// 获取节点所需的内存空间
          __try
            {
	      ::new ((void*)__node) _Node;
	      _Node_alloc_traits::construct(_M_get_Node_allocator(),		// 在其内存空间对其进行构造
					    __node->_M_valptr(),
					    std::forward<_Args>(__args)...);
            }
          __catch(...)
            {
              this->_M_put_node(__node);								
              __throw_exception_again;
            }
          return __node;
        }
```
由于是后插，所以如果想要插入位置为第一个元素，将无法仅利用begin()来完成，其需要借助before_begin()才能实现。
begin返回的为头节点的next节点，也就是第一个元素的节点

```cpp
      const_iterator
      begin() const noexcept
      { return const_iterator(this->_M_impl._M_head._M_next); }
```
before_begin()返回的为头部节点
```cpp
      const_iterator
      before_begin() const noexcept
      { return const_iterator(&this->_M_impl._M_head); }
```
#### erase
```cpp
      iterator
      erase_after(const_iterator __pos)
      { return iterator(this->_M_erase_after(const_cast<_Node_base*>
					     (__pos._M_node))); }
	
	
	  template<typename _Tp, typename _Alloc>
    _Fwd_list_node_base*
    _Fwd_list_base<_Tp, _Alloc>::
    _M_erase_after(_Fwd_list_node_base* __pos)
    {
      _Node* __curr = static_cast<_Node*>(__pos->_M_next);		// __curr = __pos->next
      __pos->_M_next = __curr->_M_next;							// 链接后面的节点
      _Node_alloc_traits::destroy(_M_get_Node_allocator(),		// 对__curr中保存的元素调用析构函数
				  __curr->_M_valptr());
      __curr->~_Node();											// 调用node的析构函数，不过这个没执行任何操作
      _M_put_node(__curr);										// 回收__curr节点
      return __pos->_M_next;									
    }
```
## 总结
* 由于forward_list迭代器的单向性，所以其只能执行insert_after，erase_after的操作，并且需要引入before_begin()这种特殊迭代器。
* forward_list不同于list的两点是。1、头节点不存储任何元素，所以size()操作不支持。2、forward_list不是循环链表，其end()指向nullptr。
* forward_list优势于简单，其实现了一个基本的单链表，节点有效密度相较于list节省了一个prev指针的存储空间。