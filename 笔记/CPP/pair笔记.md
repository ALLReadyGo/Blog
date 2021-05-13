---
title: pair笔记
categories: CPP
tags:
 - CPP
---



## pair

pair是一个struct 模板，其定义如下
```cpp
template<
    class T1,
    class T2
> struct pair;
```
从中我们可以看出其实现依靠struct。从使用方面进行考虑，其为programmer提供了方便的使用，我们不必为每次使用一个简单的二元struct而专门定义一个struct自定义类型，而是交由编译器自动实现。
相较于其他容器，如vector<>。其利用struct实现，所以其内存占用栈空间，不用在运行时申请地址空间。所以其在构造时没有内存申请和释放的损耗。
overload了完整的逻辑运算，方便对pair进行逻辑运算。
## simple pair implementation
```cpp
/*
 * pair 简单实现
*/ 
template<class T1, class T2>
struct mypair{
    using first_type = T1;
    using second_type = T2;

    first_type first;
    second_type second;

    mypair() 
        :first(first_type()), second(second_type()) {};

    mypair(const first_type& x, const second_type& y)
        :first(x), second(y){
    };    
};
```
## Usage
```cpp

class C1 {
public:
    C1() = delete;                  // 禁用default construct
    C1(int i){};                    // 支持单值构造函数
};

void pairInitialization() {
    // 1、default consturct:
    // 会分别调用 T1, T2的默认构造函数，如果不存在默认构造函数则无法初始化
    std::pair<std::vector<int>, double> tp1;
    // std::pair<C1, int> tp2;            // error:   no default constructor exists for class "std::pair<C1, int>"

    // 2、 二元构造
    std::pair<int, double> tp3(12, 12.5);

    // 3、 异构pair进行初始化,
    // 由于char* -> string  & int -> double 都能进行转换，所以可以实现
    // 此处可以看到其赋值过程中是tp5.first = tp4.first; tp5.first = tp4.first;
    std::pair<const char*, int> tp4("Heng", 13);
    
    std::pair<std::string, double> tp5(tp4);

    // 4、 make_pair 
    // make_pair 根据传递的两个参数的类型决定最终生成的pair类型
    // tp5::type == std::pair<int, double>
    auto tp6 = std::make_pair(12, 12.5);
    
}

void pairAccess() {
    // pair struct 模板定义了两个数据变量，first和second。我们可以直接访问
    std::pair<int, double> tp1(12, 12.5);
    auto sum = tp1.first + tp1.second;

    // std::get()       获取pair中的元素
    // 通过下标获取
    auto first = std::get<0>(tp1);              
    auto second = std::get<0>(tp1);

    // 通过pair元素类型获取
    first = std::get<int>(tp1);         
    second = std::get<double>(tp1);
}

void logicalOperation() {
    // pair 重载了 ==，!=， <, >, <=, >= 6个逻辑运算
    // 以 < 举例， 
    // 如果 lhs.first < rhs.first， 则直接返回true
    // 否则 lhs.first > rhs.first， 则直接返回false
    // 否则 lhs.first == rhs.first, 此时
    //      比较lhs.second < lhs.seond ， 成立返回true， 否则false

    using pt = std::pair<int, std::string>;
    pt p1(12, "A");
    pt p2(11, "A");
    pt p3(12, "B");

    std::vector<pt> vec {p1, p2, p3};
    std::sort(vec.begin(), vec.end());

    for(auto it : vec) {
        std::cout << it.first << "   " << it.second << std::endl;
    }
    /*
     *  output:
     *      11   A
     *      12   A
     *      12   B
    */
}
```