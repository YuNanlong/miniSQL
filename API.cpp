#include "API.h"
#include "base.h"
#include "Catalog.h"
#include "recordmanager.h"
#include "buffer.h"

using namespace std;

extern BufferManage buffer;

void API::createTable(Table& tableinfo)
{
	Catalog ca;
	RecordManager rm(&buffer);

	ca.addTable(tableinfo);
	rm.tableCreate(tableinfo);
}

void API::dropTable(const string& tablename)
{
	Catalog ca;
	RecordManager rm(&buffer);

	Table t = ca.getTable(tablename);
	rm.tableDrop(t);
	ca.deleteTable(tablename);
}

void API::createIndex(const string& indexName, const string& tableName, const string& attrName){
	Catalog ca;

	ca.addIndex(tableName, indexName, attrName);
}

void API::dropIndex(const string& indexName){
	Catalog ca;

	ca.deleteIndex(indexName);
}

void API::insert(const Table& tableInfo, Tuple& line){
	RecordManager rm(&buffer);

	rm.InsertValue(tableInfo, line);
}

int API::del(const Table& tableInfo, vector<condition> comp){
	RecordManager rm(&buffer);

	return rm.Delete(tableInfo, comp);
}

Table API::select(Table& tableInfo, vector<int> attrIndex, vector<condition> comp){
	RecordManager rm(&buffer);

	return rm.SelectWithWhere(tableInfo, attrIndex, comp);
}
