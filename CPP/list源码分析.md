---
title: list源码分析
categories: CPP
tags:
 - CPP
---



list是CPP对双向链表的实现，其结构相对于传统的链表结构并无太大差异。CPP的实现将第一个节点设置为头节点，其存储的元素类型为size_t，用于存储list链表中数据元素的个数。其后的节点才是真正存储数据的节点。并且其为快速定位头部尾部且同一插入删除操作，其定义为循环链表。
其结构示意图如下
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200818150907602.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzM3NjU0NzA0,size_16,color_FFFFFF,t_70#pic_center)


## _List_node
_List_node是链表中节点的定义，其包含三个元素，分别是传统previous指针、next指针、存储元素_Tp。其中previous指针、next指针是基类_list_node_base中的元素。通过这一继承操作，我们便可以通过_List_node_base指针指向存储任意类型的_LIst_node实体。
```cpp
   struct _List_node_base {
      _List_node_base* _M_next;
      _List_node_base* _M_prev;
   }
```
_list_node继承自__List_node_base
```cpp
 template<typename _Tp>
    struct _List_node : public __detail::_List_node_base {
		_Tp _M_data;
    }
```
## _List_iterator
 _List_iterator中仅包含一个_List_node的指针，其重载了++，--，*，->之类的操作符来达使其能够像操作连续存储空间的指针一样操作链表指针。
 其中_List_node_base 为_list_node的基类，指向链表节点。
 ```cpp
 __detail::_List_node_base* _M_node;
 ```
其重载的部分实现如下，在operator*操作中，其会对元素进行强制类型转化，目标类型为_List_node<_Tp>，即向子类进行转化。
```cpp
// typedef _List_node<_Tp>			_Node;
      reference
      operator*() const _GLIBCXX_NOEXCEPT
      { return *static_cast<_Node*>(_M_node)->_M_valptr(); }

      pointer
      operator->() const _GLIBCXX_NOEXCEPT
      { return static_cast<_Node*>(_M_node)->_M_valptr(); }

      _Self&
      operator++() _GLIBCXX_NOEXCEPT
      {
	_M_node = _M_node->_M_next;
	return *this;
      }

      _Self
      operator++(int) _GLIBCXX_NOEXCEPT
      {
	_Self __tmp = *this;
	_M_node = _M_node->_M_next;
	return __tmp;
      }
```
## container
list在最开始构造时会构造一个头节点，其存储的数据为链表中的元素个数，类型为size_t，所以严格意义上说链表中的所有节点并不为同一类型。
container中仅包含一个元素，便是头节点
```cpp
_List_node<size_t> _M_node;
```
#### construct
default construct会构建一个空链表，其函数调用栈如下：
```cpp
      list()
      noexcept(is_nothrow_default_constructible<_Node_alloc_type>::value)
      : _Base() { }

      _List_base()						// _List_base中包含成员变量 _M_impl
      : _M_impl()
      { _M_init(); }

	_List_impl() _GLIBCXX_NOEXCEPT		// _List_impl包含头节点_M_node， _Node_alloc_type是内存分配器，其为了提供独立的内存分配策略
	: _Node_alloc_type(), _M_node()
	{ }
	
	void
      _M_init() _GLIBCXX_NOEXCEPT								// 头节点的初始化方式
      {
	this->_M_impl._M_node._M_next = &this->_M_impl._M_node;		// next 指向自身
	this->_M_impl._M_node._M_prev = &this->_M_impl._M_node;		// prev 指向自身
	_M_set_size(0);												// 设置头节点中的值为0，表示内部没有元素
      }
```
对其`template< class InputIt > list( InputIt first, InputIt last, const Allocator& alloc = Allocator() )`进行分析，其构造过程等同于进行[__first, __last)的连续插入操作，并不会做任何优化。而在连续内存空间的分配中会先计算插入数量，然后依据此数量分配内存，这样相较于多次push_back便节省了多次内存分配的过程，所以起到了一定的加速作用。并且这种构造方式存在部分构造的可能，不同于vector的range construct，在vector中一旦存在某一元素发生错误，便会对已经构造完成的全部对其析构。而list则只是析构发生错误的点，先前的节点会被保存下来。
```cpp
    template<typename _InputIterator,
	       typename = std::_RequireInputIter<_InputIterator>>
	list(_InputIterator __first, _InputIterator __last,
	     const allocator_type& __a = allocator_type())
	: _Base(_Node_alloc_type(__a))
	{ _M_initialize_dispatch(__first, __last, __false_type()); }

	      template<typename _InputIterator>
	void
	_M_initialize_dispatch(_InputIterator __first, _InputIterator __last,
			       __false_type)
	{
	  for (; __first != __last; ++__first)							// 不断进行尾部插入
	    emplace_back(*__first);										// 使用*first获取其值，调用emplace_back进行构造
	}
	
		emplace_back(_Args&&... __args)
	{
	  this->_M_insert(end(), std::forward<_Args>(__args)...);		// 实质上就是在尾部进行插入操作
	  return back();
	}
```
#### insert
对于`iterator insert(const_iterator __position, const value_type& __x);`，其调用栈如下
```cpp
iterator
      insert(const_iterator __position, value_type&& __x)
      { return emplace(__position, std::move(__x)); }

  template<typename _Tp, typename _Alloc>
    template<typename... _Args>
      typename list<_Tp, _Alloc>::iterator
      list<_Tp, _Alloc>::
      emplace(const_iterator __position, _Args&&... __args)
      {
	_Node* __tmp = _M_create_node(std::forward<_Args>(__args)...);		// 创建一个完成构造的节点
	__tmp->_M_hook(__position._M_const_cast()._M_node);					// 将此节点挂载到__position的前面
	this->_M_inc_size(1);												// 将链表的长度增加1
	return iterator(__tmp);
      }

     template<typename... _Args>
	_Node*
	_M_create_node(_Args&&... __args)
	{
	  auto __p = this->_M_get_node();									// 通过内存分配器分配一个node大小的内存
	  auto& __alloc = _M_get_Node_allocator();						
	  __allocated_ptr<_Node_alloc_type> __guard{__alloc, __p};			
	  _Node_alloc_traits::construct(__alloc, __p->_M_valptr(),			// 在此内存直接构造目标元素
					std::forward<_Args>(__args)...);
	  __guard = nullptr;
	  return __p;
	}
```
其中_M_inc_size查看源码如下，其将_M_node中的值+1。
```cpp
void _M_inc_size(size_t __n) { *_M_impl._M_node._M_valptr() += __n; }
```
#### erase
对于`iterator erase( iterator pos )`，其函数实现如下
```cpp
	  erase(iterator __position)
       { return _M_erase(__position); }

      // Erases element at position given.
      void
      _M_erase(iterator __position) _GLIBCXX_NOEXCEPT
      {
	this->_M_dec_size(1);										// size - 1
	__position._M_node->_M_unhook();							// 将此节点从链表中卸下
	_Node* __n = static_cast<_Node*>(__position._M_node);		
	_Node_alloc_traits::destroy(_M_get_Node_allocator(), __n->_M_valptr());		// 对节点内的元素调用析构函数
	_M_put_node(__n);											// 回收节点的内存空间
      }

      void
      _M_put_node(typename _Node_alloc_traits::pointer __p) _GLIBCXX_NOEXCEPT
      { _Node_alloc_traits::deallocate(_M_impl, __p, 1); }		// 调用分配器_M_impl对内存进行回收
```
而`iterator erase( const_iterator first, const_iterator last )`的函数实现如下，其只是重复调用`iterator erase( iterator pos )`而已
```cpp
      iterator
      erase(const_iterator __first, const_iterator __last) noexcept
      {
	while (__first != __last)
	  __first = erase(__first);
	return __last._M_const_cast();
      }
```
#### destruct
析构时情况如下，list析构并不做任何处理，而真正的析构处理全部交由_List_base去完成，其会将所有数据节点全部调用其析构函数并回收其分配的内存。这里由于是clear操作，所以对节点操作时也就没有unhook操作
```cpp
~list() = default;

~_List_base() _GLIBCXX_NOEXCEPT
      { _M_clear(); }

  template<typename _Tp, typename _Alloc>
    void
    _List_base<_Tp, _Alloc>::
    _M_clear() _GLIBCXX_NOEXCEPT
    {
      typedef _List_node<_Tp>  _Node;
      __detail::_List_node_base* __cur = _M_impl._M_node._M_next;		// 指向头节点的下一个节点
      while (__cur != &_M_impl._M_node)									// cur不为头节点，表明未到达尾部
	{
	  _Node* __tmp = static_cast<_Node*>(__cur);						
	  __cur = __tmp->_M_next;
	  _Tp* __val = __tmp->_M_valptr();						
	  _Node_alloc_traits::destroy(_M_get_Node_allocator(), __val);		// 调用析构函数
	  _M_put_node(__tmp);												// 回收节点所占用的空间
	}
    }
```
#### begin，end
list是一个双向循环链表，只有这样才能快速地找到头部尾部节点。
begin指向第一个元素的位置，即头部节点的next，如果list为空时，由于我们构建的是循环链表，他会指向头部节点自身。
```cpp
	  iterator
      begin() _GLIBCXX_NOEXCEPT
      { return iterator(this->_M_impl._M_node._M_next); }
```
end指向超尾，即最后一个元素的下一个元素，其便为头部节点（因为是循环链表）。
```cpp
      iterator
      end() _GLIBCXX_NOEXCEPT
      { return iterator(&this->_M_impl._M_node); }
```
## 总结
* list在range insert和range erase时都是重复调用single insert和single erase函数，所以在我们对其进行选择时，我们完全不必从性能的角度进行考虑，其只是提供了多样和同一的调用接口而已。
* 不同于我常见的list，其将size_t作为第一个节点，用于存储链表大小。其有如下好处：1、头部节点的引用统一了插入删除操作，让我们不必再每次检验某些特殊位置。2、充分利用了头部节点这个空缺位置。
* 其头部节点与尾部连接，构成循环链表。让我们可以节省一个尾部迭代器的存储空间，并且避免了插入删除时这个尾部迭代器的修改工作。