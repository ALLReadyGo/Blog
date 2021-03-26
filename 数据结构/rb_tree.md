---
title: rb_tree
categories: 数据结构
tags: 
 - 数据结构
---



### 红黑树

这里先列出我的学习资料

[红黑树(一)之 原理和算法详细介绍](https://www.cnblogs.com/skywang12345/p/3245399.html)
[红黑树深入剖析及Java实现](https://zhuanlan.zhihu.com/p/24367771)
[红黑树可视化工具](https://www.cs.usfca.edu/~galles/visualization/RedBlack.html)
> 第一篇博客介绍的相当详细，并且理解也很深入，但是其博客中存在部分错误，大家看的时候需要对照着第二篇博客
> 第二篇博客介绍的较少，但是内容正确性有保证
> 学习完概念之后可以利用可视化工具试着进行插入删除




我对于红黑树的理解：
* 不存在父子红节点 and 任何一个节点向下遍历到其子孙的叶子节点，所经过的黑节点个数必须相等
这条性质保证了红黑树左右子树高度只差至多为log(n)。这里从极限状态比较好理解，当左右子树高度只差最大时，就是一个子树上仅存在黑色节点，另一个子树上的节点以红黑交替分布。所以一个子树的高度至多是另一颗子树的一倍。
* 红黑树局部平衡则整体平衡
红黑树的节点插入或者删除都存在打破红黑树平衡的可能，红黑树保持平衡的关键就是优先调整当前子树，如果当前子树平衡则停止调整，如果无法利用当前子树完成调整，则将其传递上层节点进行调整。这里我们需要注意子树平衡要注意一点，不能改变路径中黑节点个数。
* 红黑树插入
红黑树插入被分为了三个状态，其中case 3只是为了转换至case 2。case 2可以完美达成子树平衡（黑色节点个数不变，而不存在连续红节点），而case1将不平衡性向上进行了传递，对于传递后的不平衡点依然可以套用case 1， 2， 3。在处理插入时，不平衡性主要由连续红打破，但是调整时我们需要保证子树高度不变这一要求。
* 红黑树删除
删除是红黑树中最复杂的操作，删除引发的不平衡问题主要是路径中黑色节点个数发生改变。这里我建议大家仔细阅读博客1中对于删除操作的理解，我这里为了表述清除引入fix节点概念。我们都知道红黑树最开始删除时等同于BST删除方法（查找前驱节点进行值替换，再删除前驱节点）。对于删除的前驱节点，其一定不存在右孩子，其为一棵单枝树。假设此节点存在孩子节点，我们称其孩子节点为fix节点，那么此fix节点将会占用被删除节点的位置。如果删除的节点为红色，那么我们不必进行任何调整 ，而如果节点的颜色为黑色，那么fix节点便会处于一种缺少黑色父节点的状态，博客1中将此节点状态成为黑+黑，也就是说如果经过fix节点时黑色节点路径长度+2，那么子树平衡。所以为了让子树平衡，我们必须在fix前插入一个黑色节点。
删除操作中一共存在4中情况：
case 1是为了转换至case 2，3，4中的。case 2将fix节点上移，case 3：为了转至case 4。case 4才能真正使子树平衡，同时case4也标志着调整的结束。

####
花了一天多写的红黑树，留念一下
```cpp
#include <iostream>

using namespace std;

enum RBTColor{RED, BLACK};

struct RBTNode {
    RBTColor color;
    int value;
    RBTNode *parent;
    RBTNode *left, *right;

    RBTNode(int val, RBTColor color, RBTNode *parent, RBTNode *left, RBTNode *right) 
        :value(val), color(color), parent(parent), left(left), right(right){
    };
};

class RBTree {
private:
    RBTNode *m_root;

public:
    RBTree():m_root(nullptr){};
    void insert(int val);
    void remove(int val);
    void print();
};


void RBTree::insert(int val) {
    RBTNode *cur, *fixed;
    if(m_root == nullptr) {
        m_root = new RBTNode(val, BLACK, nullptr, nullptr, nullptr);
        return;
    }
    cur = m_root;

    while(cur != nullptr) {
        if(val < cur->value) {
            if(cur->left == nullptr) {
                cur->left = new RBTNode(val, RED, cur, nullptr, nullptr);
                fixed = cur->left;
                break;
            } else {
                cur = cur->left;
            }
        }else {
            if(cur->right == nullptr) {
                cur->right = new RBTNode(val, RED, cur, nullptr, nullptr);
                fixed = cur->right;
                break;
            } else {
                cur = cur->right;
            }
        }
    }

    while(fixed->parent != nullptr && fixed->parent->color == RED) {
        // case: 2, 3
        if( fixed->parent->parent->left == nullptr || fixed->parent->parent->right == nullptr ||
            fixed->parent->parent->left->color == BLACK || fixed->parent->parent->right->color == BLACK) {
            // case 3 to case 2
            if(fixed->parent->parent->left == fixed->parent && fixed->parent->right == fixed) {
                RBTNode *t1, *t2, *t3;
                t1 = fixed->parent->parent;
                t2 = fixed->parent;
                t3 = fixed;
                t1->left = t3;
                t3->parent = t1;

                t2->right = t3->left;
                t3->left = t2;
                t2->parent = t3;
                t3->parent = t1;

                fixed = t2;
            }else if(fixed->parent->parent->right == fixed->parent && fixed->parent->left == fixed) {
                RBTNode *t1, *t2, *t3;
                t1 = fixed->parent->parent;
                t2 = fixed->parent;
                t3 = fixed;

                t1->right = t3;
                t3->parent = t1;
                
                t2->left = t3->right;
                t3->right = t2;
                t2->parent = t3;
                fixed = t2;
            }

            // case 2:
            // left left
            if(fixed->parent->left == fixed) {
                RBTNode *t1, *t2, *t3, *gg;
                gg = fixed->parent->parent->parent;
                t1 = fixed->parent->parent;
                t2 = fixed->parent;
                t3 = fixed;

                if(gg == nullptr)
                    m_root = t2;
                else {
                    if(gg->left == t1) 
                        gg->left = t2;
                    else
                        gg->right = t2;
                }
                t2->parent = gg;

                t1->left = t2->right;
                t2->right = t1;
                t1->parent = t2;
                
                t1->color = RED;
                t2->color = BLACK;
            }
            // right right
            else {
                RBTNode *t1, *t2, *t3, *gg;
                gg = fixed->parent->parent->parent;
                t1 = fixed->parent->parent;
                t2 = fixed->parent;
                t3 = fixed;
                
                if(gg == nullptr)
                    m_root = t2;
                else {
                    if(gg->left == t1) 
                        gg->left = t2;
                    else
                        gg->right = t2;
                }
                t2->parent = gg;

                t1->right = t2->left;
                t2->left = t1;
                t1->parent = t2;
                
                t1->color = RED;
                t2->color = BLACK;
            }
            break;
        }else {
            // case: 1
            fixed->parent->parent->left->color = BLACK;
            fixed->parent->parent->right->color = BLACK;
            fixed->parent->parent->color = RED;
            fixed = fixed->parent->parent;
        }
    }
    m_root->color = BLACK;
};

void RBTree::remove(int val) {
    // find node
    RBTNode* cur = m_root;
    RBTNode* removeNode;
    
    while(cur && cur->value != val) {
        if(val < cur->value) {
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }

    if(cur == nullptr)
        return;
    
    if(cur->left != nullptr && cur->right != nullptr) {
        RBTNode* delNode = cur->left;
        while(delNode->right)
            delNode = delNode->right;
        swap(delNode->value, cur->value);
        cur = delNode;
    }

    // cur is the remove node
    
    RBTNode* gg = cur->parent, *fixed;
    bool isLeftChild = false;
    bool isFixSelf = false;
    if(gg && gg->left == cur) 
        isLeftChild = true;

    bool removeRed = cur->color == RED;
    if(cur->left != nullptr) {
        cur->left->parent = gg;
        fixed = cur->left;
        fixed->parent = gg;
    } else if(cur->right != nullptr) {
        cur->right->parent = gg;
        fixed = cur->right;
        fixed->parent = gg;
    } else {
        fixed = nullptr;
    }

    delete cur;
    if(gg) {
        if(isLeftChild)
            gg->left = fixed;
        else
            gg->right = fixed;
    }
    else {
        m_root = fixed;
        if(m_root != nullptr)
            m_root->color = BLACK;
        return;
    }

    if(removeRed)
        return;

    // delete single black node
    if(fixed == nullptr) {
        if(gg->left != nullptr) {
            if(gg->left->color == RED) {
                bool isLeftChild = false;
                bool isRoot = false;
                RBTNode *ggg = gg->parent;
                if(ggg) {
                    if(gg->parent->left == gg)
                        isLeftChild = true;
                }
                RBTNode *t1 = gg->left;
                gg->left = t1->right;
                t1->right->parent = gg;
                
                t1->right = gg;
                gg->parent = t1;

                t1->parent = ggg;
                if(ggg == nullptr) 
                    m_root = t1;
                else {
                    if(isLeftChild)
                        ggg->left = t1;
                    else
                        ggg->right = t1;
                }
                gg->left->color = RED;
                fixed = t1;
            }
            else {
                if(gg->left->left || gg->left->right) {
                    bool isLeftChild = false;
                    bool isRoot = false;
                    RBTNode *ggg = gg->parent;
                    if(ggg) {
                        if(gg->parent->left == gg)
                            isLeftChild = true;
                    }

                    if(gg->left->left != nullptr) {
                        // right rotate
                        RBTNode *t1 = gg->left;
                        gg->left = t1->right;
                        if(t1->right)
                            t1->right->parent = gg;
                        
                        t1->right = gg;
                        gg->parent = t1;

                        t1->parent = ggg;
                        if(ggg == nullptr) 
                            m_root = t1;
                        else {
                            if(isLeftChild)
                                ggg->left = t1;
                            else
                                ggg->right = t1;
                        }
                    } else {
                        RBTNode *t1 = gg->left;
                        RBTNode *t2 = t1->right;
                        t2->right = gg;
                        gg->parent = t2;
                        t2->left = t1;
                        gg->parent = t1;
                        
                        t1->right = nullptr;
                        gg->left = nullptr;

                        t2->parent = ggg;
                        if(ggg == nullptr)
                            m_root = t2;
                        else {
                            if(isLeftChild)
                                ggg->left = t2;
                            else
                                ggg->right = t2;
                        }
                        t2->color = gg->color;
                        gg->color = BLACK;
                    }
                    m_root->color = BLACK;
                    return;
                } else {
                    gg->left->color = RED;
                    fixed = gg;
                }
            }
            
        }
        else {
            if(gg->right->color == RED) {
                bool isLeftChild = false;
                bool isRoot = false;
                RBTNode *ggg = gg->parent;
                if(ggg) {
                    if(gg->parent->left == gg)
                        isLeftChild = true;
                }
                RBTNode *t1 = gg->right;
                gg->right = t1->left;
                t1->left->parent = gg;
                
                t1->left = gg;
                gg->parent = t1;

                t1->parent = ggg;
                if(ggg == nullptr) 
                    m_root = t1;
                else {
                    if(isLeftChild)
                        ggg->left = t1;
                    else
                        ggg->right = t1;
                }
                gg->right->color = RED;
                fixed = t1;
            }
            else {
                if(gg->right->left || gg->right->right) {
                    bool isLeftChild = false;
                    bool isRoot = false;
                    RBTNode *ggg = gg->parent;
                    if(ggg) {
                        if(gg->parent->left == gg)
                            isLeftChild = true;
                    }

                    if(gg->right->right != nullptr) {
                        // left rotate
                        RBTNode *t1 = gg->right;
                        gg->right = t1->left;
                        if(t1->left)
                            t1->left->parent = gg;
                        
                        t1->left = gg;
                        gg->parent = t1;

                        t1->parent = ggg;
                        if(ggg == nullptr) 
                            m_root = t1;
                        else {
                            if(isLeftChild)
                                ggg->left = t1;
                            else
                                ggg->right = t1;
                        }
                    } else {
                        RBTNode *t1 = gg->right;
                        RBTNode *t2 = t1->left;
                        t2->left = gg;
                        gg->parent = t2;
                        t2->right = t1;
                        gg->parent = t1;
                        
                        t1->left = nullptr;
                        gg->right = nullptr;
                        t2->parent = ggg;

                        if(ggg == nullptr)
                            m_root = t2;
                        else {
                            if(isLeftChild)
                                ggg->left = t2;
                            else
                                ggg->right = t2;
                        }
                        t2->color = gg->color;
                        gg->color = BLACK;
                    }
                    m_root->color = BLACK;
                    return;
                } else {
                    gg->right->color = RED;
                    fixed = gg;
                }
            }
        }
    }

    //fix the RBTree
    while(1) { 
        // fixed RED Node
        if(fixed->color == RED || fixed->parent == nullptr) {
            fixed->color = BLACK;
            break;
        }
        // fix in left
        if(fixed->parent->left == fixed) {
            RBTNode *bro = fixed->parent->right;
            // case 1
            if(bro->color == RED) {
                // left rotate
                RBTNode* gg = fixed->parent->parent;
                RBTNode* t1 = fixed->parent;            // parent
                RBTNode* t2 = fixed->parent->right;     // bro

                bool isGGLeftChild = true;  
                if(gg != nullptr && gg->left != t1)
                    isGGLeftChild = false;  
                t1->right = t2->left;
                t2->left = t1;
                t1->parent = t2;
                
                t2->parent = gg;

                if(gg != nullptr && isGGLeftChild) {
                    gg->left = t2;
                }else {
                    gg->right = t2;
                }
                t2->color = BLACK;
                t1->color = RED;
                continue;
            }
            else {
                // case 2:
                if(bro->left->color == BLACK && bro->right->color == BLACK) {
                    bro->color = RED;
                    fixed = fixed->parent;
                    continue;
                }
                // case 3:
                else if(bro->left->color == RED && bro->right->color == BLACK) {
                    // right rotate
                    RBTNode* gg = bro->parent;              
                    RBTNode* t1 = bro;                      
                    RBTNode* t2 = bro->left;                

                    t1->left = t2->right;
                    t2->right = t1;
                    t1->parent = t2;

                    gg->right = t2;
                    t2->parent = gg;

                    t2->color = BLACK;
                    t1->color = RED;
                }
                
                // case 4:
                else {
                    // left rotate
                    RBTNode* gg = fixed->parent->parent;
                    RBTNode* t1 = fixed->parent;            // parent
                    RBTNode* t2 = fixed->parent->right;     // bro

                    bool isGGLeftChild = true;  
                    if(gg != nullptr && gg->left != t1)
                        isGGLeftChild = false;  

                    swap(t1->color, t2->color);
                    t2->right->color = BLACK;

                    t1->right = t2->left;
                    t2->left = t1;
                    t1->parent = t2;
                    
                    t2->parent = gg;

                    if(gg != nullptr && isGGLeftChild) {
                        gg->left = t2;
                    }else {
                        gg->right = t2;
                    }
                    break;
                }
            }
        }
        // fix in right
        else {
            RBTNode *bro = fixed->parent->left;
            // case 1
            if(bro->color == RED) {
                // right rotate
                RBTNode* gg = fixed->parent->parent;
                RBTNode* t1 = fixed->parent;            // parent
                RBTNode* t2 = fixed->parent->left;     // bro

                bool isGGLeftChild = true;  
                if(gg != nullptr && gg->left != t1)
                    isGGLeftChild = false;  
                
                t1->left = t2->right;
                t2->right = t1;
                t1->parent = t2;
                
                t2->parent = gg;

                if(gg != nullptr && isGGLeftChild) {
                    gg->left = t2;
                }else {
                    gg->right = t2;
                }
                t2->color = BLACK;
                t1->color = RED;
                continue;
            }
            else {
                // case 2:
                if(bro->left->color == BLACK && bro->right->color == BLACK) {
                    bro->color = RED;
                    fixed = fixed->parent;
                    continue;
                }
                // case 3:
                else if(bro->left->color == BLACK && bro->right->color == RED) {
                    // left rotate
                    RBTNode* gg = bro->parent;              
                    RBTNode* t1 = bro;                      
                    RBTNode* t2 = bro->left;                

                    t1->right = t2->left;
                    t2->left = t1;
                    t1->parent = t2;

                    gg->left = t2;
                    t2->parent = gg;

                    t2->color = BLACK;
                    t1->color = RED;
                }
                
                // case 4:
                else {
                    // right rotate
                    RBTNode* gg = fixed->parent->parent;
                    RBTNode* t1 = fixed->parent;            // parent
                    RBTNode* t2 = fixed->parent->left;     // bro

                    bool isGGLeftChild = true;
                    if(gg != nullptr && gg->left != t1)
                        isGGLeftChild = false;  

                    swap(t1->color, t2->color);
                    t2->left->color = BLACK;

                    t1->left = t2->right;
                    t2->right = t1;
                    t1->parent = t2;
                    
                    t2->parent = gg;

                    if(gg != nullptr && isGGLeftChild) {
                        gg->left = t2;
                    }else {
                        gg->right = t2;
                    }
                    break;
                }
            }
        }
    }
};

void RBTree::print() {
    queue<RBTNode*> q;
    if(m_root == nullptr)
        return;
    q.push(m_root);
    while(!q.empty()) {
        size_t levelNum = q.size();
        while(levelNum--) {
            RBTNode* cur = q.front();
            q.pop();
            cout << cur->value << (cur->color == RED ? "R" : "B") << "   ";
            if(cur->left != nullptr)
                q.push(cur->left);
            if(cur->right != nullptr)
                q.push(cur->right);
        }
        cout << endl;
    }
};

int main() {
    RBTree rbt;
    vector<int> in{100, 50, 75, 47, 36, 131, 182, 127, 52, 96};
    for(auto it : in) {
        cout << "-----------insert  " << it << "  -------------" << endl;
        rbt.insert(it);
        rbt.print();
    }

    vector<int> re {100, 75, 47, 131, 182, 127};
    for(auto it : re) {
        cout << "-----------remove  " << it << "  -------------" << endl;
        rbt.remove(it);
        rbt.print();
    }

    vector<int> in2{39, 156, 231, 3, 184};
    for(auto it : in2) {
        cout << "-----------insert  " << it << "  -------------" << endl;
        rbt.insert(it);
        rbt.print();
    }
    vector<int> re2{131, 52, 96, 36, 50, 3, 184, 39, 156, 231};
    for(auto it : re2) {
        cout << "-----------remove  " << it << "  -------------" << endl;
        rbt.remove(it);
        rbt.print();
    }
        
    return 0;
}
```