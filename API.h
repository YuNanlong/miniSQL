#ifndef _API_H
#define _API_H

#include "base.h"
#include<string>

class API{
public:
	void createTable(Table& tableinfo);
	void dropTable(const std::string& tablename);
	void createIndex(const std::string& indexName, const std::string& tableName, const std::string& attrName);
	void dropIndex(const std::string& indexName);
	void insert(const Table& tableInfo, Tuple& line);
	int del(const Table& tableInfo, std::vector<condition> comp);
	Table select(Table& tableInfo, std::vector<int> attrIndex, std::vector<condition> comp);
};

#endif
