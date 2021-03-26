---
title: ziplist分析
categories: redis
tags:
 - redis
---



这里直接贴`ziplist.c`中的作者的注释，从注释中我们可以直观的看到`ziplist`是用于存储string字符串的双向链表，其目的是更好地利用内存去存储数据，减少无用数据的比例。

```cpp
/* The ziplist is a specially encoded dually linked list that is designed
 * to be very memory efficient. It stores both strings and integer values,
 * where integers are encoded as actual integers instead of a series of
 * characters. It allows push and pop operations on either side of the list
 * in O(1) time. However, because every operation requires a reallocation of
 * the memory used by the ziplist, the actual complexity is related to the
 * amount of memory used by the ziplist.
 * 
 * ziplist是一个特殊编码的的双链表，他被设计成能够高效利用内存。它能够存储字符串和整数，其中
 * 整数被编码为真正的数字而不是一连串的字符。他允许在O(1)时间内对齐从两边进行push、pop操作。
 * 然而，因为每个操作需要内存的重新分配，这个真实的复杂度依赖于ziplist总计使用的内存数。
 * ----------------------------------------------------------------------------
 *
 * ZIPLIST OVERALL LAYOUT:
 * The general layout of the ziplist is as follows:
 * <zlbytes><zltail><zllen><entry><entry><zlend>
 * 
 * <zlbytes> is an unsigned integer to hold the number of bytes that the
 * ziplist occupies. This value needs to be stored to be able to resize the
 * entire structure without the need to traverse it first.
 * <zlbytes> 是一个存储ziplist占用字节数量的无符号整数。这个值需要存储，
 * 从而当想要resize整个结构时，我们不必先遍历他
 * 
 * <zltail> is the offset to the last entry in the list. This allows a pop
 * operation on the far side of the list without the need for full traversal.
 * <zltail> 是距离链表最后一个条目的偏移量。这样可以不用整个遍历而对远端进行一次pop操作。
 * 
 * <zllen> is the number of entries.When this value is larger than 2**16-2,
 * we need to traverse the entire list to know how many items it holds.
 * <zllen>是条目数量。当这个值超过2^16 - 2，我们需要全部遍历整个列表才能知道他有多少项
 * 
 * <zlend> is a single byte special value, equal to 255, which indicates the
 * end of the list.
 * <zelend>是一个等于255的特殊字节，它指示链表的结束
 * 
 * ZIPLIST ENTRIES:
 * ZIP列表条目
 * Every entry in the ziplist is prefixed by a header that contains two pieces
 * of information. First, the length of the previous entry is stored to be
 * able to traverse the list from back to front. Second, the encoding with an
 * optional string length of the entry itself is stored.
 * 每个ziplist中的条目都有一个头部前缀，这个前缀包含两部分信息。首先，存储上一个
 * 条目的长度使其能够从后向前遍历。第二，编码和一个可选的自身存储的条目的字符串长度。
 * 
 * The length of the previous entry is encoded in the following way:
 * If this length is smaller than 254 bytes, it will only consume a single
 * byte that takes the length as value. When the length is greater than or
 * equal to 254, it will consume 5 bytes. The first byte is set to 254 to
 * indicate a larger value is following. The remaining 4 bytes take the
 * length of the previous entry as value.
 * （第一部分）
 * 前一个条目的长度使用以下方式进行编码：
 * 如果这个长度小于254字节，他会仅占用一个字节，将长度设置为其值
 * 当这个长度超过或者等于254时，他会占用5个字节。
 * 第一个字节会被设置为254去指明其后跟随了一个更大的数字。剩余的4字节将上一个字节的长度作为其值。
 * 
 * The other header field of the entry itself depends on the contents of the
 * entry. When the entry is a string, the first 2 bits of this header will hold
 * the type of encoding used to store the length of the string, followed by the
 * actual length of the string. When the entry is an integer the first 2 bits
 * are both set to 1. The following 2 bits are used to specify what kind of
 * integer will be stored after this header. An overview of the different
 * types and encodings is as follows:
 * 
 * （第二部分）
 * 其余的头部领域依赖于条目内容。当这个条目是一个string时，头部的前两个bits
 * 会保存存储字符串长度的编码类型，其后跟随的为字符串的实际长度。
 * 当这个条目是一个整数时，前两个bit均会设置为1.接下来的两个bit会被用于指明什么类型的整数
 * 会被存储。
 * 一个不同的类型和编码概述如下：
 * |00pppppp| - 1 byte
 *      String value with length less than or equal to 63 bytes (6 bits).
 *      只使用6bit去指明字符串的长度
 * |01pppppp|qqqqqqqq| - 2 bytes
 *      String value with length less than or equal to 16383 bytes (14 bits).
 *      使用14bits去存储字符串长度，最高到达16383长度
 * |10______|qqqqqqqq|rrrrrrrr|ssssssss|tttttttt| - 5 bytes
 *      String value with length greater than or equal to 16384 bytes.
 *      使用4bytes去存储这个字符串，2^32能够表达4G的容量，所以只要大于16384bytes的
 *      字符串都将使用这种方式进行操作
 * 
 * 接下来的的条目表示整数
 * |11000000| - 1 byte
 *      Integer encoded as int16_t (2 bytes).
 *      00 指明整数编码为int16_t
 * |11010000| - 1 byte
 *      Integer encoded as int32_t (4 bytes).
 *      01
 * |11100000| - 1 byte
 *      Integer encoded as int64_t (8 bytes).
 *      10 整数编码为64_t
 * |11110000| - 1 byte
 *      Integer encoded as 24 bit signed (3 bytes).
 *      11 整数编码为24byte的符号整数
 * |11111110| - 1 byte  // 1 字节编码数字这么奇怪的原因在后面
 *      Integer encoded as 8 bit signed (1 byte).
 *      111110  8bit符号整数
 * |1111xxxx| - (with xxxx between 0000 and 1101) immediate 4 bit integer.
 *      Unsigned integer from 0 to 12. The encoded value is actually from
 *      1 to 13 because 0000 and 1111 can not be used, so 1 should be
 *      subtracted from the encoded 4 bit value to obtain the right value.
 *      1111xxxx 编码的，我们使用最后的xxxx表示4bit整数，由于我们没办法使用1111,1111、
 *      1111,1110 和 1111,0000这三个数字，所以我们只能表示1到13。我们想要0-12就要将最后结果值减1；
 * |11111111| - End of ziplist.
 */

```
这里我分析几个关键问题，这些问题在源代码中作者也有介绍。
#### 其高效利用内存是如何实现的
 其实高效的意思就是我们要存储更多的有效数据，避免对无效数据的存储。例如传统的链表，如果我们仅仅存储一个`int`，结构体的定义一般为
```cpp
struct node{
 	int val;
 	struct node *next;
 }
```
这个结构体中有效数据仅为50%，因为另一半数据我们必须拿去维护链表结构。所以对于存储小型数据来说，链表结构就相对性价比较低。
除此之外还有一种浪费就是，假设我们存储数据`{123, 12, 41, 31}`，我们使用`int(4 bytes)`去存储，但是实际上我们可以仅使用`char(1 bytes)`就可以完成这些数据的存储。这将节省下将近四倍的空间。如果存储的数据中大多是这种`small int`，那么选取合适的`int`是完全能够达到降低内存使用的目标的。

`ziplist`要求占用一个连续的内存空间，每个数据存储在每个`entry`中。为了满足entry不定长的需求，我们需要在当前节点中记录自身节点的大小和前一个节点的大小(实现双向链表)。那么这两个`int`我们也需要选择合适的大小去存储，否则我们还是会陷入为了维护结构而占用过多存储比例的结构。（数组就是一个存储率100%的数据结构，应为他的结构是通过其内存位置关系来维护，没有代价）。

#### 通过时间换取空间，其操作更为复杂
由于我们对于每个entry都进行了编码操作，例如我们查看`prelen`，我们需要先去判断他是采用了`5 bytes`存储还是`1bytes`存储，如果采用了`1bytes`存储，那么其值为`p[0]`否则为`p[1:4]`。
并且我们如果想要获取`len`，在未获取`prelen`的具体存储情况下根本无法获得，因为我们无法得知`len`的起始位置。
由于`ziplist`要求必须存储在一个连续的内存空间当中，所以当我们对其进行插入或者删除时，我们必须对其进行`zrealloc`和` memcpy`。并且其操作还需要像链表那样只能挨个寻找，无法实现随即查询。

#### 结构的波动问题
我们假设存储的每个`entry`为253 bytes。这样的`entrys`一共有100多个。那么我们来看一个问题：
如果此时我们在头部插入一个500 bytes的`entry`。当元素插入成功后，我们需要修改`next entry`的`prelen`，此时我们可以看到原先的`1 bytes prelen`无法存储下500，那么我们需要将其夸大至`5 bytes`(既然面对扩容，我们就必须调用`zrealloc`和` memcpy`)。然而这还没完，当我们对`next entry`进行扩容完毕时，`next next entry`的`prelen`对应值将不再是`253`而应是`257`，我们还需要对`next next entry`进行扩容以让其`prelen`能够容纳下`257`，以此类推我们将会执行100次`zrealloc`和`memcpy`，而这一切都是为了完成一次插入。
作者在源代码中是这样描述这个问题的
```cpp
/* When an entry is inserted, we need to set the prevlen field of the next
 * entry to equal the length of the inserted entry. It can occur that this
 * length cannot be encoded in 1 byte and the next entry needs to be grow
 * a bit larger to hold the 5-byte encoded prevlen. This can be done for free,
 * because this only happens when an entry is already being inserted (which
 * causes a realloc and memmove). However, encoding the prevlen may require
 * that this entry is grown as well. This effect may cascade throughout
 * the ziplist when there are consecutive entries with a size close to
 * ZIP_BIGLEN, so we need to check that the prevlen can be encoded in every
 * consecutive entry.
 *
 * Note that this effect can also happen in reverse, where the bytes required
 * to encode the prevlen field can shrink. This effect is deliberately ignored,
 * because it can cause a "flapping" effect where a chain prevlen fields is
 * first grown and then shrunk again after consecutive inserts. Rather, the
 * field is allowed to stay larger than necessary, because a large prevlen
 * field implies the ziplist is holding large entries anyway.
 *
 * The pointer "p" points to the first entry that does NOT need to be
 * updated, i.e. consecutive fields MAY need an update. */

/* 当一个entry被插入时，我们需要将next entry的prevlen设置为插入的entry。
 * 这可能会导致当前entry的长度无法被编码为1bytes长度，而next entry需要增长从而存储5 bytes编码的prelen
 * 这个可以不用担心，因为这个仅发生在entry已经被插入时（插入时将会导致realloc 和 memmove）。
 * 然而，编码这个prevlen可能需要此entry同样增长。
 * 当有一连串entries其大小都相近于ZIP_BIGLEN，这个影响可能造成坏的影响对于整个ziplist。
 * 所以我们需要去检查每个prelen是否都能编码进entry
 * 
 * 注意需要编码prevlen的bytes可以收缩，这个影响会造成相反的效果。
 * 这个影响我们故意忽略，因为他会造成一种波动的影响。当一条链发生增长之后可能会由于新的插入导致收缩。
 * 我们宁愿这个区域的大小超过实际需求，因为大的prelen暗指ziplist中存储着大的entries。
*/
static unsigned char *__ziplistCascadeUpdate(unsigned char *zl, unsigned char *p) {
    size_t curlen = intrev32ifbe(ZIPLIST_BYTES(zl)), rawlen, rawlensize;
    size_t offset, noffset, extra;
    unsigned char *np;
    zlentry cur, next;

    while (p[0] != ZIP_END) {
        // cur 为 当前entry的结构体 
        cur = zipEntry(p);
        rawlen = cur.headersize + cur.len;
        rawlensize = zipPrevEncodeLength(NULL,rawlen);

        /* Abort if there is no next entry. */
        if (p[rawlen] == ZIP_END) break;

        // next 为下一个entry的结构体
        next = zipEntry(p+rawlen);

        /* Abort when "prevlen" has not changed. */
        // 当prevlen未发生改变时，我们跳出循环
        if (next.prevrawlen == rawlen) break;

        // 当prevlen发生改变时，我们需要修改他
        if (next.prevrawlensize < rawlensize) {
            /* The "prevlen" field of "next" needs more bytes to hold
             * the raw length of "cur". */
            /* next的 prevlen 需要更多的bytes 去存储 cur 的长度*/
            offset = p-zl;
            extra = rawlensize-next.prevrawlensize;
            zl = ziplistResize(zl,curlen+extra);
            p = zl+offset;

            /* Current pointer and offset for next element. */
            np = p+rawlen;
            noffset = np-zl;

            /* Update tail offset when next element is not the tail element. */
            if ((zl+intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl))) != np) {
                ZIPLIST_TAIL_OFFSET(zl) =
                    intrev32ifbe(intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl))+extra);
            }

            /* Move the tail to the back. */
            memmove(np+rawlensize,
                np+next.prevrawlensize,
                curlen-noffset-next.prevrawlensize-1);
            zipPrevEncodeLength(np,rawlen);

            /* Advance the cursor */
            /* 进一步递归循环，保证整个ziplist正确性 */
            p += rawlen;
            curlen += extra;
        } else {
            if (next.prevrawlensize > rawlensize) {
                /* This would result in shrinking, which we want to avoid.
                 * So, set "rawlen" in the available bytes. */
                // 这个将导致收缩，但是我们想要避免这样。所以我们直接设置rawlen 进入相应bytes中
                zipPrevEncodeLengthForceLarge(p+rawlen,rawlen);
            } else {
                // 刚好容纳地下，我们此时只用改一下大小就可以
                zipPrevEncodeLength(p+rawlen,rawlen);
            }

            /* Stop here, as the raw length of "next" has not changed. */
            break;
        }
    }
    return zl;
}
```