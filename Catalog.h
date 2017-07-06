#ifndef _CATALOG_H
#define _CATALOG_H

#include <string>
#include <map>
#include <vector> 
#include "base.h"

/*only change the information in Catalog files*/
class Catalog{
public:
	Catalog();
	~Catalog();
	void addTable(Table& tableinfo);
	void deleteTable(const std::string& tablename);
	Table getTable(const std::string& tablename);
	void addIndex(const std::string& tablename, const std::string& indexname, const std::string& attriname);
	void deleteIndex(const std::string& indexname);
private:
	int findTable(const std::string& tablename);  //return the byteoffset of the the table, if not exit, return -1
	int findIndex(const std::string& indexname);
	bool findDup(const std::string& tablename, const std::string& attriname);
	void dropTableIndex(const std::string& tablename);
	void initialize();
private:
	std::map<std::string, int> cat;
	std::map<std::string, int> cat_index;
	std::vector<int> buffer_blocks;
};

#endif 
