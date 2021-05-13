---
title: vector源码分析
categories: CPP
tags:
 - CPP
---



这里引用源码中的一段描述

> In some terminology a %vector can be described as a dynamic C-style array, it offers fast and efficient access to individual elements in any order and saves the user from worrying aboutmemory and size allocation. 

vector毫不夸张得说就是有动态内存分配功能的传统数组。vector中包含三个关键成员变量,
`_M_start`指向第一个元素的位置，同时也是连续内存的起始地址。`_M_finish`指向存储的最后一个元素的后一个位置。所以当`vector.empty() == true`时，`_M_start == _M_finish`。`_M_end_of_storage`存储的是连续内存内存地址的末尾地址的后一个位置。
```cpp
pointer _M_start;			
pointer _M_finish;
pointer _M_end_of_storage;
```
通过`begin(), end(), size(), empty(), capacity()`函数实现我们便能更加深刻得理解这三个指针的意义。
```cpp
const_iterator
begin() const _GLIBCXX_NOEXCEPT
{ return const_iterator(this->_M_impl._M_start); }

const_iterator
end() const _GLIBCXX_NOEXCEPT
{ return const_iterator(this->_M_impl._M_finish); }


size_type
size() const _GLIBCXX_NOEXCEPT
{ return size_type(this->_M_impl._M_finish - this->_M_impl._M_start); }


bool
empty() const _GLIBCXX_NOEXCEPT
{ return begin() == end(); }


size_type
capacity() const _GLIBCXX_NOEXCEPT
{ return size_type(this->_M_impl._M_end_of_storage
		 - this->_M_impl._M_start); }
```

vector的关键就是内存的动态分配和释放如何实现，其涉及内存分配的操作主要有construct、insert、push_back，operator=。

#### construct
对于构造函数`vector()`，其存在如下调用路径。其结果就是三个指针全部指向nullptr。
```cpp
vector()
: _Base() { }

_Vector_base()
: _M_impl() { }

_Vector_impl()
: _Tp_alloc_type(), _M_start(), _M_finish(), _M_end_of_storage()
{ }      
```


对于构造函数`vector( size_type count, const T& value, const Allocator& alloc = Allocator())`，此函数中内存分配的任务是在`_M_create_storage`中分配，其分配了__n大小的连续内存空间，并设置了三个指针的位置。而对于其地址的初始化任务则交由_M_fill_initialize完成。此函数对数组中每个空间调用placement new operator，来对其空间进行构造。
```cpp
    vector(size_type __n, const value_type& __value,
        const allocator_type& __a = allocator_type())
    : _Base(__n, __a)
    { _M_fill_initialize(__n, __value); }


    _Vector_base(size_t __n, const allocator_type& __a)
    : _M_impl(__a)
    { _M_create_storage(__n); }


    _M_create_storage(size_t __n)
      {
	this->_M_impl._M_start = this->_M_allocate(__n);
	this->_M_impl._M_finish = this->_M_impl._M_start;
	this->_M_impl._M_end_of_storage = this->_M_impl._M_start + __n;
      }
    };
```
_M_fill_initialize会调用如下函数，在这里我看可以看到placement new的调用，并且还能看到vector的异常安全性是如何保证的，其在调用构造函数出现异常时，会对之前已经构造好的数据调用`destruct`，并返回异常。
```cpp
  template<bool _TrivialValueType>
    struct __uninitialized_fill_n
    {
      template<typename _ForwardIterator, typename _Size, typename _Tp>
        static _ForwardIterator
        __uninit_fill_n(_ForwardIterator __first, _Size __n,
			const _Tp& __x)
        {
	  _ForwardIterator __cur = __first;
	  __try
	    {
	      for (; __n > 0; --__n, ++__cur)
		std::_Construct(std::__addressof(*__cur), __x);
	      return __cur;
	    }
	  __catch(...)
	    {
	      std::_Destroy(__first, __cur);
	      __throw_exception_again;
	    }
	}
    };
  
    template<typename _T1, typename... _Args>
    inline void
    _Construct(_T1* __p, _Args&&... __args)
    { ::new(static_cast<void*>(__p)) _T1(std::forward<_Args>(__args)...); }
```
#### push_back
此操作在插入时存在两种状态，第一种为存在剩余空间，即`_M_finish != _M_end_of_storage`，此时可以直接在此地址空间构造。第二种则是剩余空间不足，此时需要重新分配内存空间。
```cpp
      void
      push_back(const value_type& __x)
      {
	if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage)
	  {
	    _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish,
				     __x);
	    ++this->_M_impl._M_finish;
	  }
	else
	  _M_realloc_insert(end(), __x);
      }
```
_M_realloc_insert函数意义为，重新分配内存空间，并将元素__x插入到指定iterator之后，其函数定义和主要注释如下，其过程与常规的传统动态数组重新分配内存空间过程一致。值得注意的是里面也加入了异常处理，保证程序的正常执行。
```cpp


  template<typename _Tp, typename _Alloc>
    template<typename... _Args>
      void
      vector<_Tp, _Alloc>::
      _M_realloc_insert(iterator __position, _Args&&... __args)
    {
    // 获取增长后的内存大小
      const size_type __len =
	_M_check_len(size_type(1), "vector::_M_realloc_insert");
      const size_type __elems_before = __position - begin();
      pointer __new_start(this->_M_allocate(__len));            // 分配一个新的内存空间，长度为先前获取的 #__len
      pointer __new_finish(__new_start);                    
      __try
	{
	  _Alloc_traits::construct(this->_M_impl,
				   __new_start + __elems_before,
				   std::forward<_Args>(__args)...);             // __new_start + __elems_before 为 插入元素在新的内存空间的位置
	  __new_finish = pointer();

	  __new_finish
	    = std::__uninitialized_move_if_noexcept_a
	    (this->_M_impl._M_start, __position.base(),
	     __new_start, _M_get_Tp_allocator());                   // 将_M_start至position 之间的元素全部复制到 __new_start 之中

	  ++__new_finish;

	  __new_finish
	    = std::__uninitialized_move_if_noexcept_a               // 将position 至 _M_finish 的元素复制到_new_positon 之后
	    (__position.base(), this->_M_impl._M_finish,
	     __new_finish, _M_get_Tp_allocator());
	}
      __catch(...)
	{
	  if (!__new_finish)                
	    _Alloc_traits::destroy(this->_M_impl,                   // 异常发生时仅构造了插入元素，此时我们仅析构构造元素
				   __new_start + __elems_before);           
	  else                                                      // 异常发生时，对新空间元素进行了复制操作，需要对其进行析构
	    std::_Destroy(__new_start, __new_finish, _M_get_Tp_allocator());
	  _M_deallocate(__new_start, __len);                        // 释放新分配的内存空间
	  __throw_exception_again;
	}
      std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish,    // 释放原始内存空间
		    _M_get_Tp_allocator());
      _M_deallocate(this->_M_impl._M_start,
		    this->_M_impl._M_end_of_storage
		    - this->_M_impl._M_start);
      this->_M_impl._M_start = __new_start;                     // 设置新的三个指针
      this->_M_impl._M_finish = __new_finish;
      this->_M_impl._M_end_of_storage = __new_start + __len;
    }
```
这里需要注意的是_M_check_len操作，其用于返回下次应分配的内存空间。其采用的策略为eager模式，当我们仅申请一个很小的内存空间时，他并不是只简单地扩大了所需的内存空间大小，而是将原先的空间扩大了一倍。这样在我们连续进行push_back操作时，就不会造成需要进行连续的内存空间分配操作。如果我们申请的为一个大内存空间，则只是分配一个刚刚好的内存大小，避免内存浪费。
```cpp
 // Called by _M_fill_insert, _M_insert_aux etc.
      size_type
      _M_check_len(size_type __n, const char* __s) const
      {
	if (max_size() - size() < __n)              // max_size()返回内存分配器所能分配的最大内存大小
	  __throw_length_error(__N(__s))            // 如果所能分配的大小不满足需求，即max_size() > size() + __n，则抛出异常
      ;

	const size_type __len = size() + std::max(size(), __n);         // 新的内存空间为 2 * size() 和 size() + __n 中较大的
	return (__len < size() || __len > max_size()) ? max_size() : __len;
      }
```
#### insert
insert涉及到的操作主要有潜在的内存分配，插入位置之后的元素需要进行移位操作。`template< class InputIt > void insert( iterator pos, InputIt first, InputIt last)`，主要调用下面的函数来实现插入。对于数组中元素的移动操作，当向右移动时我们应该从backward（右向左）方向进行复制元素，而向左移动时则forward（左向右）复制，这样可以节省一个临时中转的内存空间。
```cpp
      template<typename _Alloc>
    template<typename _ForwardIterator>
      void
      vector<bool, _Alloc>::
      _M_insert_range(iterator __position, _ForwardIterator __first, 
		      _ForwardIterator __last, std::forward_iterator_tag)
      {
	if (__first != __last)                  // 先判断是否插入空元素
	  {
	    size_type __n = std::distance(__first, __last);     // 计算插入数据的大小
	    if (capacity() - size() >= __n)                     // 剩余空间充足
	      {
		std::copy_backward(__position, end(),
				   this->_M_impl._M_finish
				   + difference_type(__n));                 // 这里使用复制操作，将插入节点之后的元素后移
		std::copy(__first, __last, __position);             // 复制插入元素
		this->_M_impl._M_finish += difference_type(__n);    // 重新设置新的finish节点
	      }
	    else                                                // 剩余空间不足
	      {
		const size_type __len =
		  _M_check_len(__n, "vector<bool>::_M_insert_range");       // 这里调用_M_check_len操作，当插入小空间数据时，将直接使当前空间扩大一倍
		_Bit_pointer __q = this->_M_allocate(__len);                // 获取到新的内存空间
		iterator __start(std::__addressof(*__q), 0);                
		iterator __i = _M_copy_aligned(begin(), __position, __start);  // 复制begin() ~ __position
		__i = std::copy(__first, __last, __i);                          // 复制 first ~ last，也就是插入元素
		iterator __finish = std::copy(__position, end(), __i);          // 复制插入元素之后的元素
		this->_M_deallocate();
		this->_M_impl._M_end_of_storage = __q + _S_nword(__len);
		this->_M_impl._M_start = __start;
		this->_M_impl._M_finish = __finish;
	      }
	  }
      }
```
#### erase
删除操作实质为将erase节点之后的元素向前移动，覆盖erase节点
```cpp
erase(iterator __position)
{ return _M_erase(__position._M_const_cast()); }


  template<typename _Alloc>
    typename vector<bool, _Alloc>::iterator
    vector<bool, _Alloc>::
    _M_erase(iterator __position)
    {
      if (__position + 1 != end())
        std::copy(__position + 1, end(), __position);
      --this->_M_impl._M_finish;
      return __position;
    }
```
#### pop_back()
设置_M_finish，并且析构最后一个元素。
```cpp
      void
      pop_back() _GLIBCXX_NOEXCEPT
      {
	__glibcxx_requires_nonempty();
	--this->_M_impl._M_finish;
	_Alloc_traits::destroy(this->_M_impl, this->_M_impl._M_finish);
      }
```
#### destructor
对vector中的每个元素都要调用其相应的析构函数。
```cpp
      ~vector() _GLIBCXX_NOEXCEPT
      { std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish,
		      _M_get_Tp_allocator()); }

  template<typename _ForwardIterator, typename _Allocator>
    void
    _Destroy(_ForwardIterator __first, _ForwardIterator __last,
	     _Allocator& __alloc)
    {
      typedef __gnu_cxx::__alloc_traits<_Allocator> __traits;
      for (; __first != __last; ++__first)
	__traits::destroy(__alloc, std::__addressof(*__first));
    }
```
vector析构函数中并没有析构其分配的数组空间，其数组空间的析构是在基类`_Vecotr_base`中完成。其继承关系和函数调用如下
```cpp
	// 继承关系
	template<typename _Tp, typename _Alloc = std::allocator<_Tp> >
    class vector : protected _Vector_base<_Tp, _Alloc>

	// _Vecotr_base的析构函数
      ~_Vector_base() _GLIBCXX_NOEXCEPT
      { _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage
		      - this->_M_impl._M_start); }

      void
      _M_deallocate(pointer __p, size_t __n)
      {
	typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Tr;
	if (__p)
	  _Tr::deallocate(_M_impl, __p, __n);
      }
```
### 总结
vector的内存分配主要在添加原始时进行，而删除元素时并不会对其存储空间重新分配。所以变相可以说vector是一个只会不断增大的动态数组。当我们需要减少其空间大小是就必须调用shrink_to_fit来达到目的，其并不会自动调用。每次对vector中元素进行插入时都会调用元素对应的复制构造函数来进行元素构造，而删除时则调用析构函数来对其进行删除操作。