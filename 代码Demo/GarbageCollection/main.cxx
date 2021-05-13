#include <iostream>
#include "gc_ptr.h"
using namespace std;



class A : ENABLE_GC
{
public:
	gc_ptr<A>		next;

	A(int a)
	{
	}

    virtual void f() {cout << "A" << endl;};

	~A()
	{
		// assert(next.operator->() == nullptr);
	}
};

class B : ENABLE_GC, public virtual A
{
public:
	B(int a) :A(a)
	{
	}

    virtual void f() {cout << "B" << endl;};

	~B()
	{
		// assert(next.operator->() == nullptr);
	}
};

class C : ENABLE_GC, public virtual A
{
public:
	C(int a) :A(a)
	{
	}

    virtual void f() {cout << "C" << endl;};

	~C()
	{
		// assert(next.operator->() == nullptr);
	}
};

// class D : ENABLE_GC, public B, public C
// {
// public:
// 	D(int a) :A(a), B(a), C(a)
// 	{
// 	}

// 	~D()
// 	{
// 		// assert(next.operator->() == nullptr);
// 	}
// };



int main() {
    
	int step_size = 1024;		// collect whenever the increment of the memory exceeds <step_size> bytes
	int max_size = 8192;		// collect whenever the total memory used exceeds <max_size> bytes
    gcInit(8192, 1024);
	for (int i = 0; i < 65536; i++)
	{
        auto a = make_gc<A>(1);
		auto b = make_gc<B>(1);
		auto c = make_gc<C>(2);
		
        /* 循环引用测试， 基类指针指向子类 */
        a->next = b;
        b->next = c;
        c->next = a;

        /* 动态类型转化 */
        gc_ptr<A> gptra = c;
        // auto gptrc = dynamic_gc_cast<C>(a);    gptr.ptr == nullptr
        auto gptrc = dynamic_gc_cast<C>(gptra);
        gptra->f();    
        gptrc->f();
        
		if (i % 1000 == 0)
		{
			cout << i << endl;
		}
	}
    gcDestory();
}