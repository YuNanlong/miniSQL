#ifndef _RECORDMANAGER_H
#define _RECORDMANAGER_H

#include "base.h"
#include "buffer.h"
#include "Catalog.h"
#include "indexmanager.h"
#include <cmath>

class RecordManager
{
public:
	RecordManager(BufferManage *pBF):pBuffer(pBF){}
	~RecordManager(){}

private:
	RecordManager(){}
	BufferManage *pBuffer;
public:
	bool tableCreate(const Table& tableInfo);
	bool tableDrop(const Table& tableInfo);
	Table SelectWithWhere(Table& tableInfo, std::vector<int> attrIndex, std::vector<condition> comp);
	int Delete(const Table& tableInfo, std::vector<condition> comp);	
	Table PrintAnswer(Table& tableInfo, std::vector<int>attrIndex);
	void InsertValue(const Table& tableInfo, Tuple& line);
	bool MeetCondition(const Table& tableInfo, Tuple* line, std::vector<condition> comp);
	Tuple* FindWithIndex(const Table& tableInfo, int uniqueNum, Data* uniqueKey, int tupleLength,int* posKey);
	Tuple* Line2Tuple(char* line,const Table& tableInfo);
	char* Tuple2Line(const Table& tableInfo, Tuple& line, int tupleLength);

};

#endif
