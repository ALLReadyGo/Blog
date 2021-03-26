---
title: next_permutation，prev_permutation源码分析
categories: CPP
tags: 
 - CPP
---



## next_permutaion

#### 伪代码
伪代码描述如下，这里只阐述主要步骤，不对异常进行处理
```cpp
// last 表示 A.size() - 1
// 1： 从后向前遍历，获取第一个非递增点
for i -> last - 1 to 0
	if A[i] < A[i + 1]  break	 

// 2：从递增序列中找到第一个必A[i]大的元素
for j -> last to i + 1
	if A[j] > A[i] break

// 3、交换
swap(A[i], A[j])
// 4、逆序
reverse(A, i + 1, last);
```

#### 实例分析
通过实例对伪代码进行解释：

对字符串`2431`执行算法：
按照注释中的标号，算法步骤被分为三部分，

**1、从后向前遍历，获取非递增点**
此时我们定位元素2，因为 4，3，1从右向左为递增序列

**2、从递增序列中找到第一个比A[i]大的元素**
我们要知道，2之后的元素是一个右向左的递增序列，此时我们从右侧开始查找，找到的第一个比A[i]大的元素是查找序列中比A[i]大的所有元素中的最小元素，此时我们定位3

**3、交换**
交换元素2 3，此时字符串变为 `3421`。对于原始待逆序序列`431`来说，其通过交换变为`421`。我们可以发现其仍保持递增特性，因为我们交换的节点为比A[i]大的所有元素中的最小元素，其顺序不会发生改变。

**4、逆序**
字符串通过交换变为`3421`，我们需要逆序[i + 1, last]的所有元素，即`421`，此时字符串变为`3124`，此结果变为字符串的下一个字典顺序。

#### STL源码
```cpp

  /**
   *  @brief  Permute range into the next @e dictionary ordering.
   *  @ingroup sorting_algorithms
   *  @param  __first  Start of range.
   *  @param  __last   End of range.
   *  @return  False if wrapped to first permutation, true otherwise.
   *
   *  Treats all permutations of the range as a set of @e dictionary sorted
   *  sequences.  Permutes the current sequence into the next one of this set.
   *  Returns true if there are more sequences to generate.  If the sequence
   *  is the largest of the set, the smallest is generated and false returned.
  */
  template<typename _BidirectionalIterator>
    inline bool
    next_permutation(_BidirectionalIterator __first,
		     _BidirectionalIterator __last)
    {
      // concept requirements
      __glibcxx_function_requires(_BidirectionalIteratorConcept<
				  _BidirectionalIterator>)
      __glibcxx_function_requires(_LessThanComparableConcept<
	    typename iterator_traits<_BidirectionalIterator>::value_type>)
      __glibcxx_requires_valid_range(__first, __last);
      __glibcxx_requires_irreflexive(__first, __last);

      return std::__next_permutation
	(__first, __last, __gnu_cxx::__ops::__iter_less_iter());
    }



  template<typename _BidirectionalIterator, typename _Compare>
    bool
    __next_permutation(_BidirectionalIterator __first,
		       _BidirectionalIterator __last, _Compare __comp)
    {
      if (__first == __last)                      // 传入内容为空，直接返回false
	return false;
      _BidirectionalIterator __i = __first;       // 仅要求双向迭代器
      ++__i;                                      
      if (__i == __last)                          // 仅包含一个元素，返回false
	return false;
      __i = __last;
      --__i;                                      // __i 指向最后一个元素

      for(;;)
	{
	  _BidirectionalIterator __ii = __i;            //  
	  --__i;										// __i + 1 == __i , 这两个迭代器负责找到第一个非递增点
	  if (__comp(__i, __ii))						// 获得非递增点
	    {
	      _BidirectionalIterator __j = __last;      
	      while (!__comp(__i, --__j))               // 获取第一个比 __i 大的元素
		{}
	      std::iter_swap(__i, __j);					// 交换
	      std::__reverse(__ii, __last,				// 逆序
			     std::__iterator_category(__first));
	      return true;
	    }
	  if (__i == __first)							// 查找到了first， 此时所有元素全部递增，所以此时序列最大
	    {
	      std::__reverse(__first, __last,
			     std::__iterator_category(__first)); // 最大序列逆序，会成为最小序列
	      return false;
	    }
	}
    }
```

## prev_permutation
#### 实例
我们以`3124`进行分析，看能否恢复至`2431`

**1、从后向前遍历，获取非递减点**
此时我们定位元素3，因为 1，2，4从右向左为递减序列

**2、从递增序列中找到第一个比A[i]小的元素**
此时我们定位`2`

**3、交换**
交换元素2 3，此时字符串变为 `2134`。

**4、逆序**
逆序[i + 1, last]的所有元素，即`134`，此时字符串变为`2431`。

#### STL源码
```cpp
  /**
   *  @brief  Permute range into the previous @e dictionary ordering.
   *  @ingroup sorting_algorithms
   *  @param  __first  Start of range.
   *  @param  __last   End of range.
   *  @return  False if wrapped to last permutation, true otherwise.
   *
   *  Treats all permutations of the range as a set of @e dictionary sorted
   *  sequences.  Permutes the current sequence into the previous one of this
   *  set.  Returns true if there are more sequences to generate.  If the
   *  sequence is the smallest of the set, the largest is generated and false
   *  returned.
  */
  template<typename _BidirectionalIterator>
    inline bool
    prev_permutation(_BidirectionalIterator __first,
		     _BidirectionalIterator __last)
    {
      // concept requirements
      __glibcxx_function_requires(_BidirectionalIteratorConcept<
				  _BidirectionalIterator>)
      __glibcxx_function_requires(_LessThanComparableConcept<
	    typename iterator_traits<_BidirectionalIterator>::value_type>)
      __glibcxx_requires_valid_range(__first, __last);
      __glibcxx_requires_irreflexive(__first, __last);

      return std::__prev_permutation(__first, __last,
				     __gnu_cxx::__ops::__iter_less_iter());
    }

  template<typename _BidirectionalIterator, typename _Compare>
    bool
    __prev_permutation(_BidirectionalIterator __first,
		       _BidirectionalIterator __last, _Compare __comp)
    {
      if (__first == __last)
	return false;
      _BidirectionalIterator __i = __first;
      ++__i;
      if (__i == __last)
	return false;
      __i = __last;
      --__i;

      for(;;)
	{
	  _BidirectionalIterator __ii = __i;				// 依然从右向左搜寻
	  --__i;
	  if (__comp(__ii, __i))							// __ii 与 __i的位置与next_permutation中不一样，此时查找第一个递减断点
	    {
	      _BidirectionalIterator __j = __last;			
	      while (!__comp(--__j, __i))					// 查找第一个比其小的元素
		{}
	      std::iter_swap(__i, __j);						
	      std::__reverse(__ii, __last,
			     std::__iterator_category(__first));
	      return true;
	    }
	  if (__i == __first)
	    {
	      std::__reverse(__first, __last,
			     std::__iterator_category(__first));
	      return false;
	    }
	}
    }
```