---
title: weak_ptr和shared_ptr总结
categories: CPP
tags:
 - CPP
---



#### 结构分析

weak_ptr和shared_ptr都包含一个_M_refcount数据成员，追溯其定义，内部包含一个_Sp_counted_base<_LP>*  _M_pi。
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200809120618954.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzM3NjU0NzA0,size_16,color_FFFFFF,t_70)
#### shared_ptr
shared_ptr能够实现其功能依赖于对于多个shared_ptr只实例化一个_Sp_counted_base<_Lp>。当我们通过某一shared_ptr初始化另一shared_ptr时，其会执行如下两个步骤
1、this->_M_ptr = other->_M_ptr
2、this->_M_refcount._M_pi指向了other->_M_refcount._M_pi，并且将_M_use_count++，表示对一个实体的引用增加1个。

当shared_ptr析构时，执行如下步骤
1、this->_M_refcount._M_pi->_M_use_count 减1
2、如果this->_M_refcount._M_pi->_M_use_count归零，则析构this->_M_ptr所执行的内存空间。
3、如果this->_M_refcount._M_pi->_M_weak_count归零，则析构this->_M_refcount._M_pi的空间

#### weak_ptr
weak_ptr必须依靠shared_ptr才能进行构造，其构造过程如下：
1、weak->_M_ptr = shared->_M_ptr
2、weak->_M_refcount._M_pi指向了shared->_M_refcount._M_pi，并且_M_weak_count++，表示存在一个weak_ptr引用了此结构

weak_ptr析构时，执行如下步骤
1、this->_M_refcount._M_pi->_M_weak_count 减1
2、当this->_M_refcount._M_pi->_M_weak_count归零时，则析构this->_M_refcount._M_pi的空间。

#### _M_pi分析
此结构就是实体的引用计数，他含有两个计数_M_use_count，其表示shared_ptr的引用指向数目，当创建一个指向此实体的shared_ptr时，_M_use_count +1，当一个shared_ptr析构时，_M_use_count -1。如果一个_M_use_count -1归零时便会释放此实体的内存空间。
但是这个引用计数什么时候释放？不仅只有shared_ptr指向此引用计数，还有weak_ptr指向此引用计数，所以这里又引入了另一个计数_M_weak_count，与shared_ptr相似，创建时+1，析构时-1。仅有当_M_weak_count归零时才会对其进行释放。
这也就是reference说的weak_ptr会延长计数存在时间的原因。