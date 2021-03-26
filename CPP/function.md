---
title: function
categories: CPP
tags:
 - CPP
---

# function

本文通过简单的例子来探究function的原理，指导function的使用

`std::function`是一个class模板，其能够存储各种`callable`对象，例如函数对象、函数指针、类成员函数、lambda等。

这里提供了两种实现方式，这两种方式为了便于查看代码，都只适配type(type)形式的函数指针。之后对于变参模板再给出实现。

第一种利用基类指针调用子类方法的方式实现，具体代码如下

```cpp
int getone(int num) {
    return 1;
}

struct gettwo {
    gettwo(){};
    int operator()(int num) {
        return 2;
    };
};

template<typename T>
class Function;

template<typename Re_Type, typename Args0>
class Function<Re_Type(Args0)> {
private:
    struct callable_base {
        virtual Re_Type operator()(Args0) = 0;
        virtual callable_base* copy() const = 0;
        virtual ~callable_base(){};
    };

    callable_base *base;							// 基类指针
	
    /* 模板类， */
    template<typename T>
    struct callable_derived : public callable_base {
        T f;
        callable_derived(T t):f(t){};
        
        virtual Re_Type operator()(Args0 args0) override{
            return f(args0);
        };

        virtual callable_base* copy() const override {
            return new callable_derived<T>(f);
        };

        virtual ~callable_derived() {};
    };

public:

    Function():base(nullptr) {};

    /* 函数模板，通过此模板可以获得传入的callable对象的类型（函数指针，函数对象...） */
    template<typename T>
    Function(T func) {
        base = new callable_derived<T>(func);		// 利用T，实例化对应的derived对象
    }

    Function(const Function& fun) : base(fun.base->copy()) {
    }

    Function& operator=(const Function& fun) {
        if(this == &fun)
            return *this;
        delete base;
        if(fun.base == nullptr)
            base = nullptr;
        else
            base = fun.base->copy();		// 复制对应对象的派生类
        return *this;
    }

    ~Function() {
        delete base;
    }

    Re_Type operator()(Args0 args0) {
        return (*base)(args0);
    }

};

int main(int argc, char const *argv[])
{

    Function<int(int)> f1 = Function<int(int)>(getone);
    Function<int(int)> f2 = Function<int(int)>(gettwo());

    Function<int(int)> f3 = f1;
    std::cout << f1(1) << std::endl;
    std::cout << f2(2) << std::endl;
    std::cout << f3(3) << std::endl;

    return 0;
}
```

下一个方法将callable实体以`void *`形式存储，此时我们需要再`Function`对象中保存3个函数指针，这三个函数指针用于对`void *`进行适当的处理,来达到多态的效果.

```cpp
template<typename T>
class Function;

template<typename Re_Type, typename Args0>
class Function<Re_Type(Args0)> {
private:
    Re_Type (*call_func)(const Function&, Args0);
    void* (*copy_func)(const Function&);
    void (*destory_func)(Function&);

    /* 3个模板函数,能够生成对应callable类型的代码 */
    template<typename T>
    static Re_Type call(const Function& fun, Args0 args0) {
        T* fp = static_cast<T *>(fun.callable_any);
        (*fp)(args0);
    };

    template<typename T>
    static void* copy(const Function& fun) {
        return new T(*(static_cast<T*>(fun.callable_any)));
    };

    template<typename T>
    static void destory(Function& fun) {
        delete static_cast<T*>(fun.callable_any);
    };

    void *callable_any;
    
public:

    template<typename T>
    Function(T fun)
     : callable_any(new T(fun)), 
       call_func(call<T>), 
       copy_func(copy<T>), 
       destory_func(destory<T>) {
    }

    Function(const Function& from) 
     : callable_any(from.copy_func(from)),
       call_func(from.call_func),
       copy_func(from.copy_func),
       destory_func(from.destory_func)
       {
        
    }

    Re_Type operator()(Args0 args) {
        call_func(*this, args);
    };

    Function& operator=(const Function& from) {
        if(this == &from)
            return *this;
        destory_func(*this);
        callable_any = from.copy_func(from);
        call_func = from.call_func;
        copy_func = from.copy_func;
        destory_func = from.destory_func;
        return *this;
    }

    ~Function() {
        destory_func(*this);
    }
};
```

最后的代码中,引入变参模板来令模板函数支持任意函数签名的函数

```cpp

template<typename T>
class Function;

template<typename Re_Type, typename ...Args>
class Function<Re_Type(Args...)> {
private:
    struct callable_base
    {
        virtual Re_Type call(Args&& ...args) = 0;
        virtual callable_base* copy() = 0;
        virtual ~callable_base(){};
    };
    
    template<typename T>
    struct callable_derived : public callable_base
    {
        T fun;
        callable_derived(T f)
          :fun(f) {
        };

        virtual Re_Type call(Args&& ...args) {			// 定义了支持变参的call函数
            return fun((std::forward<Args>(args))...);
        };
        virtual callable_base* copy() {
            return new callable_derived(*this);
        }
        virtual ~callable_derived() {

        };
    };

    callable_base* base;

public:

    template<typename T>
    Function(T f)
      :base(new callable_derived<T>(f)) {
    }

    Function(const Function& from) {
        base = from.base->copy();
    }

    Function& operator=(const Function& from) {
        if(this == &from)
            return *this;
        delete base;
        base = from.base->copy();
        return *this;
    }

    Re_Type operator()(Args&& ...args) {			// operator() 支持变参
        base->call((std::forward<Args>(args))...);
    }

    ~Function() {
        delete base;
    }
    
};

int ff(int a, int b, int c) {
    std::cout << a << "  " << b << "   " << c << std::endl;
}

struct ff_struct {
    int operator()(int a, int b, int c) {
        std::cout << a << "  " << b << "   " << c << std::endl;
    }
};

int main(int argc, char const *argv[]) 
{
    Function<int(int, int, int)> fun1 = ff;
    Function<int(int, int, int)> fun2 = ff_struct();
    fun1(1, 2, 3);
    fun2(4, 5, 6);
    return 0;
}

```

