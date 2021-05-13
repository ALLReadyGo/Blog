---
title: array源码分析
categories: CPP
tags: 
 - CPP
---

array容器封装了一个固定大小的数组，其语义上等价于一个struct，其内部只封装了`T[N]`作为其唯一成员。这样便可以既拥有C-style数组的性能也拥有标准容器的便利性。
其实现等价于如下

```cpp
template<typename _Tp, std::size_t _Nm>
struct array {
	_Tp _M_elems[_Nm];
}
```
我们都知道标准容器在内存分配中采用的方式为alloc内存分配器分配，其从根本上来说都是调用动态内存分配。那么这种内存分配方式就存在性能损耗，我们程序运行过程中就不得不花费计算性能在获得一块合适内存上。并且在多线程编程中，内存分配器有可能还必须保证线程安全，那么锁的引入又带来了性能损耗。相较于动态内存分配的性能问题，栈上的内存分配是在编译期间就决定好的，所以在运行时并不存在任何的性能损耗。因此栈中定义数组就有了得天独厚的性能优势。如下测试代码，证明了栈中定义数组所处的内存空间。
```cpp
int main() {
    int k = 3;
    int a[10];
    cout << &k << "\n" << &a[0];
}
```
## array
array的实现就是在struct中定义数组作为成员变量，其定义如下。_M_elems便是其数组定义
```cpp
  template<typename _Tp, std::size_t _Nm>
    struct array
    {
      typedef _Tp 	    			      value_type;
      typedef value_type*			      pointer;
      typedef const value_type*                       const_pointer;
      typedef value_type&                   	      reference;
      typedef const value_type&             	      const_reference;
      typedef value_type*          		      iterator;						// 迭代器就是普通的指针
      typedef const value_type*			      const_iterator;
      typedef std::size_t                    	      size_type;
      typedef std::ptrdiff_t                   	      difference_type;
      typedef std::reverse_iterator<iterator>	      reverse_iterator;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

      // Support for zero-sized arrays mandatory.
      typedef _GLIBCXX_STD_C::__array_traits<_Tp, _Nm> _AT_Type;
      typename _AT_Type::_Type                         _M_elems;          // 此处定义成员变量 _Tp[_Nm]数组
      ...
    }
```
查看__array_traits定义，可以看到如下定义，其内部数据类型_Type便为 _Tp[]。
```cpp
  template<typename _Tp, std::size_t _Nm>
    struct __array_traits
    {
      typedef _Tp _Type[_Nm];
      typedef __is_swappable<_Tp> _Is_swappable;
      typedef __is_nothrow_swappable<_Tp> _Is_nothrow_swappable;

      static constexpr _Tp&
      _S_ref(const _Type& __t, std::size_t __n) noexcept
      { return const_cast<_Tp&>(__t[__n]); }

      static constexpr _Tp*
      _S_ptr(const _Type& __t) noexcept
      { return const_cast<_Tp*>(__t); }
    };
```
除此之外还能看到`struct __array_traits<_Tp, 0>`定义，这属于模板特化。当_Nm == 0时，` typedef _Tp _Type[_Nm]`显然无法通过编译，所以这里将其_Type定义为了一个空的struct。这是实现size == 0的array的技术原理。
```cpp
 template<typename _Tp>
   struct __array_traits<_Tp, 0>
   {
     struct _Type { };
     typedef true_type _Is_swappable;
     typedef true_type _Is_nothrow_swappable;

     static constexpr _Tp&
     _S_ref(const _Type&, std::size_t) noexcept
     { return *static_cast<_Tp*>(nullptr); }

     static constexpr _Tp*
     _S_ptr(const _Type&) noexcept
     { return nullptr; }
   };
```
#### construct
其construct为隐式声明，并没有显示声明任何构造函数。
所以其构造为默认构造函数的一般过程，即会默认构造内部变量` _M_elems`，其服从C-stype数组的调用规则，自定义类必须提供默认构造函数，否则无法通过编译。
可以看出其并未定义初始化列表构造函数，这也是为什么array初始化时要这样书写的原因。最外层`{  }`用于struct，其内部`{}`用于_M_elems数组，等价于C-style struct的初始化方式。
```cpp
array<int, 3> arr = {{1, 2, 3}};
```
#### iterator
```cpp
      _GLIBCXX17_CONSTEXPR iterator
      begin() noexcept										// iterator
      { return iterator(data()); }

      _GLIBCXX17_CONSTEXPR const_iterator
      begin() const noexcept								// const iterator
      { return const_iterator(data()); }

      _GLIBCXX17_CONSTEXPR iterator
      end() noexcept
      { return iterator(data() + _Nm); }

      _GLIBCXX17_CONSTEXPR const_iterator
      end() const noexcept
      { return const_iterator(data() + _Nm); }
```
data()的定义如下，直接返回_M_elems数组
```cpp
      _GLIBCXX17_CONSTEXPR pointer
      data() noexcept
      { return _AT_Type::_S_ptr(_M_elems); }
```
#### element access
operator[]直接调用`_S_ref(_M_elems, __n)`，而at(__n)则在调用之前进行了边界检查
```cpp
      // Element access.
      _GLIBCXX17_CONSTEXPR reference
      operator[](size_type __n) noexcept
      { return _AT_Type::_S_ref(_M_elems, __n); }

      constexpr const_reference
      operator[](size_type __n) const noexcept
      { return _AT_Type::_S_ref(_M_elems, __n); }

      _GLIBCXX17_CONSTEXPR reference
      at(size_type __n)
      {
	if (__n >= _Nm)
	  std::__throw_out_of_range_fmt(__N("array::at: __n (which is %zu) "
					    ">= _Nm (which is %zu)"),
					__n, _Nm);
	return _AT_Type::_S_ref(_M_elems, __n);
      }
```
`_S_ref(_M_elems, __n)`定义如下，其等价于数组元素的直接访问
```cpp
      static constexpr _Tp&
      _S_ref(const _Type& __t, std::size_t __n) noexcept
      { return const_cast<_Tp&>(__t[__n]); }
```