/*
 * 这里实现B+树的思路和数据库系统概念这本书上的有点不一样，
 * 实现思路相当于是对算法导论上的B树的一点改进
 */
#ifndef BPTREE_H
#define BPTREE_H

#include <string>
#include "base.h"
#include "buffer.h"

enum keyType{INT = -1, FLOAT = 0, STR}; //节点中关键字类型

extern BufferManage buffer;

class BpTree
{
public:
    int number; //索引文件中存在的block的数量

    BpTree(std::string fileName);
    ~BpTree();
    bool insert(Data *key, int row);
    bool lazyDelete(Data *key);
    int find(Data *key);
    std::vector<int> findRange(Data *leftKey, Data *rightKey);
private:
    std::string indexFileName; //索引文件的文件名
    int keyType; //用于构建索引的属性的类型，其值为keyType枚举类中的值
    int keyLength; //用于构建索引的属性的类型占用的内存字节数
    int rootBlockOffset; //在索引文件中存储根节点的block的偏移量
    int minDegree; //B+树的最小度，详情参考算法导论B树部分

    char* initBlock(int isLeaf, int keyNum, int blockOffset);
    int searchInNode(char *block, Data *key);
    void split(char *fatherBlock, int childOffset, int insertPos);
    int search(Data *key, int *leafBlockOffset);
};


#endif // BPTREE_H
