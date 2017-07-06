#ifndef SQL_INDEXMANAGER_H
#define SQL_INDEXMANAGER_H

#include <string>
#include <vector>
#include "base.h"

class IndexManager{
public:
    void createIndexFile(std::string fileName, int keyType);
    bool insertKey(std::string fileName, Data *key, int row);
    bool deleteKey(std::string fileName, Data *key);
    int findKey(std::string fileName, Data *key);
    std::vector<int> findKeyRange(std::string fileName, Data *leftKey, Data *rightKey);
    void createIndex(std::string fileName);
    void dropIndex(std::string fileName);
    void test();
};


#endif //SQL_INDEXMANAGER_H
