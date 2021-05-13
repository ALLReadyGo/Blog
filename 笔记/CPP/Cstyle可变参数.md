---
title: C style 可变参数
categories: CPP
tags:
 - CPP
---



# 可变参数

C语言中`printf (const char *__restrict __fmt, ...)`，中`fmt`是格式化字符串用于控制输出，而`...`便是可变参数，它可以接受任意数量，任意类型的参数，是一种灵活的函数调用形式。
当用户想要解析传递的可变参数时，他需要包含`stdarg.h`头文件，以使用其内部定义的解析函数。
其定义的函数如下：
```c
 
#define va_start _crt_va_start
#define va_arg _crt_va_arg
#define va_end _crt_va_end

/* file path: Microsoft Visual Studio 9.0\VC\include\vadefs.h */
#define _ADDRESSOF(v)   ( &(v) )
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define _crt_va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define _crt_va_end(ap)      ( ap = (va_list)0 )
```
其中重点解释下`_INTSIZEOF(n)`，此函数的作用是求解类型的对齐大小，因为栈的存放是需要进行对齐的，所以栈中每个元素的开始地址必定为`int`的整数倍。如果有兴趣各位可以查一下内存对齐。
我们举例来说明其作用：
* `_INTSIZEOF(char)`，其中`char`类型长度为1字节，`int`类型长度为4字节。那么这个函数的返回值就为4字节。
* `_INTSIZEOF(some_struct)`，其中`some_struct`长度为6字节，那么这个函数的返回值就是8字节。

从中我们可以直观地看出这个函数将原有数值补齐为`int`大小的整数倍，这个大小就是对齐大小。

理解了函数作用我们再来看看这个函数是怎么实现的
假设此时`sizeof(n) == 6`，`sizeof(int) == 4`，那么我们向上取`6`的`4`倍数，等价于向下取`6 + 3`的`4`的倍数。向下取`4`的倍数可以使用如下方法试下`(sizeof(n) + sizeof(int) - 1) / sizeof(int) * sizeof(int)`。
`& ~(sizeof(int) - 1)` 便是和 `/ sizeof(int) * sizeof(int)`一样的作用。

此处我们还能看到宏定义函数其可以实现参数类型作为参数进行传递，其可以实现传统函数所不能实现的功能。

## 可变参数传递问题
 此时我们有一个需求，我们需要返回一个格式化字符串。其函数定义如下
 `char *out_str(const char* fmt, ...)`
 可以进行如下调用。
`char *str = out_str("name:%s    age:%d", "HelloWorld", 16)`。
我第一事件想到的就是封装一下`sprintf()`便可以简单实现，但是发现我无法传递`...`参数。之后查阅资料知道了`vsprintf()`这个函数，其函数定义如下和使用方式如下：
```c
// 函数定义
int vsprintf(char *str, const char *format, va_list arg)

// 调用方式，这个是我自己写的方便理解
int sprintf(char *buf, const char *fmt, ...){
    int status;
    va_list args;
    va_start(args, fmt);
    status = vsprintf(buf, fmt, args);
    va_end(args);
    return status;
}
```
从中我们可以看到`vsprintf()`相较于`sprintf()`就是将`...`变为了`va_list arg`。
但是这个却有很多本质上的区别：
* `va_list`只是传递了参数指针，所以最开始是我们需要调用`va_start`进行第一个可变参数的定位。
* `...`进行参数参数时必须将所有可变参数进行压栈，而`va_list`则只是给出其在栈中的位置，并没有进行那么多次的压栈操作
* `va_list`可以实现多次传递，而`...`则无法进行传递。

所以在我们使用可变参数进行函数定义时，我们应该指派两套函数：
`int sprintf(char *buf, const char *fmt, ...)`
`int vsprintf(char *str, const char *format, va_list arg)`
第一个函数是方便用户进行调用，第二个函数是方便函数的复用。