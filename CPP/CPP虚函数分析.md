---
title: CPP虚函数分析
categories: CPP
tags:
 - CPP
---



```cpp
#include <iostream>
#include <string>
using namespace std;

/* 
 *关闭内存对其，我们能够看到类实际的空间大小
 *（从中我们可以看出，不光是结构体存在内存对齐，类也存在内存对齐）
 */
#pragma pack(1)

class A{
private:
public:
    int a;
    A(){};
    A(int a):a(a){};
    void show(){cout << "A class, a = " << a << endl;};
};

class B:public A{
private:
public:
    int b;
    B(){};
    B(int a, int b)
        :A(a),b(b){};
    virtual void show(){cout << "B class, b = " << b << endl;};
};

class C:public B{
private:
public:
    int c;
    C(){};
    C(int a, int b, int c)
        :B(a, b), c(c){};
    // C中并没有将show声明为virtual，但是由于其与B中声明的virtual show同名，所以也会成为虚函数。
    // 也就是说如果A中将show声明为虚函数，那么之后B，C不用显式声明也会设置为virtual
    void show(){cout << "C class, c = " << c << endl;};   
};

/*
 * MemA 描述了类A的内存情况，其中仅保存了一个int数据，（这是一个没有虚函数的类的内存情况）
 * 其中没有vptr是因为A没有定义虚函数，所以我们没有必要浪费那个空间去存储vptr
 * 但是这样却对导致A, B， C的数据布局不一致。
 * A offset 0为数据， 而B, C的offset 8才开始存储数据。
*/
struct MemA
{
    int a;
};


/*
 * MemC 描述了类C的内存情况，其offset +0存储虚函数表，+8存储A.a
*/
struct MemC
{
    void * vptr;   // 这里使用一个(void *)来存储vptr
    int a;          // 分别对应 A.a , B.b, C.c   
    int b;
    int c;
};

int main(){

    /* 各个类的内存大小 */
    cout << "sizeof A  = " << sizeof(A) << endl;    /* 4 bytes */
    cout << "sizeof B  = " << sizeof(B) << endl;    /* 16 bytes*/
    cout << "sizeof C  = " << sizeof(C) << endl;    /* 20 bytes*/

    /* 使用结构体修改类中的数据 */
    C c(1, 2, 3);
    cout << c.a << "   " << c.b << "   " << c.c << endl;   /*1     2       3*/
    MemC *pc = (MemC *)&c;   // 使用结构体指向类的内存空间
    cout << pc->a << "   " << pc->b << "   " << pc->c << endl; /* 1       2        3  */
    pc->a = 100;             // 使用结构体去修改类中的数据
    cout << c.a << "   " << c.b << "   " << c.c << endl;  /* 100      2      3*/

    /* 类指针的转换问题 */
    A *pa1, *pa2;
    pa1 = (A *)&c;   // 此处并不是将&c的值直接赋值给pa1，而是向后移动8 bytes，保证了C*向A*的正确转换 
    pa2 = (A *)(void *)&c;  // 此处并不会，因为这个转换只是void* 向 A*进行
    cout << pa1  << "      " << "pa1->a = " << pa1->a << endl;  // 0x7ffcc158f378      pa1->a = 100
    cout << pa2  << "      " << "pa2->a = " << pa2->a << endl;  // 0x7ffcc158f370      pa2->a = 1389382912

    /* 静态联编， 动态联编 */
    C *ppc = (C *)&c;
    ppc->show();         // C class, c = 3, 此处的调用顺序是，先访问vptr，通过vptr找到应该调用的函数地址
    pc->vptr = nullptr;  // 此处我们去去破坏vptr。
    c.show();           // C class, c = 3 ， 此处的调用顺序是，在编译阶段直接将c.show()指向C::show()，因为这个可以在编译期间确定
    (&c)->show();       // C class, c = 3,  此处同上，也在编译期间就能确定，这种方式可以加速执行速度(少了以此间接寻址过程)
    ppc->show();        // Segmentation fault (core dumped)


}
```
运行结果
![运行结果](https://img-blog.csdnimg.cn/20200305190300291.png)