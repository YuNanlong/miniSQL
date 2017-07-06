/*
 * 一个block存储B+树的一个节点，block中前16个字节存储节点基本信息，按顺序分别是：
 * 节点类型标记（叶节点，内节点），相当于isLeaf变量，4个字节，
 * 节点中关键字数量，相当于keyNum变量，4个字节，
 * 节点是否被删除，相当于delFlag变量，4个字节，
 * 节点对应的block在index文件中的偏移量，相当于blockOffset变量，4个字节
 */
#include <string>
#include "bptree.h"

#include <iostream>
//Debug !!!

using namespace std;

BpTree::BpTree(std::string fileName)
    :indexFileName(fileName)
{
    int bufferNum = buffer.giveMeABlock(fileName, 0);
    //buffer.setLocked(bufferNum, true);
    char *block = new char[BLOCKSIZE];
    memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
    //buffer.setLocked(bufferNum, false);
    keyType = *(int*)(block + 0);
    keyLength = *(int*)(block + 4);
    rootBlockOffset = *(int*)(block + 8);
    number = *(int*)(block + 12);
    minDegree = *(int*)(block + 16);
    delete[] block;
}

BpTree::~BpTree(){
    int bufferNum = buffer.giveMeABlock(indexFileName, 0);
    //buffer.setLocked(bufferNum, true);
    char *block = new char[BLOCKSIZE];
    memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
    *(int*)(block + 8) = rootBlockOffset;
    *(int*)(block + 12) = number;
    memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE);
    //buffer.setLocked(bufferNum, false);
    buffer.setUpdated(bufferNum, true);
    buffer.WriteBack(bufferNum);
    delete[] block;
}
/*
 * 为一块block分配空间，根据形式参数初始化block中的节点基本信息
 */
char* BpTree::initBlock(int isLeaf, int keyNum, int blockOffset){
    char *newBlock = new char[BLOCKSIZE];
    *(int*)(newBlock + ISLEAF) = isLeaf;
    *(int*)(newBlock + KEYNUM) = keyNum;
    *(int*)(newBlock + DELFLAG) = false;
    *(int*)(newBlock + BLOCKOFFSET) = blockOffset;
    *(int*)(newBlock + INDEXBLOCKINFO + 8) = -1;
    if(isLeaf == true){
        *(int*)(newBlock + INDEXBLOCKINFO) = -1;
    }
    return newBlock;
}
/*
 * 在表示节点的block中搜索所有小于或等于形参key的值的关键字中最大值的位置
 */
int BpTree::searchInNode(char *block, Data *key){
    int last = INDEXBLOCKINFO + 8;
    int current = *(int*)(block + last);
    int keyNum = *(int*)(block + KEYNUM);
    int i;
    for(i = 0; i < keyNum; i++){
        if(keyType == INT){
            if(*(int*)(block + current) > ((Datai*)key)->val){
                break;
            }
        }
        else if(keyType == FLOAT){
            if(*(float*)(block + current) > ((Dataf*)key)->val){
                break;
            }
        }
        else{
            if(strncmp((block + current), ((Datac*)key)->val.c_str(), keyLength) > 0){
                break;
            }
        }
        last = current + keyLength + 8;
        current = *(int*)(block + last);
    }
    return last;
}
/*
 * 该函数将属性值key和指向该属性值所在的tuple（即表中的一行）插入B+树中，key为指向属性值的指针，row为属性值对应的tuple在表中的行号（即记录的偏移量）
 */
bool BpTree::insert(Data *key, int row){
    int bufferNum = buffer.giveMeABlock(indexFileName, rootBlockOffset); //将B+树的根节点的block读至buffer，并得到在buffer中的位置
    //buffer.setLocked(bufferNum, true);
    //key重复时抛出异常
    int keyNum = *(int*)(buffer.Buf[bufferNum].BufBlock + KEYNUM); //获取根节点的keyNum
    //buffer.setLocked(bufferNum, false);
    if(keyNum == 2 * minDegree - 1){ //如果根节点已满，则先分裂根节点再插入
        int emptyBufferNum = buffer.giveMeABlock(indexFileName, ++number); //获取一个空的buffer block
        //buffer.setLocked(emptyBufferNum, true);
        char *newBlock = initBlock(false, 0, number); //这里的number是index文件中block的数量
        *(int*)(newBlock + INDEXBLOCKINFO) = rootBlockOffset; //新的根节点中只有一个指向原根节点的指针
        rootBlockOffset = number; //将BpTree对象中的根指针指向新的节点
        memmove(buffer.Buf[emptyBufferNum].BufBlock, newBlock, BLOCKSIZE); //将newBlock拷贝至buffer中的空block
        buffer.setUpdated(emptyBufferNum, true);
        //buffer.setLocked(emptyBufferNum, false);
        buffer.WriteBack(emptyBufferNum);
        delete[] newBlock;
    }
    
    int offset = rootBlockOffset, childOffset;
    int isLeaf, insertPos, emptyPos, cBufferNum, cKeyNum;
    char *block = new char[BLOCKSIZE];
    char *childBlock = new char[BLOCKSIZE];
    
    while(1){
        bufferNum = buffer.giveMeABlock(indexFileName, offset);
        buffer.setLocked(bufferNum, true);
        memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE); //从buffer的相应block中拷贝数据至tempBlock
        keyNum = *(int*)(block + KEYNUM);
        isLeaf = *(int*)(block + ISLEAF);
        insertPos = searchInNode(block, key); //在当前节点搜索插入位置
        emptyPos = INDEXBLOCKINFO + POINTERLEN + keyNum * (keyLength + POINTERLEN);
        if(true == isLeaf){
            //插入key的值
            if(keyType == INT){
                if(insertPos != (INDEXBLOCKINFO + 8)
                   && *(int*)(block + insertPos - keyLength - 8) == ((Datai*)key)->val){
                    if(*(int*)(block + insertPos - 4) == 0){
                        buffer.setLocked(bufferNum, false);
                        delete[] block;
                        delete[] childBlock;
                        return false;
                    }
                    else{
                        *(int*)(block + insertPos - 8) = row;
                        *(int*)(block + insertPos - 4) = 0;
                        memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE); //将tempBlock拷贝回buffer中原节点的block
                        buffer.setUpdated(bufferNum, true);
                        buffer.WriteBack(bufferNum);
                        break;
                    }
                }
                *(int*)(block + emptyPos) = (((Datai*)key)->val);
            }
            else if(keyType == FLOAT){
                if(insertPos != (INDEXBLOCKINFO + 8)
                   && *(float*)(block + insertPos - keyLength - 8) == ((Dataf*)key)->val){
                    if(*(int*)(block + insertPos - 4) == 0){
                        buffer.setLocked(bufferNum, false);
                        delete[] block;
                        delete[] childBlock;
                        return false;
                    }
                    else{
                        *(int*)(block + insertPos - 8) = row;
                        *(int*)(block + insertPos - 4) = 0;
                        memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE); //将tempBlock拷贝回buffer中原节点的block
                        buffer.setUpdated(bufferNum, true);
                        buffer.WriteBack(bufferNum);
                        break;
                    }
                }
                *(float*)(block + emptyPos) = (((Dataf*)key)->val);
            }
            else{
                if(insertPos != (INDEXBLOCKINFO + 8)
                   && strncmp((block + insertPos - keyLength - 8), ((Datac*)key)->val.c_str(), keyLength) == 0){
                    if(*(int*)(block + insertPos - 4) == 0){
                        buffer.setLocked(bufferNum, false);
                        delete[] block;
                        delete[] childBlock;
                        return false;
                    }
                    else{
                        *(int*)(block + insertPos - 8) = row;
                        *(int*)(block + insertPos - 4) = 0;
                        memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE); //将tempBlock拷贝回buffer中原节点的block
                        buffer.setUpdated(bufferNum, true);
                        buffer.WriteBack(bufferNum);
                        break;
                    }
                }
                memset((block + emptyPos), 0, keyLength);
                memmove((block + emptyPos), ((Datac*)key)->val.c_str(), keyLength);
            }
            *(int*)(block + emptyPos + keyLength + 8) = *(int*)(block + insertPos); //新插入的关键字的next指针指向下一个关键字
            *(int*)(block + insertPos) = emptyPos; //新插入的关键字的上一个关键字的next指针指向新插入的关键字
            *(int*)(block + emptyPos + keyLength) = row; //新插入的关键字的child指针指向表文件中的tuple
            *(int*)(block + emptyPos + keyLength + 4) = 0; //新插入的关键字的删除标记应该赋值为0
            *(int*)(block + KEYNUM) = ++keyNum; //节点中的keyNum增加1
            memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE); //将tempBlock拷贝回buffer中原节点的block
            buffer.setUpdated(bufferNum, true);
            buffer.WriteBack(bufferNum);
            break;
        }
        else{
            childOffset = *(int*)(block + insertPos - 8);
            cBufferNum = buffer.giveMeABlock(indexFileName, childOffset);
            memmove(childBlock, buffer.Buf[cBufferNum].BufBlock, BLOCKSIZE);
            cKeyNum = *(int*)(childBlock + KEYNUM);
            
            if(cKeyNum == 2 * minDegree - 1){
                split(block, childOffset, insertPos);
                int nextPos = *(int*)(block + insertPos);
                if(keyType == INT){
                    if(*(int*)(block + nextPos) <= ((Datai*)key)->val){
                        childOffset = *(int*)(block + nextPos + keyLength);
                    }
                }
                else if(keyType == FLOAT){
                    if(*(float*)(block + nextPos) <= ((Dataf*)key)->val){
                        childOffset = *(int*)(block + nextPos + keyLength);
                    }
                }
                else{
                    if(strncmp((block + nextPos), ((Datac*)key)->val.c_str(), keyLength) <= 0){
                        childOffset = *(int*)(block + nextPos + keyLength);
                    }
                }
                memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE); //将tempBlock拷贝回buffer中原节点的block
                buffer.setUpdated(bufferNum, true);
                buffer.WriteBack(bufferNum);
                offset = childOffset;
                continue;
            }
            buffer.setLocked(bufferNum, false);
            offset = childOffset;
        }
    }
    delete[] block;
    delete[] childBlock;
    return true;
}

void BpTree::split(char *fatherBlock, int childOffset, int insertPos){
    int fKeyNum = *(int*)(fatherBlock + KEYNUM);
    int bufferNum = buffer.giveMeABlock(indexFileName, childOffset); //需要被buffer类的函数替换
    buffer.setLocked(bufferNum, true);
    char *childBlock = new char[BLOCKSIZE];
    char *leftBlock, *rightBlock;
    memmove(childBlock, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE); //从buffer的相应block中拷贝数据至childBlock
    int cIsLeaf = *(int*)(childBlock + ISLEAF);
    int i;

    int emptyBufferNum = buffer.giveMeABlock(indexFileName, ++number);
    buffer.setLocked(emptyBufferNum, true);
    int pos = INDEXBLOCKINFO + 8; //pos用于遍历节点中的关键字
    if(cIsLeaf == true){
        leftBlock = initBlock(true, minDegree, childOffset); //为新分裂出的左节点初始化一个block
        rightBlock = initBlock(true, minDegree - 1, number); //为新分裂出的右节点初始化一个block

        for(i = 0; i < minDegree; i++){
            pos = *(int*)(childBlock + pos);
            memcpy(leftBlock + INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN), (childBlock + pos), (keyLength + POINTERLEN));
            pos += keyLength + 8;
        }
        for(i = 0; i < minDegree; i++){
            *(int*)(leftBlock + INDEXBLOCKINFO + i * (keyLength + POINTERLEN) + 8) = INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN);
        }
        *(int*)(leftBlock + INDEXBLOCKINFO + minDegree * (keyLength + POINTERLEN) + 8) = -1;

        int emptyPos = INDEXBLOCKINFO + POINTERLEN + fKeyNum * (keyLength + POINTERLEN); //block中空闲部分的起始位置
        memcpy((fatherBlock + emptyPos), (childBlock + *(int*)(childBlock + pos)), keyLength); //将位于被分裂节点中间的关键字的值写入其父节点中
        *(int*)(fatherBlock + emptyPos + keyLength + 8) = *(int*)(fatherBlock + insertPos); //新插入的关键字的next指针指向下一个关键字
        *(int*)(fatherBlock + insertPos) = emptyPos; //新插入的关键字的上一个关键字的next指针指向新插入的关键字
        *(int*)(fatherBlock + emptyPos + keyLength) = number; //新插入的关键字的child指针指向分裂出的新节点
        *(int*)(fatherBlock + KEYNUM) = ++fKeyNum; //父节点中的keyNum增加1

        for(int i = 0; i < minDegree - 1; i++){ //将被分裂节点的后minDegree - 1个关键字写入新分裂出的右节点中
            pos = *(int*)(childBlock + pos);
            memcpy((rightBlock + INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN)), (childBlock + pos), (keyLength + POINTERLEN));
            pos += keyLength + 8;
        }
        for(int i = 0; i < minDegree - 1; i++){ //将新分裂出的右节点中的关键字串联成链表
            *(int*)(rightBlock + INDEXBLOCKINFO + i * (keyLength + POINTERLEN) + 8) = INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN);
        }
        *(int*)(rightBlock + INDEXBLOCKINFO + (minDegree - 1) * (keyLength + POINTERLEN) + 8) = -1;
        *(int*)(rightBlock + INDEXBLOCKINFO) = *(int*)(childBlock + INDEXBLOCKINFO); //新分裂出的右节点的next指针指向原来被分裂节点的next指针指向的位置
        *(int*)(leftBlock + INDEXBLOCKINFO) = number; //新分裂出的左节点的next指针指向新分裂出的右节点
    }
    else{
        leftBlock = initBlock(false, minDegree - 1, childOffset); //为新分裂出的左节点初始化一个block
        rightBlock = initBlock(false, minDegree - 1, number); //为新分裂出的右节点初始化一个block

        memcpy(leftBlock + INDEXBLOCKINFO, childBlock + INDEXBLOCKINFO, POINTERLEN);
        for(i = 0; i < minDegree - 1; i++){ //将被分裂节点的前minDegree - 1个关键字写入新分裂出的左节点中
            pos = *(int*)(childBlock + pos);
            memcpy(leftBlock + INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN), (childBlock + pos), (keyLength + POINTERLEN));
            pos += keyLength + 8;
        }
        for(i = 0; i < minDegree - 1; i++){ //将新分裂出的左节点中的关键字串联成链表
            *(int*)(leftBlock + INDEXBLOCKINFO + i * (keyLength + POINTERLEN) + 8) = INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN);
        }
        *(int*)(leftBlock + INDEXBLOCKINFO + (minDegree - 1) * (keyLength + POINTERLEN) + 8) = -1; //关键字链表中最后一个next指针指向null

        pos = *(int*)(childBlock + pos) + keyLength + 8; //pos指向被分裂节点中间的关键字
        int emptyPos = INDEXBLOCKINFO + POINTERLEN + fKeyNum * (keyLength + POINTERLEN); //block中空闲部分的起始位置
        memcpy((fatherBlock + emptyPos), (childBlock + pos - keyLength - 8), keyLength); //将位于被分裂节点中间的关键字的值写入其父节点中
        *(int*)(fatherBlock + emptyPos + keyLength + 8) = *(int*)(fatherBlock + insertPos); //新插入的关键字的next指针指向下一个关键字
        *(int*)(fatherBlock + insertPos) = emptyPos; //新插入的关键字的上一个关键字的next指针指向新插入的关键字
        *(int*)(fatherBlock + emptyPos + keyLength) = number; //新插入的关键字的child指针指向分裂出的新节点
        *(int*)(fatherBlock + KEYNUM) = ++fKeyNum; //父节点中的keyNum增加1

        *(int*)(rightBlock + INDEXBLOCKINFO) = *(int*)(childBlock + pos - 8);
        for(int i = 0; i < minDegree - 1; i++){ //将被分裂节点的后minDegree - 1个关键字写入新分裂出的右节点中
            pos = *(int*)(childBlock + pos);
            memcpy((rightBlock + INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN)), (childBlock + pos), (keyLength + POINTERLEN));
            pos += keyLength + 8;
        }
        for(int i = 0; i < minDegree - 1; i++){ //将新分裂出的右节点中的关键字串联成链表
            *(int*)(rightBlock + INDEXBLOCKINFO + i * (keyLength + POINTERLEN) + 8) = INDEXBLOCKINFO + POINTERLEN + i * (keyLength + POINTERLEN);
        }
        *(int*)(rightBlock + INDEXBLOCKINFO + (minDegree - 1) * (keyLength + POINTERLEN) + 8) = -1; //关键字链表中最后一个next指针指向null
    }
    memmove(buffer.Buf[emptyBufferNum].BufBlock, rightBlock, BLOCKSIZE); //将rightBlock拷贝至空的buffer block
    buffer.setUpdated(emptyBufferNum, true);
    buffer.WriteBack(emptyBufferNum);
    memmove(buffer.Buf[bufferNum].BufBlock, leftBlock, BLOCKSIZE); //将leftBlock拷贝回buffer中原被分裂节点的block
    buffer.setUpdated(bufferNum, true);
    buffer.WriteBack(bufferNum); //将buffer block写回内存
    delete[] rightBlock;
    delete[] leftBlock;
    delete[] childBlock;
}

bool BpTree::lazyDelete(Data *key){
    int blockOffset;
    int pos = search(key, &blockOffset);
    if(pos == INDEXBLOCKINFO + 8){
        return false;
    }
    int bufferNum = buffer.giveMeABlock(indexFileName, blockOffset); //需要被buffer类的函数替换
    //buffer.setLocked(bufferNum, true);
    char *block = new char[BLOCKSIZE];
    memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE); //从buffer的相应block中拷贝数据至block数组
    if(keyType == INT){
        if(*(int*)(block + pos - 8 - keyLength) == ((Datai*)key)->val && *(int*)(block + pos - 4) == 0){
            *(int*)(block + pos - 4) = 1;
            memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE);
            buffer.setUpdated(bufferNum, true);
            //buffer.setLocked(bufferNum, false);
            buffer.WriteBack(bufferNum);
            delete[] block;
            return true;
        }
    }
    else if(keyType == FLOAT){
        if(*(float*)(block + pos - 8 - keyLength) == ((Dataf*)key)->val && *(int*)(block + pos - 4) == 0){
            *(int*)(block + pos - 4) = 1;
            memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE);
            buffer.setUpdated(bufferNum, true);
            //buffer.setLocked(bufferNum, false);
            buffer.WriteBack(bufferNum);
            delete[] block;
            return true;
        }
    }
    else{
        if(strncmp((block + pos - 8 - keyLength), ((Datac*)key)->val.c_str(), keyLength) == 0
           && *(int*)(block + pos - 4) == 0){
            *(int*)(block + pos - 4) = 1;
            memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE);
            buffer.setUpdated(bufferNum, true);
            //buffer.setLocked(bufferNum, false);
            buffer.WriteBack(bufferNum);
            delete[] block;
            return true;
        }
    }
    delete[] block;
    return false;
}

int BpTree::search(Data *key, int *leafBlockOffset){
    int blockOffset = rootBlockOffset; //blockOffset变量用于表示搜索过程中遇到的节点的block在index文件中的偏移量
    char *tempBlock = new char[BLOCKSIZE];
    int pos, isLeaf, keyNum, bufferNum;
    while(1){
        bufferNum = buffer.giveMeABlock(indexFileName, blockOffset); //需要被buffer类的函数替换
        //buffer.setLocked(bufferNum, true);
        memmove(tempBlock, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE); //从buffer的相应block中拷贝数据至tempBlock
        //buffer.setLocked(bufferNum, false);
        isLeaf = *(int*)(tempBlock + ISLEAF);
        keyNum = *(int*)(tempBlock + KEYNUM);
        pos = searchInNode(tempBlock, key); //搜索节点中小于或等于形参key的关键字中的最大值的位置
        if(isLeaf == true){
            break;
        }
        else{
            blockOffset = *(int*)(tempBlock + pos - 8); //blockOffset变为child节点的block在index文件中的偏移量
        }
    }
    delete[] tempBlock;
    *leafBlockOffset = blockOffset; //将包含搜索结果的block在index文件中的偏移量通过形参leafBlockOffset变量返回
    return pos; //返回搜索结果在block中的位置
}

int BpTree::find(Data *key){
    int blockOffset, resultRow = -1;
    int pos = search(key, &blockOffset);
    int bufferNum = buffer.giveMeABlock(indexFileName, blockOffset); //需要被buffer类的函数替换
    //buffer.setLocked(bufferNum, true);
    char *block = new char[BLOCKSIZE];
    memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE); //从buffer的相应block中拷贝数据至block数组
    //buffer.setLocked(bufferNum, false);
    if(keyType == INT){
        if(*(int*)(block + pos - 8 - keyLength) == ((Datai*)key)->val && *(int*)(block + pos - 4) == 0){
            resultRow = *(int*)(block + pos - 8);
        }
    }
    else if(keyType == FLOAT){
        if(*(float*)(block + pos - 8 - keyLength) == ((Dataf*)key)->val && *(int*)(block + pos - 4) == 0){
            resultRow = *(int*)(block + pos - 8);
        }
    }
    else{
        if(strncmp((block + pos - 8 - keyLength), ((Datac*)key)->val.c_str(), keyLength) == 0
           && *(int*)(block + pos - 4) == 0){
            resultRow = *(int*)(block + pos - 8);
        }
    }
    delete[] block;
    return resultRow;
}

vector<int> BpTree::findRange(Data *leftKey, Data *rightKey){
    vector<int> result;
    if(keyType == INT && ((Datai*)leftKey)->val > ((Datai*)rightKey)->val){
        return result;
    }
    else if(keyType == FLOAT && ((Dataf*)leftKey)->val > ((Dataf*)rightKey)->val){
        return result;
    }
    else if(strncmp(((Datac*)leftKey)->val.c_str(), ((Datac*)rightKey)->val.c_str(), keyLength) > 0){
        return result;
    }
    int leftBlockOffset, rightBlockOffset, leftPos, rightPos, pos;
    int isLeaf, blockOffset, bufferNum;
    char *block = new char[BLOCKSIZE];
    if(leftKey != nullptr){
        leftPos = search(leftKey, &leftBlockOffset);
        bufferNum = buffer.giveMeABlock(indexFileName, leftBlockOffset);
        //buffer.setLocked(bufferNum, true);
        memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
        //buffer.setLocked(bufferNum, false);
        if(leftPos != INDEXBLOCKINFO + 8){
            if(keyType == INT){
                if(*(int*)(block + leftPos - keyLength - 8) == ((Datai*)leftKey)->val){
                    leftPos = leftPos - keyLength - 8;
                }
                else{
                    leftPos = *(int*)(block + leftPos);
                }
            }
            else if(keyType == FLOAT){
                if(*(float*)(block + leftPos - keyLength - 8) == ((Dataf*)leftKey)->val){
                    leftPos = leftPos - keyLength - 8;
                }
                else{
                    leftPos = *(int*)(block + leftPos);
                }
            }
            else{
                if(strncmp((block + leftPos - keyLength - 8), ((Datac*)leftKey)->val.c_str(), keyLength) == 0){
                    leftPos = leftPos - keyLength - 8;
                }
                else{
                    leftPos = *(int*)(block + leftPos);
                }
            }
        }
        else{
            leftPos = *(int*)(block + leftPos);
        }
    }
    else{
        blockOffset = rootBlockOffset;
        while(1){
            bufferNum = buffer.giveMeABlock(indexFileName, blockOffset);
            //buffer.setLocked(bufferNum, true);
            memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
            isLeaf = *(int*)(block + ISLEAF);
            if(isLeaf == true){
                leftPos = INDEXBLOCKINFO + POINTERLEN;
                leftBlockOffset = blockOffset;
                //buffer.setLocked(bufferNum, false);
                break;
            }
            else{
                blockOffset = *(int*)(block + INDEXBLOCKINFO); //取指向最左子节点的指针
                //buffer.setLocked(bufferNum, false);
            }
        }
    }
    if(rightKey != nullptr){
        rightPos = search(rightKey, &rightBlockOffset);
        bufferNum = buffer.giveMeABlock(indexFileName, rightBlockOffset);
        //buffer.setLocked(bufferNum, true);
        memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
        //buffer.setLocked(bufferNum, false);
        rightPos = *(int*)(block + rightPos);
        
        blockOffset = leftBlockOffset;
        bufferNum = buffer.giveMeABlock(indexFileName, blockOffset);
        //buffer.setLocked(bufferNum, true);
        memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
        //buffer.setLocked(bufferNum, false);
        pos = leftPos;
        while(1){
            if(blockOffset == rightBlockOffset){
                while(pos != rightPos){
                    if(*(int*)(block + pos + keyLength + 4) == 0){
                        result.push_back(*(int*)(block + pos + keyLength));
                    }
                    pos = *(int*)(block + pos + keyLength + 8);
                }
                break;
            }
            else{
                while(pos != -1){
                    if(*(int*)(block + pos + keyLength + 4) == 0){
                        result.push_back(*(int*)(block + pos + keyLength));
                    }
                    pos = *(int*)(block + pos + keyLength + 8);
                }
                blockOffset = *(int*)(block + INDEXBLOCKINFO);
                bufferNum = buffer.giveMeABlock(indexFileName, blockOffset);
                //buffer.setLocked(bufferNum, true);
                memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
                //buffer.setLocked(bufferNum, false);
                pos = *(int*)(block + INDEXBLOCKINFO + 8);
            }
        }
    }
    else{
        blockOffset = leftBlockOffset;
        bufferNum = buffer.giveMeABlock(indexFileName, blockOffset);
        //buffer.setLocked(bufferNum, true);
        memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
        //buffer.setLocked(bufferNum, false);
        pos = leftPos;
        while(blockOffset != -1){
            while(pos != -1){
                if(*(int*)(block + pos + keyLength + 4) == 0){
                    result.push_back(*(int*)(block + pos + keyLength));
                }
                pos = *(int*)(block + pos + keyLength + 8);
            }
            blockOffset = *(int*)(block + INDEXBLOCKINFO);
            if(blockOffset == -1){
                break;
            }
            bufferNum = buffer.giveMeABlock(indexFileName, blockOffset);
            //buffer.setLocked(bufferNum, true);
            memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
            //buffer.setLocked(bufferNum, false);
            pos = *(int*)(block + INDEXBLOCKINFO + 8);
        }
    }
    return result;
}
