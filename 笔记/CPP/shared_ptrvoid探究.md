---
title: smart_ptr<void>探究
categories: CPP
tags:
 - CPP
---



# shared_ptr\<void\>探究

最近看代码发现了一个shared_ptr有趣的用法，首先来看看下面的这行代码

```cpp
struct cla {
    cla() {
        std::cout << "cla constructing" << std::endl;
    };

    ~cla() {
        std::cout << "cla destructing" << std::endl;
    };
};

int main(int argc, char const *argv[])
{
    {
        // 下面这行代码能否正常析构
        std::shared_ptr<void> void_ptr = std::make_shared<cla>();
    }
    return 0;
}

```

其输出为

```shell
cla constructing
cla destructing
```

可以看出其正常的析构了，但是这里就有疑问了。一个`void*`是如何能够析构`cla`的，按照我之前的理解，其shared_ptr内部实现中保存了两个关键数据，分别是`reference count`和`pointer`。`reference count`表示了有多少shared_ptr指向这个对象，当其归为0时我们便可以对pointer指向的内存进行析构，而具体调用哪个析构函数由pointer的类型决定，由于析构函数一般为virtual所以即使父类指向子类的情况也能够正常析构。但是这个观点显然面对这个例子很无力，我肯定之前遗漏了些什么。

```cpp
/* shared_ptr 继承自__shared_ptr<_Tp>，其具体实现由__shared_ptr<>实现 */
template<typename _Tp>
class shared_ptr : public __shared_ptr<_Tp>
{
    ...
}


/* 
  __shared_ptr内部包含两个关键元素
  一个是_M_ptr，其指向shared_ptr所指向的内存
  __shared_count<_Lp> 便是我们说的引用计数
  
*/
template<typename _Tp, _Lock_policy _Lp>
class __shared_ptr
: public __shared_ptr_access<_Tp, _Lp>
{
  ...
  element_type*	   _M_ptr;         // Contained pointer.
  __shared_count<_Lp>  _M_refcount;    // Reference counter.
};

/*
   __shared_count内部有一个_Sp_counted_base<_Lp>* _M_pi
   其是一个类的指针，我们来看看这个指针指向的对象是什么（此处注意其是基类指针）
*/
template<_Lock_policy _Lp>
class __shared_count
{
    ...
    _Sp_counted_base<_Lp>*  _M_pi;
}

/*
  
  _Atomic_word  _M_use_count;     由于标记shared_ptr引用
  _Atomic_word  _M_weak_count;    用于标记weak_ptr引用，具体看weak_ptr分析
  他们都是原子变量，所以异步状态下shared_ptr依然能够正确进行count的增减，这也就意味着其对于内存的管理是lock_free
  但是这依然不能解释shared_ptr<void>是如何工作的
*/
template<_Lock_policy _Lp = __default_lock_policy>
class _Sp_counted_base
: public _Mutex_base<_Lp>
{
  ...
  _Atomic_word  _M_use_count;     // #shared
  _Atomic_word  _M_weak_count;    // #weak + (#shared != 0)
};

/*
	此时我们回看__shared_count结构
	仔细观察构造函数，我们可以看到如下特征
	_Sp_counted_base<_Lp>*  _M_pi 是一个基类指针，其可以指向任意派生类
	再来查看构造函数，我们发现_Sp_counted_ptr<_Ptr, _Lp>(__p)作为了具体指派。
*/
template<_Lock_policy _Lp>
class __shared_count
{
    ... 
        
    // 构造函数
    template<typename _Ptr>
    explicit
    __shared_count(_Ptr __p) : _M_pi(0)
    {
    __try
    {
        _M_pi = new _Sp_counted_ptr<_Ptr, _Lp>(__p);	// 使用的是_Sp_counted_ptr<_Ptr, _Lp>类型
    }
    __catch(...)
    {
        delete __p;
        __throw_exception_again;
    }
    }
    ...
        
    _Sp_counted_base<_Lp>*  _M_pi;
}

/*
	这里直接贴出了所有代码
	可以看出其内部还保存了一个_M_Ptr，其类型是_Ptr
	这下破案了，析构时使用的并不是__shared_ptr._M_ptr，而是_Sp_counted_ptr._M_ptr。
*/
  template<typename _Ptr, _Lock_policy _Lp>
    class _Sp_counted_ptr final : public _Sp_counted_base<_Lp>
    {
    public:
      explicit
      _Sp_counted_ptr(_Ptr __p) noexcept
      : _M_ptr(__p) { }

      virtual void
      _M_dispose() noexcept
      { delete _M_ptr; }						// 释放指向内存，其是虚函数能够正确调用

      virtual void
      _M_destroy() noexcept
      { delete this; }

      virtual void*
      _M_get_deleter(const std::type_info&) noexcept
      { return nullptr; }

      _Sp_counted_ptr(const _Sp_counted_ptr&) = delete;
      _Sp_counted_ptr& operator=(const _Sp_counted_ptr&) = delete;

    private:
      _Ptr             _M_ptr;					// 其还有个_Ptr
    };
```

让我们再看看看析构过程来验证下我们的思路：

```cpp
/* 默认析构函数，调用基类析构 */
template<typename _Tp>
class shared_ptr : public __shared_ptr<_Tp>
{
	~shared_ptr() = default;
}

/* 基类析构函数default， 这里会调用__shared_count<_Lp>的析构函数（这样写是想利用RAII特性） */
template<typename _Tp, _Lock_policy _Lp>
class __shared_ptr
: public __shared_ptr_access<_Tp, _Lp>
{
    ~__shared_ptr() = default;		//
    element_type*	   _M_ptr;         // Contained pointer.
    __shared_count<_Lp>  _M_refcount;    // Reference counter.
}

/*
   __shared_count析构时会显式调用_M_pi->_M_release()，这是个虚函数其会调用具体指向的实体
   这里我们还能看到，_Sp_counted_base<_Lp>*是不带_Tp类型的，所以其可以在任意类型的shared_ptr中传递，
   而且具体指向_Sp_counted_ptr<_Tp,_Lp>是带_Tp类型的，所以这便保证了其内部存储了正确的类型信息
   这样的方法保证了传递任意性和具体指向类型的不变性
*/
template<_Lock_policy _Lp>
class __shared_count
{
    ~__shared_count() noexcept
    {
        if (_M_pi != nullptr)
        _M_pi->_M_release();
    }

    _Sp_counted_base<_Lp>*  _M_pi;
};
```

# unique_ptr\<void\>的可行性

```cpp
struct cla {
    cla() {
        std::cout << "cla constructing" << std::endl;
    };

    ~cla() {
        std::cout << "cla destructing" << std::endl;
    };
};

int main(int argc, char const *argv[])
{
    {
        // 下面这行代码能否正常析构
        std::unique_ptr<void> void_ptr = std::make_unique<cla>();
    }
    return 0;
}
```

答案是连编译都过不去，具体原因直接看代码

```bash
In file included from /usr/include/c++/7/memory:80:0,
                 from main.cpp:6:
/usr/include/c++/7/bits/unique_ptr.h: In instantiation of ‘void std::default_delete<_Tp>::operator()(_Tp*) const [with _Tp = void]’:
/usr/include/c++/7/bits/unique_ptr.h:268:17:   required from ‘std::unique_ptr<_Tp, _Dp>::~unique_ptr() [with _Tp = void; _Dp = std::default_delete<void>]’
main.cpp:22:64:   required from here
/usr/include/c++/7/bits/unique_ptr.h:74:2: error: static assertion failed: can't delete pointer to incomplete type
  static_assert(!is_void<_Tp>::value,
```

上面的那段代码经历了两个过程

```cpp
std::unique_ptr<cla> cla_ptr = std::make_unique<cla>();    // progress 1

std::unique_ptr<void> void_ptr = std::move(cla_ptr);	   // progress 2
```

先让我们看progress 1

```cpp
template <typename _Tp, typename _Dp = default_delete<_Tp>>
class unique_ptr
{
    __uniq_ptr_impl<_Tp, _Dp> _M_t;				// 具体实现类
    // ...
    template <typename _Up = _Dp, typename = _DeleterConstraint<_Up>>
    explicit unique_ptr(pointer __p) noexcept
      : _M_t(__p)								// 利用传入指针构造_M_t
    { }
    
      /// Destructor, invokes the deleter if the stored pointer is not null.
      ~unique_ptr() noexcept
      {
            auto& __ptr = _M_t._M_ptr();		
            if (__ptr != nullptr)
              get_deleter()(__ptr);				// 利用deleter释放掉_M_t._M_ptr
            __ptr = pointer();
      }
}

/*  __uniq_ptr_impl完整代码  */
 template <typename _Tp, typename _Dp>
    class __uniq_ptr_impl
    { 
      template <typename _Up, typename _Ep, typename = void>
	struct _Ptr
	{
	  using type = _Up*;
	};

      template <typename _Up, typename _Ep>
	struct
	_Ptr<_Up, _Ep, __void_t<typename remove_reference<_Ep>::type::pointer>>
	{
	  using type = typename remove_reference<_Ep>::type::pointer;
	};

    public:
      using _DeleterConstraint = enable_if<
        __and_<__not_<is_pointer<_Dp>>,
	       is_default_constructible<_Dp>>::value>;

      using pointer = typename _Ptr<_Tp, _Dp>::type;

      __uniq_ptr_impl() = default;
      __uniq_ptr_impl(pointer __p) : _M_t() { _M_ptr() = __p; }

      template<typename _Del>
      __uniq_ptr_impl(pointer __p, _Del&& __d)
	: _M_t(__p, std::forward<_Del>(__d)) { }

      pointer&   _M_ptr() { return std::get<0>(_M_t); }
      pointer    _M_ptr() const { return std::get<0>(_M_t); }
      _Dp&       _M_deleter() { return std::get<1>(_M_t); }					// 获取deleter
      const _Dp& _M_deleter() const { return std::get<1>(_M_t); }

    private:
      tuple<pointer, _Dp> _M_t;												// 其内部保存了deleter
    };
```

这里面也保存了deleter，为什么析构就出现了异常？其实如果观察仔细我们可以发现`__uniq_ptr_impl<_Tp, _Dp>`是一个带类型的类，一个`__uniq_ptr_impl<cla, _Dp>` 是不可能直接赋值给`__uniq_ptr_impl<cla, _Dp>`，所以在progress 2阶段肯定发生了转化

```cpp
template <typename _Tp, typename _Dp = default_delete<_Tp>>
class unique_ptr
{
    __uniq_ptr_impl<_Tp, _Dp> _M_t;				// 具体实现类
    // ...
    
   /** @brief Converting constructor from another type
   *
   * Requires that the pointer owned by @p __u is convertible to the
   * type of pointer owned by this object, @p __u does not own an array,
   * and @p __u has a compatible deleter type.
   */
   /** @brief 转化构造函数
   * 需要__u所包含的指针与this所包含的指针兼容，并且拥有一个兼容的deleter
   * 这里对模板编程不太了解，不过可以看出，其应该是为了编译时类型检查
   */
    template<typename _Up, typename _Ep, typename = _Require<__safe_conversion_up<_Up, _Ep>,
	       typename conditional<is_reference<_Dp>::value,is_same<_Ep, _Dp>,        
		   is_convertible<_Ep, _Dp>>::type>>
	unique_ptr(unique_ptr<_Up, _Ep>&& __u) noexcept
	: _M_t(__u.release(), std::forward<_Ep>(__u.get_deleter()))
	{ }      
}   


template <typename _Tp, typename _Dp>
class __uniq_ptr_impl
{ 
// 这里简单粗暴，直接tuple<pointer, _Dp> _M_t = tuple(__p, std::forward<_Del>(__d));
// 那么这便要求，default_delete<_Dp> 能够与 default_delete<_Del>
// 并且也可以看到一件事，deleter类型在这里发生了丢失！！！！
template<typename _Del>
__uniq_ptr_impl(pointer __p, _Del&& __d)
    : _M_t(__p, std::forward<_Del>(__d)) { }
    
}

/// Primary template of default_delete, used by unique_ptr
/// 这是个函数模板，其不占用大小！！
/// 这些解决问题了，原来在unique_ptr进行转换的过程中发生转换的不止内部指针，其内部default_delete也会发生改变。
template<typename _Tp>
struct default_delete
{
    /// Default constructor
    constexpr default_delete() noexcept = default;

    /** @brief Converting constructor.
    *
    * Allows conversion from a deleter for arrays of another type, @p _Up,
    * only if @p _Up* is convertible to @p _Tp*.
    */
    template<typename _Up, typename = typename						
    enable_if<is_convertible<_Up*, _Tp*>::value>::type>				// 会进行类型测试
    default_delete(const default_delete<_Up>&) noexcept { }

    /// Calls @c delete @p __ptr
    void perator()(_Tp* __ptr) const
    {
        static_assert(!is_void<_Tp>::value,					
        "can't delete pointer to incomplete type");					// 我们直接找到了报错的元凶，原来default_delete是不能释放void
        static_assert(sizeof(_Tp)>0,
        "can't delete pointer to incomplete type");
        delete __ptr;
    }
};
```

为什么不像shared_ptr那样呢，答案在占用空间上，如果我们想要动态保存deleter，势必需要利用到指针（基类、子类特性）。这样unique_ptr内部将保存两个指针。而如果采用刚才那样的方式，default_delete是不占用内存空间的（因为他是一个内部没有元素的函数对象），所以unique_ptr实际占用的大小与一个普通指针一致，符合smart、fast特性。