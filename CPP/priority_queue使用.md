---
title: priority_queue使用
categories: CPP
tags:
 - CPP
---



## priority_queue分析

priority_queue是容器适配器，并不是容器。其并没有单独实现任何容器相关功能，而是在现有容器的基础上进行了再封装而已。
参照如下源代码我们不难看出，其实现依靠了algorithm库相关与heap的操作
```cpp
	// stl_queue.h   push方法
      void
      push(value_type&& __x)
      {
	c.push_back(std::move(__x));
	std::push_heap(c.begin(), c.end(), comp);
      }
```
在源码中我们还能发现，其包含如下定义
```cpp
  template<typename _Tp, typename _Sequence = vector<_Tp>,
	   typename _Compare  = less<typename _Sequence::value_type> >
    class priority_queue
    {
    ....
protected:
      //  See queue::c for notes on these names.
      _Sequence  c;
      _Compare   comp;
      }
    ....
```
从中可以看出，其默认使用vector<>作为其内部存储数据的容器，而传递的函数默认使用std::less<>类型。
让我们接着看一下std::less的定义，可以看出其就是一个结构体，定义了operator()，因此可以作为函数对象进行传递。而其底层实现仅是调用了类型变量之间的operator<。
```cpp
  template<typename _Tp>
    struct less : public binary_function<_Tp, _Tp, bool>
    {
      _GLIBCXX14_CONSTEXPR
      bool
      operator()(const _Tp& __x, const _Tp& __y) const
      { return __x < __y; }
    };
```
## 使用方法
* 初始化
```cpp
void priorityQueueInitialization() {

    /* 
    * 模板原型 
    * template<
    *   class T,
    *   class Container = std::vector<T>,
    *   class Compare = std::less<typename Container::value_type>
    * > class priority_queue; 
    * 
    * T ： 操作的数据类型
    * Container： 使用的容器
    * Compare：   函数对象的类型（class， 函数指针， Lambda表达式），其必须满足Compare要求
    * 
    * 对于Compare，个人感觉是一个历史遗留问题，function之后我们明显可以在构造的时候不传入这个参数。
    */

    // 使用简单类型
    std::vector<int> vec {1, 2, 3, 4, 5, 6};
    std::vector<int> vv{vec.begin(), vec.end()};
    std::priority_queue<int> q1();                           // default construct
    std::priority_queue<int> q2(std::greater<int>);          // 初始化compare 函数
    std::priority_queue<int> q3(std::less<int>(), vec);      // 使用vec的内容初始化priority队列

    std::priority_queue<int> q4(vec.begin(), vec.end());     
    std::priority_queue<int, std::vector<int>, std::greater<int>> q5(vec.begin(), vec.end(), std::greater<int>(), vv);
}
```
* 常规使用方法
```cpp
bool operator<(const std::pair<size_t, std::string>& lhs, const std::pair<size_t, std::string>& rhs) {
    return lhs.first > rhs.first;
}


// 利用全特化，我们特化less模板来达到目的
namespace std{

template<>
struct less<std::pair<size_t, std::string>> 
{
    bool operator()(const std::pair<size_t, std::string>& lhs, const std::pair<size_t, std::string>& rhs) {
        return lhs.first > rhs.first;
    };
};

}



void priorityQueueUsageMethod() {

    using priorityThreadPair = std::pair<size_t, std::string>;      // thread_priority 、name
    auto priorityThreadPairComp1 = [](const priorityThreadPair &lhs, const priorityThreadPair &rhs) ->bool {
                                        return lhs.first > rhs.first;
                                    };

    struct PriorityThreadPairComp2 {
        bool operator() (const priorityThreadPair &lhs, const priorityThreadPair &rhs) {
            return lhs.first > rhs.first;
        }
    }priorityThreadPairComp2;

    std::priority_queue<priorityThreadPair, std::vector<priorityThreadPair>, decltype(priorityThreadPairComp1)> q1(priorityThreadPairComp1);
    
    // 这里也可以传递函数对象,只要其支持函数操作
    std::priority_queue<priorityThreadPair, std::vector<priorityThreadPair>, PriorityThreadPairComp2> q1(priorityThreadPairComp2);

    /*
     * 这里相当于 std::priority_queue<priorityThreadPair, std::vector<priorityThreadPair>, std::less<priorityThreadPair>> q1;
     * 其中less的源代码如下：
                template<typename _Tp>
                struct less : public binary_function<_Tp, _Tp, bool>
                {
                _GLIBCXX14_CONSTEXPR
                bool
                operator()(const _Tp& __x, const _Tp& __y) const
                { return __x < __y; }
                };
        发现起始就是相当于调用了operator < ，所以我们可以直接重载这个操作符。
    */

    /*
     * 我们还可以利用全特化的方式来达到目的
    */
    std::priority_queue<priorityThreadPair> q1;


    q1.push({12, "t1"});
    q1.push({100, "t2"});
    q1.push({25, "t3"});
    while(!q1.empty()) {
        const auto& p = q1.top();
        std::cout << p.first << "   " << p.second << std::endl;
        q1.pop();
    }    
}

```