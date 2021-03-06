# 单例模式

**what**

一种设计模式，用于保证全局中仅存在唯一实例

**why**

程序中某些类在设计时要求必须仅有一个实例，如果存在多个实例那么在操作时可能会导致程序的异常行为。因此为了保证这一要点我们必须采用某些特殊技术，来限制单例类的构造。单例模式避免了某些错误的行为造成多个实例出现的问题。

**how**

无论是哪种单例模式实现，他们所依托的方式都是私有化构造函数，只将构造函数暴露给特定函数。而这些函数负责保证这些类只存在一个实例。

## 单例模式要点

* 全局中能存在一个实例，禁止用户自己声明构造实例
* 线程安全
* 禁止拷贝、移动、赋值
* 用户通过接口获得实例

## static local实现

**static local指针**
```cpp
class singleton {

private:
    singleton();
    singleton(singleton&) = delete;
    singleton(singleton&&) = delete;
    singleton& operator=(singleton&) = delete;
    singleton& operator=(singleton&&) = delete;

public:
    static singleton* get_instance() {
        static singleton* instance = new singleton();
        return instance;
    };
};
```

**static local变量**
```cpp

class singleton {

private:
    singleton();
    singleton(singleton&) = delete;
    singleton(singleton&&) = delete;
    singleton& operator=(singleton&) = delete;
    singleton& operator=(singleton&&) = delete;

public:
    static singleton* get_instance() {
        static singleton instance;
        return &instance;
    };
};
```

这两种实现均利用static local仅被初始化一次，并且初始化时线程安全两个特性来实现单例模式。关于static local初始化时线程安全，可以参考此链接:[C++函数内的静态变量初始化以及线程安全问题？](https://www.zhihu.com/question/267013757)

**对比**

这两个实现方法推荐第二个，两者的差别仅在于`实例`所处的内存空间。第一个实例使用new进行构造，因此其实例保存于heap当中，对于内存分配算法来说我们需要某些结构去维护已分配的内存，来避免重复分配。如果大量的内存都采用这种方式进行分配，那么这个结构的维护成本将会很高。并且单例模式的内存将一直保存到程序的终止才会释放，因此这个时间成本会伴随整个程序的生命周期。第二种，内存空间分配于bss段，在编译时已经分配完成，其地址固定。相较于第一种方法，其可能导致程序运行初期需要较大的内存空间。

## mutex + 双检锁实现

```cpp
class singleton {

private:
    singleton();
    singleton(singleton&) = delete;
    singleton(singleton&&) = delete;
    singleton& operator=(singleton&) = delete;
    singleton& operator=(singleton&&) = delete;

public:
    static singleton* get_instance() {
        if(instance == nullptr) {
            lock_guard<std::mutex> lock(singleton_mutex);
            if(instance == nullptr) {
                instance = new singleton();
            }
        }
        return instance;
    }
    static singleton* instance;
    static std::mutex singleton_mutex;
};

singleton* singleton::instance = nullptr;
std::mutex singleton::singleton_mutex;
```

这种方式我们需要一个额外的锁去保证我们对于实例初始化时的互斥访问。避免两个线程同时对`instance`指针进行初始化。这里采用了双检锁(double-checked-locking)的方式来避免每次`get_instance`时都需要进行加锁，因为加锁需要进入内核态，会花费较为昂贵的开销还影响了并发性。

## 单例模板

单例操作时，我们可以通过模板的方式来快速实现一个单例类，来达到我们的目的。

```cpp
template<typename T>
class singleton_template {

public:
    static T* get_instance() {
        static T instance;
        return &instance;
    }

private:
    singleton_template(singleton_template&) = delete;
    singleton_template(singleton_template&&) = delete;
    singleton_template& operator=(singleton_template&) = delete;
    singleton_template& operator=(singleton_template&&) = delete;

protected:
    singleton_template(){};
};

class singleton : public singleton_template<singleton> {
    friend class singleton_template<singleton>;
    
private:
    singleton(){};
};


int main() {
    singleton* i1 = singleton::get_instance();
    singleton* i2 = singleton::get_instance();
    if(i1 == i2)
        cout << "Haha" << endl;
    else
        cout << "5555" << endl;

}
```

此种方法在模板基类中删除了赋值、移动、构造，保证了子类不会产生相关的默认函数。

这种方式我们需要在单例类中进行如下操作

1. 继承模板基类
2. 显示声明模板基类友元，让其能够访问子类的构造函数

---

```cpp
template<typename T>
class singleton_template {

public:
    static T* get_instance() {
        static T instance{token()};
        return &instance;
    }

private:
    singleton_template(singleton_template&) = delete;
    singleton_template(singleton_template&&) = delete;
    singleton_template& operator=(singleton_template&) = delete;
    singleton_template& operator=(singleton_template&&) = delete;

protected:
    singleton_template() = default;
    struct token {};
};

class singleton : public singleton_template<singleton> {

public:
    singleton(token){};
};
```
相较于第一种方法，此方法通过要求传递仅能由类内成员访问的token结构来保证其子类的构造函数只能由`singleton_template`访问，但是这也不是一个非常严格的单例实现，它可以通过继承来窃取其构造函数，过程如下
```cpp
class hack : public singleton {
public:
    static singleton* hackInstance() {
        return new singleton(token());
    };
};
```
---
```cpp
template<typename T>
class singleton_template : public T {

private:
    singleton_template() = default;
    singleton_template(singleton_template&) = delete;
    singleton_template& operator=(singleton_template&) = delete;
    singleton_template& operator=(singleton_template&&) = delete;

public:
    static singleton_template* get_instance() {
        static singleton_template instance;
        return &instance;
    }
};

class singleclass {

protected:
    singleclass() = default;
};

using singleclass_singleton = singleton_template<singleclass>;
```
第三种方法对于单例类来说是最简单的，其仅需要将默认构造函数声明为protected，并在头文件中声明模板别名即可。不过也不能保证继承单例安全。
