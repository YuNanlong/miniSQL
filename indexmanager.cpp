#include "indexmanager.h"
#include "bptree.h"

using namespace std;

void IndexManager::createIndexFile(string fileName, int keyType){
    int keyLength;
    buffer.createFile(fileName);
    int emptyBufferNum = buffer.giveMeABlock(fileName, 0);
    buffer.setLocked(emptyBufferNum, true);
    char *block = new char[BLOCKSIZE];
    *(int*)(block + 0) = keyType; //keyType
    if(keyType == -1 || keyType == 0){ //keyLength
        *(int*)(block + 4) = keyLength = 4;
    }
    else{
        *(int*)(block + 4) = keyLength = keyType;
    }
    *(int*)(block + 8) = 1; //rootOffset
    *(int*)(block + 12) = 1; //number
    int order = (BLOCKSIZE - INDEXBLOCKINFO) / (POINTERLEN + keyLength) / 2;
    if(order % 2 == 0){
        order--;
    }
    *(int*)(block + 16) = order; //order
    *(int*)(block + 20) = true; //delete or not
    memmove(buffer.Buf[emptyBufferNum].BufBlock, block, BLOCKSIZE);
    buffer.setUpdated(emptyBufferNum, true);
    buffer.WriteBack(emptyBufferNum);

    emptyBufferNum = buffer.giveMeABlock(fileName, 1);
    buffer.setLocked(emptyBufferNum, true);
    *(int*)(block + ISLEAF) = true;
    *(int*)(block + KEYNUM) = 0;
    *(int*)(block + DELFLAG) = false;
    *(int*)(block + BLOCKOFFSET) = 1;
    *(int*)(block + INDEXBLOCKINFO + 8) = -1; //节点内关键字链表的next指针
    *(int*)(block + INDEXBLOCKINFO) = -1; //叶节点之间的next指针
    memmove(buffer.Buf[emptyBufferNum].BufBlock, block, BLOCKSIZE);
    buffer.setUpdated(emptyBufferNum, true);
    buffer.WriteBack(emptyBufferNum);
    delete[] block;
}

bool IndexManager::insertKey(string fileName, Data *key, int row){
    BpTree tree(fileName);
    return tree.insert(key, row);
}

int IndexManager::findKey(string fileName, Data *key){
    BpTree tree(fileName);
    return tree.find(key);
}

vector<int> IndexManager::findKeyRange(string fileName, Data *leftKey, Data *rightKey){
    BpTree tree(fileName);
    return tree.findRange(leftKey, rightKey);
}

bool IndexManager::deleteKey(string fileName, Data *key){
    BpTree tree(fileName);
    return tree.lazyDelete(key);
}

void IndexManager::createIndex(string fileName){
    int bufferNum = buffer.giveMeABlock(fileName, 0);
    buffer.setLocked(bufferNum, true);
    char *block = new char[BLOCKSIZE];
    memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
    *(int*)(block + 20) = false;
    memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE);
    buffer.setLocked(bufferNum, false);
    buffer.WriteBack(bufferNum);
    delete[] block;
}

void IndexManager::dropIndex(string fileName){
    int bufferNum = buffer.giveMeABlock(fileName, 0);
    buffer.setLocked(bufferNum, true);
    char *block = new char[BLOCKSIZE];
    memmove(block, buffer.Buf[bufferNum].BufBlock, BLOCKSIZE);
    *(int*)(block + 20) = true;
    memmove(buffer.Buf[bufferNum].BufBlock, block, BLOCKSIZE);
    buffer.setLocked(bufferNum, false);
    buffer.WriteBack(bufferNum);
    delete[] block;
}
