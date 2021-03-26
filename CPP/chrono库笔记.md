---
title: chrono库笔记
categories: CPP
tags: 
 - CPP
---



`chrono`库主要包含clocks、time point、duration三个主要类型。

## duration
duration为一个模板类，表示时间间隔，其定义如下：
`template<class Rep, class Period = std::ratio<1>>`
Period表示一个时钟滴答周期，每当经过一个Period秒时，其Rep类型的滴答总数就会执行`count + 1`。在日常使用中`Rep`可以为整数或者浮点数，Period使用标准库中的分数模板`ratio`进行表示。
* duration初始化
```cpp
void duration_initialization() {
    /* 使用默认构造函数构造*/
    std::chrono::duration<int32_t, std::ratio<1, 1000>> default_duration_milliseconds;         // 其初始化值未定义，随生成环境而定

    /* period count = 1000*/
    std::chrono::duration<int32_t, std::ratio<1, 1000>> duration_milliseconds(1000);        // 1000 ms

    /* double 类型的pre*/
    std::chrono::duration<double> double_duration_seconds(3.5);                             // 3.5s

    /* 使用预定义类型，进行实例化*/
    std::chrono::milliseconds duration_predefine_milliseconds(1000);                        // 1000ms

    /* 使用其他类型的duration进行构造, 要求from_duration 可以隐式转化至 to_duration*/ 
    std::chrono::microseconds t1_duration_microseseconds = duration_milliseconds;           // 1000 ms
    std::chrono::microseconds t2_duration_microseseconds(duration_milliseconds);            // 1000 ms

    /* 使用用户定义字面值来进行初始化*/
    using namespace std::chrono;
    auto user_define_literals = 1000ms;                                                     //1000 ms
};
}
```
* duration数据访问
```cpp

void data_access() {
    using namespace std::chrono;
    seconds duration_seconds(12);                                                           // 12s

    std::cout << "duration represent   " 
              << duration_seconds.count() << "s" << std::endl;                              //  duration represent 12s

    auto max_duration_seconds = seconds::max();                                             // 获取seconds所能表示的最大duration

    std::cout << "maximum seconds duration    "
              << max_duration_seconds.count() << std::endl;                                 // maximum seconds duration    9223372036854775807

    auto min_duration_seconds = seconds::min();                                             // 获取seconds所能表示的最小duration
    std::cout << "minimum seconds duration    "
              << min_duration_seconds.count() << std::endl;                                 // minimum seconds duration    -9223372036854775808
    
/**  这里我们查看seconds的定义为 
*       typedef duration<int64_t> 		    seconds;
*       duration的max和min实质是为int64_t的max和min
*   从这个例子中我们也可以看到duration可以为负值
*/
    std::cout << "INT64_MAX" << INT64_MAX << "   " 
              << "INT64_MIN" << INT64_MIN << std::endl;                                     //INT64_MAX9223372036854775807   INT64_MIN-9223372036854775808
};
```
* duration转化规则
当两者duration之间都为整数并且源period可被目标period整除时，或者目标duration为浮点数时可以使用传统类型转化或者隐式调用其单值构造函数，不必调用`duration_cast`。
```cpp
void typeConversions() {
/*
 * 对于duration之间的类型转换来说，其存在两种转化情况
 *  1、 转化过程中数据完整
 *  2、 转化过程中数据发生部分丢失
*/

/**
 * 转换过程中数据完整时的转换
 * 
 * duration 包含如下构造函数
 *          template< class Rep2, class Period2 >
 *          constexpr duration( const duration<Rep2,Period2>& d );
 * 我们可以看到这是一个单值构造函数，按照C++语言特性，此函数作为转换构造函数，在类型转化时使用。
 * 这个构造函数为防止数据截断， 仅仅会在如下两种情况下执行转化：
 *  1、
 *     目标duration的Rep类型为浮点数 
 *  2、
 *     源duration的Rep类型为整数时，其源Period 可整除 目标Period
*/

/*  允许实例 */
/*1、 目标duration为浮点数*/
    std::chrono::milliseconds int_milliseconds(1024);       // 1024ms
    std::chrono::microseconds int_microseconds(1024);       // 1024us

    std::chrono::duration<double> double_second = int_milliseconds;             // 1.024s = 1024ms      
                                  double_second = int_microseconds;             // 0.001024s = 1024us
                                  double_second = std::chrono::duration<double, std::micro>(1024.1024);           // 0.0010241024s = 1024.1024us
/*2、 源duration为整数，源Period 可整除 目标Period*/
    int_milliseconds = std::chrono::seconds(1024);          //   1024000ms = 1024s
    int_microseconds = int_milliseconds;                    //   1024000000us = 1024000ms

/*不允许实例   (允许以外的都是不允许(狗头))*/
/*1、 源duration为浮点数， 目标duration不为浮点数*/
    // 此实例按照我们常人的逻辑是可以进行的    1024102400us = 1024.1024s，这里为了保证正确性，就直接把这种转换禁止掉了
    // std::chrono::microseconds t1 = std::chrono::duration<double>(1024.1024);
/*2、 源duration 和 目标duration的Rep都为整数，但是两者Period不匹配*/
    // second period == 1, microseconds period == 1/1000000，显然 1/1000000 除以 1 不能整除 
    // std::chrono::seconds t1 = std::chrono::microseconds(1024);

/*
 * 数据会发生截断时的转化
 * chrono库提供了duration之间相互转化的函数，其定义如下
 *      template <class ToDuration, class Rep, class Period>
 *      constexpr ToDuration duration_cast(const duration<Rep,Period>& d);
*/
    std::chrono::seconds t1 = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(1024));               // 1s = 1024ms(精度损失)
    std::chrono::seconds t2 = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<double>(1.024));          // 1s = 1.024s 
}
```
* duration计算
duration提供了相当丰富的计算![在这里插入图片描述](https://img-blog.csdnimg.cn/20200716171647328.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzM3NjU0NzA0,size_16,color_FFFFFF,t_70)
```cpp
void duration_calculation() {
    /*
     *  对于二元运算操作符（+ - * / %）duration的所有算数过程可以拆分成如下步骤
     *  1、 将两侧操作数转化成共同类型的duration
     *  2、 对转化后的duration进行count的计算
     *  3、 返回这个共同类型
    */
    
    // 
    auto t1 = std::chrono::seconds(1) + std::chrono::milliseconds(10); 
    std::cout << "t1 type is "<< (bool)(typeid(t1) == typeid(std::chrono::milliseconds)) << std:: endl;
    std::cout << "t1 count " << t1.count() << std::endl;

    
    /* std::ratio<1, 3> 和 std::ratio<1, 4>>(1) 的 common type 为 std::ratio<1, 12>, 即最大公因子*/
    /* uint64_t 和 uint64_t的common type为 uint64_t */
    auto t2 = std::chrono::duration<uint64_t, std::ratio<1, 3>>(1) + std::chrono::duration<uint64_t, std::ratio<1, 4>>(1);
    std::cout << "t2 type is "<< (bool)(typeid(t2) == typeid(std::chrono::duration<uint64_t, std::ratio<1, 12>>)) << std:: endl;
    std::cout << "t2 count " << t2.count() << std::endl;         

    /* std::ratio<1, 4> 和 std::ratio<1, 6>>(1) 的 common type 为 std::ratio<1, 12>, 即最大公因子*/
    /* double 和 uint64_t的common type为 double */
    auto t3 = std::chrono::duration<double, std::ratio<1, 4>>(1) + std::chrono::duration<uint64_t, std::ratio<1, 6>>(1);
    std::cout << "t3 type is "<< (bool)(typeid(t3) == typeid(std::chrono::duration<double, std::ratio<1, 12>>)) << std:: endl;
    std::cout << "t3 count " << t3.count() << std::endl;
}   
```
* common_type
从前面的例子中，我们可以看到对duration之间的计算需要将其转化为common_type。chrono库为我们提供了方便地struct模板让我们能够轻松获取到两个duration的common type
```cpp
void common_type() {
    using commonType = std::common_type<std::chrono::duration<double, std::ratio<1, 4>>, std::chrono::duration<uint64_t, std::ratio<1, 6>>>::type;
    commonType t = std::chrono::duration<double, std::ratio<1, 4>>(1) + std::chrono::duration<uint64_t, std::ratio<1, 6>>(1);       // 可以编译通过，说明类型一致
    std::cout << "commonType type is "<< (bool)(typeid(commonType) == typeid(std::chrono::duration<double, std::ratio<1, 12>>)) << std:: endl;
}
```
## time_point
chrono库对于一个时间点来说，其采用如下表示方法。一个起始时间epoch 和 时间长度 duration。类似于Unix中的时间表示方法。在Unix中，基础时间点为1970/1/1 00:00:00。之后所有时间点都是time秒 + 基础时间点。
chrono库在这基础上的改进就是将time秒变为了duration。duration有更为丰富可变的时钟周期period，它能够表示更为精准的时间点或者粗略的时间点，这一切全部由duration进行控制。
* time_point 构造
```cpp
void initialization_time_point() {

    // 默认构造函数，创建一个time_poin代表时钟的起始时间(Clock'epoch)
    std::chrono::time_point<std::chrono::system_clock> tp1();

    // 创建一个time_point, 其time_point = Clock'epoch + duration
    // 调用条件为传入duration可以转换到time_point的duration类型
    std::chrono::time_point<std::chrono::system_clock> tp2(std::chrono::hours(24));

    // 创建一个time_point， 其值为从time_point 的拷贝
    // 从time_point1 可以转换到 time_point2 需要要求 time_point1.duration 可以向 time_point2.duration进行转化
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> tp3(std::chrono::hours(24));
    std::chrono::time_point<std::chrono::system_clock> tp4(std::chrono::hours(tp3));
}
```
* time_point 数据访问
```cpp
void time_point_since_epoch() {
    /*
     * 由此实例我们可以得出如下结论
    */
    auto tp0 = std::chrono::time_point<std::chrono::system_clock>();
    auto tp1 = std::chrono::time_point<std::chrono::system_clock>(std::chrono::hours(24));
    auto tp2 = tp0 - std::chrono::hours(24);
    
    /*其返回的duration 为实例化template 时传入的duration*/
    std::chrono::hours tp0_since_epoch_hours = std::chrono::duration_cast<std::chrono::hours>(tp0.time_since_epoch());          // 0 h
    std::chrono::hours tp1_since_epoch_hours = std::chrono::duration_cast<std::chrono::hours>(tp1.time_since_epoch());          // 24 h
    std::chrono::hours tp2_since_epoch_hours = std::chrono::duration_cast<std::chrono::hours>(tp2.time_since_epoch());          // -24h

    /*打印epoch时间*/
    std::time_t tm = std::chrono::system_clock::to_time_t(tp0);         
    std::cout << std::ctime(&tm);                   // Thu Jan  1 08:00:00 1970
}
```
* time_point 转化规则
```cpp
void time_point_type_conversions() {
    // 对于任意两个时间点来说，对于其的任何操作都需要一个前提，即两个time_point构造时使用同一个clock
    // 使用同一个clock意味着两个time_point参照同一个epoch
    // 如果两个time_point的epoch不相等，那么我们对于其任何的计算都是无意义的
    
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp1;
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::milliseconds> tp2;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> tp3;
    // tp1 == tp2;         // 可以通过编译， 这里是因为系统库 using high_resolution_clock = system_clock;
    // tp1 == tp3;         // 编译错误

    /*
     * time_point 使用duration来计算相较于epoch的时间
     * 那么不同的duration，将会导致时间点的精度不一样
     * std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> 的时间点肯定相较于  
     * std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>的时间点来说更为精确
     * 所以同duration一样，当 duration不能够进行隐式类型转化时  需要进行类型转化
     *                       duration可以进行隐式类型转化时    可直接使用单值构造函数进行隐式类型转化
     * 其中精度判断准则与duration的判断准则
    */
    using namespace std::chrono;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> tp_seconds(100s);
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp_milliseconds;
    tp_milliseconds = tp_seconds;                                                               // 可以
    // tp_seconds = tp_milliseconds;                                                            // 不可以
    tp_seconds = std::chrono::time_point_cast<std::chrono::seconds>(tp_milliseconds);           // 需要进行类型转化
}
```
* time_point的相关计算
```cpp
void time_point_calculate() {
    using namespace std::chrono;
    // 包含如下计算
    // time_point = time_point + duration
    // time_point = time_point - duration
    // time_point = duration + time_point
    // 其中返回的time_point的duration类型为 common_type<time_point::duration, duration> 
    auto tp = time_point<system_clock, duration<uint64_t, std::ratio<1, 3>>>() + duration<uint64_t, std::ratio<1, 4>>(1);
    if(typeid(tp) == typeid(time_point<system_clock, duration<uint64_t, std::ratio<1, 12>>>));           // True;

    // duration = time_point1 - time_point2
    // duration类型为 common_type<time_point1::duration, time_point2::duration> 
    auto tp1 = time_point<system_clock, duration<uint64_t, std::ratio<1, 3>>>(seconds(2));
    auto tp2 = time_point<system_clock, duration<uint64_t, std::ratio<1, 4>>>(seconds(3));
    auto dur = tp1 - tp2;
    if(typeid(dur) == typeid(duration<uint64_t, std::ratio<1, 12>>));                                    // True;
    using co_type = std::common_type<time_point<system_clock, duration<uint64_t, std::ratio<1, 3>>>, time_point<system_clock, duration<uint64_t, std::ratio<1, 4>>>>::type;
    if(typeid(co_type) == typeid(time_point<system_clock, std::common_type<duration<uint64_t, std::ratio<1, 3>>, duration<uint64_t, std::ratio<1, 4>>>::type>));        // true                       // True;
}
```
## clock
clock由一个起始时间点epoch和时钟周期tick rate组成。
chrono库为我们预定义了3个时钟类型：
* system_clock
system_clock代表系统时间，表示与操作系统时间同步的时钟。因为系统时间可以在任意时刻被更改，所以此时钟并不单调。
```cpp

void clock_system_clock() {
    std::chrono::system_clock::time_point np = std::chrono::system_clock::now();                        // 获取当前系统时间点
    
    if(std::chrono::system_clock::is_steady);                                                           // 判断此时钟是否单调

    std::time_t tm = std::chrono::system_clock::to_time_t(np);                                          // time_point --> time_t
    std::cout << std::ctime(&tm);

    using namespace std::chrono;
    std::this_thread::sleep_for(10s);                                                                   // 主线程睡眠10s
    tm = time(nullptr);                                                                                 
    np = std::chrono::system_clock::from_time_t(tm);                                                    // time_t --> time_point
    std::time_t temp = std::chrono::system_clock::to_time_t(np);                                          // time_point没办法打印， 只能依靠现有的C库进行，C++20有所改进
    std::cout << std::ctime(&temp);                                                                       // 
}
```
* steady_clock
steady_clock仅包含now成员方法，代表一个单增时钟，其时钟的时间点不可能减少，因为物理时间只能向前移动并且每次时钟跳动间隔固定。这个时钟与系统时间无关（此事件可以为，距离上次重启系统所经历的事件），其非常适合用于计算时间间隔。
* high_resolution_clock
g++中源代码如下，
```cpp
 /**
     *  @brief Highest-resolution clock
     *
     *  This is the clock "with the shortest tick period." Alias to
     *  std::system_clock until higher-than-nanosecond definitions
     *  become feasible.
    */
    using high_resolution_clock = system_clock;
```