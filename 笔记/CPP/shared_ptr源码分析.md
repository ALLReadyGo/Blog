---
title: shared_ptr源码分析
categories: CPP
tags:
 - CPP
---



shared_ptr是从C++11开始提供的智能指针，用于管理多个指针对同一实体的资源释放问题，保证在多个指针指向同一实体时，仅最后一个指针解除指向时才会释放资源。

#### shared_ptr构造
shared_ptr<_Tp>继承自__shared_ptr<_Tp>，其功能主要由其实现。让我们以单值构造函数为切入点开始分析。
```cpp
      template<typename _Yp, typename = _Constructible<_Yp*>>
	explicit
	shared_ptr(_Yp* __p) : __shared_ptr<_Tp>(__p) { }
```
从中我们可以看到，调用了基类的构造函数，定义如下
```cpp
      template<typename _Yp, typename = _SafeConv<_Yp>>
	explicit
	__shared_ptr(_Yp* __p)
	: _M_ptr(__p), _M_refcount(__p, typename is_array<_Tp>::type())
	{
	  static_assert( !is_void<_Yp>::value, "incomplete type" );
	  static_assert( sizeof(_Yp) > 0, "incomplete type" );
	  _M_enable_shared_from_this_with(__p);
	}
```
其初始化了成员变量`_M_ptr`和` _M_refcount`，这两个成员变量的定义如下。`element_type`就是构造时传入的指针指向类型，这里我们可以看到，share_ptr的实现就是最基本的指针加上引用计数。
```cpp
element_type*	   _M_ptr;         // Contained pointer.
__shared_count<_Lp>  _M_refcount;    // Reference counter.
```
让我们接下看看` _M_refcount`的构造过程，其调用如下构造函数，我们能够看到他初始化了数据成员`_M_pi`。
```cpp
      template<typename _Ptr>
        explicit
	__shared_count(_Ptr __p) : _M_pi(0)
	{
	  __try
	    {
	      _M_pi = new _Sp_counted_ptr<_Ptr, _Lp>(__p);
	    }
	  __catch(...)
	    {
	      delete __p;
	      __throw_exception_again;
	    }
	}
```
数据成员_M_pi的定义如下，他是一个基类的指针，可以指向任何派生类，所以` _M_pi = new _Sp_counted_ptr<_Ptr, _Lp>(__p)`并不存在任何语法问题。
```cpp
_Sp_counted_base<_Lp>*  _M_pi;
```
让我们接着查看代码，首先是`_Sp_counted_ptr<_Ptr, _Lp>(__p)`的构造，其初始化了数据成员`_Ptr             _M_ptr`，这里我们可以发现`_Sp_counted_ptr`存在一个指针指向其引用的对象。
```cpp
explicit
_Sp_counted_ptr(_Ptr __p) noexcept
: _M_ptr(__p) { }
```
从上面的代码中我们可以看出，其没有显式调用基类构造函数，所以使用基类默认构造函数，其默认构造函数如下：
```cpp
_Sp_counted_base() noexcept
: _M_use_count(1), _M_weak_count(1) { }
```
其初始化数据成员`_M_use_count`，`_M_weak_count`。`_M_use_count`表示的为共有多少share_ptr指向内存实体，当`_M_use_count == 0`时便会释放内存所指向的实体。之后的析构分析我们便会看到其如何使用，`_M_weak_count`是供`weak_ptr`使用，保证`weak_ptr`能够感知其实体是否依然存在。
```cpp
      _Atomic_word  _M_use_count;     // #shared
      _Atomic_word  _M_weak_count;    // #weak + (#shared != 0)
```
#### 赋值分析
赋值包括两个主要操作：copy construc和operator=。查看相应的函数，我们可以看到他们的定义如下：
```cpp
shared_ptr& operator=(const shared_ptr&) noexcept = default;
shared_ptr(const shared_ptr&) noexcept = default;

__shared_ptr(const __shared_ptr&) noexcept = default;
__shared_ptr& operator=(const __shared_ptr&) noexcept = default;
```
基类和子类的copy construc和operator=都使用默认方法，也就是说其在调用时会分别调用成员变量的copy construc和operator=。派生类没有数据成员，而基类__shared_ptr包含成员变量。
```cpp
 element_type*	   _M_ptr;         // Contained pointer.
 __shared_count<_Lp>  _M_refcount;    // Reference counter.
```
`_M_ptr`为最基本的指针，所以每次都会进行重新赋值。而`__shared_count<_Lp>`为自定义class，我们继续跟踪其行为。
```cpp
      __shared_count(const __shared_count& __r) noexcept
      : _M_pi(__r._M_pi)
      {
	if (_M_pi != 0)
	  _M_pi->_M_add_ref_copy();
      }

      __shared_count&
      operator=(const __shared_count& __r) noexcept
      {
	_Sp_counted_base<_Lp>* __tmp = __r._M_pi;
	if (__tmp != _M_pi)
	  {
	    if (__tmp != 0)
	      __tmp->_M_add_ref_copy();
	    if (_M_pi != 0)
	      _M_pi->_M_release();
	    _M_pi = __tmp;
	  }
	return *this;
      }
```
在进行复制构造时，我们可以看到其令`this->_M_pi = other._M_pi`，这里`_M_pi`的定义为`_Sp_counted_base<_Lp>*  _M_pi`。他是一个指针，因此在复制构造时，此结构直接指向被复制对象的实体，保证了多个`share_ptr`只有一个引用计数实体。`operator=`操作不同的是会先取消之前的引用计数（如果计数归零还需要释放空间），再添加新的计数。
```cpp
      
      void
      _M_add_ref_copy()
      { __gnu_cxx::__atomic_add_dispatch(&_M_use_count, 1); }

      void
      _M_release() noexcept
      {
        // Be race-detector-friendly.  For more info see bits/c++config.
        _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_use_count);
	if (__gnu_cxx::__exchange_and_add_dispatch(&_M_use_count, -1) == 1)
	  {
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_use_count);
	    _M_dispose();
	    // There must be a memory barrier between dispose() and destroy()
	    // to ensure that the effects of dispose() are observed in the
	    // thread that runs destroy().
	    // See http://gcc.gnu.org/ml/libstdc++/2005-11/msg00136.html
	    if (_Mutex_base<_Lp>::_S_need_barriers)
	      {
		__atomic_thread_fence (__ATOMIC_ACQ_REL);
	      }

            // Be race-detector-friendly.  For more info see bits/c++config.
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_weak_count);
	    if (__gnu_cxx::__exchange_and_add_dispatch(&_M_weak_count,
						       -1) == 1)
              {
                _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_weak_count);
	        _M_destroy();
              }
	  }
      }
```
上述两个便是`_Sp_counted_base`的`_M_add_ref_copy()`和 `M_release()`，对于`_M_add_ref_copy()`其就是` _M_use_count + 1`。而对于`M_release()`，在析构时再详细介绍。
#### 析构分析
当`shared_ptr`超出其definite scope时，需要智能释放对应内存，即当其为最后一个引用时需要释放内存，而如果还存在其他引用则不进行释放。
`shared_ptr`和`__shared_ptr`都没有定义析构函数，所以我们直接查看`__shared_count<_Lp>  _M_refcount`数据成员的析构函数。其定义如下
```cpp
      ~__shared_count() noexcept
      {
	if (_M_pi != nullptr)
	  _M_pi->_M_release();
      }
```
依旧是调用`_M_pi->_M_release()`，现在让我们来看看其具体过程如何。
```cpp
      void
      _M_release() noexcept
      {
        // Be race-detector-friendly.  For more info see bits/c++config.
        _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_use_count);
	if (__gnu_cxx::__exchange_and_add_dispatch(&_M_use_count, -1) == 1)
	  {
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_use_count);
	    _M_dispose();
	    // There must be a memory barrier between dispose() and destroy()
	    // to ensure that the effects of dispose() are observed in the
	    // thread that runs destroy().
	    // See http://gcc.gnu.org/ml/libstdc++/2005-11/msg00136.html
	    if (_Mutex_base<_Lp>::_S_need_barriers)
	      {
		__atomic_thread_fence (__ATOMIC_ACQ_REL);
	      }

            // Be race-detector-friendly.  For more info see bits/c++config.
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_weak_count);
	    if (__gnu_cxx::__exchange_and_add_dispatch(&_M_weak_count,
						       -1) == 1)
              {
                _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_weak_count);
	        _M_destroy();
              }
	  }
      }
```
其先对`_M_use_count -- == 1`进行判断，注意这里判断的是减之前的值，如果值为1表示这是最后一个引用的`shared_ptr`，此时会调用`_M_dispose()`函数。
```cpp
	  // Called when _M_use_count drops to zero, to release the resources
      // managed by *this.
      virtual void
      _M_dispose() noexcept = 0;
```
其为虚函数，其实现由派生类实现，让我们查看其中一个派生类 `_Sp_counted_ptr<_Ptr, _Lp>(__p)`的实现，其完整代码如下
```cpp
  // Counted ptr with no deleter or allocator support
  template<typename _Ptr, _Lock_policy _Lp>
    class _Sp_counted_ptr final : public _Sp_counted_base<_Lp>
    {
    public:
      explicit
      _Sp_counted_ptr(_Ptr __p) noexcept
      : _M_ptr(__p) { }

      virtual void
      _M_dispose() noexcept
      { delete _M_ptr; }

      virtual void
      _M_destroy() noexcept
      { delete this; }

      virtual void*
      _M_get_deleter(const std::type_info&) noexcept
      { return nullptr; }

      _Sp_counted_ptr(const _Sp_counted_ptr&) = delete;
      _Sp_counted_ptr& operator=(const _Sp_counted_ptr&) = delete;

    private:
      _Ptr             _M_ptr;
    };
```
`_M_ptr`为一个指针，其在构造时由__shared_count传入，指向`shared_ptr`所对应的内存空间，因此我们可以发现，内存管理的任务全部交由`_M_pi`处理。
```cpp
      void
      _M_release() noexcept
      {
        // Be race-detector-friendly.  For more info see bits/c++config.
        _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_use_count);
	if (__gnu_cxx::__exchange_and_add_dispatch(&_M_use_count, -1) == 1)
	  {
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_use_count);
	    _M_dispose();
	    // There must be a memory barrier between dispose() and destroy()
	    // to ensure that the effects of dispose() are observed in the
	    // thread that runs destroy().
	    // See http://gcc.gnu.org/ml/libstdc++/2005-11/msg00136.html
	    if (_Mutex_base<_Lp>::_S_need_barriers)
	      {
		__atomic_thread_fence (__ATOMIC_ACQ_REL);
	      }

            // Be race-detector-friendly.  For more info see bits/c++config.
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_weak_count);
	    if (__gnu_cxx::__exchange_and_add_dispatch(&_M_weak_count,
						       -1) == 1)
              {
                _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_weak_count);
	        _M_destroy();
              }
	  }
      }
```
` _M_dispose()`执行完之后，会再对`&_M_weak_count - 1`并判断，如果原始值为0则销毁`_M_pi`。
这里不直接销毁的原因是`weak_ptr`也会引用这个内存数据，我们必须在`weak_ptr`都不进行引用时才能对其销毁。