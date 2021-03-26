---
title: weak_ptr源码分析
categories: CPP
tags:
 - CPP
---

weak_ptr是作为shared_ptr的补充而出现的，用于避免循环引用问题。
weak_ptr结构与shared_ptr近乎一致，这里直接写出部分结构和关键函数。

1. weak_ptr<_Tp>继承自__weak_ptr<_Tp>，其主要功能都在__weak_ptr<_Tp>中实现。
2. __weak_ptr包含数据成员`element_type*	 _M_ptr; `和` __weak_count<_Lp>  _M_refcount;`
3. weak_ptr和__weak_ptr的copy construct和operation=都为default。

综上我们不难看出其引用计数的维护依靠于成员变量` __weak_count<_Lp>  _M_refcount;`，所以我们之后将重点分析`_M_refcount`。
#### 构造分析
weak_ptr包含两个主要的构造函数，默认构造和通过`shared_ptr`进行构造。让我们先来分析默认构造过程：
```cpp
constexpr weak_ptr() noexcept = default;      
```
weak_ptr直接调用default默认构造函数，其会调用基类__weak_ptr的默认构造函数。
```cpp
constexpr __weak_ptr() noexcept
      : _M_ptr(nullptr), _M_refcount()
      { }
```
 __weak_ptr的默认构造就是对两个变量进行初始化，其中_M_ptr被赋值为nullptr，_M_refcount()调用默认构造。
```cpp
constexpr __weak_count() noexcept : _M_pi(nullptr)
      { }
```
其数据成员`_M_pi`被设置为nullptr,  __weak_count中包含数据成员如下，此变量在shared_ptr中也出现了，这便是weak_ptr的如何与shared_ptr交互的本质。
```cpp
_Sp_counted_base<_Lp>*  _M_pi;
```
接下来分析通过shared_ptr进行构造的过程：
```cpp
      template<typename _Yp,
	       typename = _Constructible<const shared_ptr<_Yp>&>>
	weak_ptr(const shared_ptr<_Yp>& __r) noexcept
	: __weak_ptr<_Tp>(__r) { }
```
其依然是调用__weak_ptr<_Tp>的构造函数
```cpp
      template<typename _Yp, typename = _Compatible<_Yp>>
	__weak_ptr(const __shared_ptr<_Yp, _Lp>& __r) noexcept
	: _M_ptr(__r._M_ptr), _M_refcount(__r._M_refcount)
	{ }
```
这里我们看到其是通过复制构造复制了shared_ptr中的_M_ptr和_M_refcount，让我们再紧接着查看__weak_count()的复制构造函数。
```cpp
     __weak_count(const __shared_count<_Lp>& __r) noexcept
      : _M_pi(__r._M_pi)
      {
	if (_M_pi != nullptr)
	  _M_pi->_M_weak_add_ref();
      }
```
其调用了`_M_pi->_M_weak_add_ref()`，具体实现如下：
```cpp
void
      _M_weak_add_ref() noexcept
      { __gnu_cxx::__atomic_add_dispatch(&_M_weak_count, 1); }
```
就是将_M_weak_count + 1

#### 赋值操作
这里直接查看`__weak_count`的operator=
```cpp
      __weak_count&
      operator=(const __shared_count<_Lp>& __r) noexcept
      {
	_Sp_counted_base<_Lp>* __tmp = __r._M_pi;
	if (__tmp != nullptr)
	  __tmp->_M_weak_add_ref();
	if (_M_pi != nullptr)
	  _M_pi->_M_weak_release();
	_M_pi = __tmp;
	return *this;
      }
```
这里与shared_ptr相似，让我们查看`_M_pi->_M_weak_release()`源码继续分析
```cpp
      void
      _M_weak_release() noexcept
      {
        // Be race-detector-friendly. For more info see bits/c++config.
        _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_weak_count);
	if (__gnu_cxx::__exchange_and_add_dispatch(&_M_weak_count, -1) == 1)
	  {
            _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_weak_count);
	    if (_Mutex_base<_Lp>::_S_need_barriers)
	      {
	        // See _M_release(),
	        // destroy() must observe results of dispose()
		__atomic_thread_fence (__ATOMIC_ACQ_REL);
	      }
	    _M_destroy();
	  }
      }
```
其会对`_M_weak_count`进行自减，并检验其值。如果值为1则调用`_M_destory()`。

#### 析构分析
`__weak_count`的析构函数如下：
```
      ~__weak_count() noexcept
      {
	if (_M_pi != nullptr)
	  _M_pi->_M_weak_release();
      }
```

#### 实体感知
我们直到weak_ptr能够感知其所指引的对象是否已经释放掉，这个功能看似非常神奇，实际上实现非常简单。让我们查看下use_count的实现。
其实现在基类__weak_ptr当中
```cpp
      long
      use_count() const noexcept
      { return _M_refcount._M_get_use_count(); }
```
其调用_M_refcount的成员函数
```cpp
      long
      _M_get_use_count() const noexcept
      { return _M_pi != nullptr ? _M_pi->_M_get_use_count() : 0; }

```
再调用指向的_M_pi的成员函数
```cpp
      long
      _M_get_use_count() const noexcept
      {
        // No memory barrier is used here so there is no synchronization
        // with other threads.
        return __atomic_load_n(&_M_use_count, __ATOMIC_RELAXED);
      }
```
实质上就是返回_M_pi的_M_use_count。
而expired函数则是，仅判断其use_count是否为0而已。
```cpp
      bool
      expired() const noexcept
      { return _M_refcount._M_get_use_count() == 0; }
```