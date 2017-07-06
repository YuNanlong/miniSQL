/*
store structure of catalog:
    the number of bytes(int, 4 bytes)
    the blockoffset of the block(int, 4 bytes)
    ====the number of blocks in the file (int, 4 bytes) only in the first block=====
    table metas

table metadata store sturcture:
    valid flag (1 for valid, 0 for invalid) (int, 4 bytes)
    =============byteoffset of table from the bytes on======================
    the number of bytes of the table (include the current number int, exclude the valid flag) int, 4 bytes
    the length of the tablename (int, 4bytes)
    the name of the tablename(char*)
    the number of the attributes(int, 4 bytes)
    attributes info
       the length of the attributes, int ,4bytes
       the name of the attributes(char*)
    type of all attributes(int*)
    unqiue of all attributes(bool*)
    primary key(int)
 */

#include"Catalog.h"
#include"buffer.h"
#include"base.h"
#include<string>
#include<string.h>
#include<stdexcept>
#include<map>
#include<vector>
#include<iostream>
using namespace std;

const int BYTENUM_ = 0; //the first four bytes store the number of bytes in the block
const int BLOCKOFFSET_ = 4; //the next four bytes store the blockoffset of the block
const int BLOCKNUM_ = 8; //store the number of blocks in the catalog file in the first block

extern BufferManage buffer;

Catalog::Catalog()
{
	cat.clear();
	cat_index.clear();
	buffer_blocks.clear();
	initialize();
}

void Catalog::initialize()
{
	try{
		buffer.giveMeABlock("Catalog.bin", 0);
	}
	catch(runtime_error& ex)
	{
		buffer.createFile("Catalog.bin");
		int block = buffer.giveMeABlock("Catalog.bin", 0);
		int byte = 3 * sizeof(int);
		int blockoffset = 0;
		int blocknum = 1;
		memcpy( buffer.Buf[block].BufBlock + BYTENUM_, &byte, sizeof(int) );	
		memcpy(buffer.Buf[block].BufBlock + BLOCKOFFSET_, &blockoffset, sizeof(int) );
		memcpy(buffer.Buf[block].BufBlock + BLOCKNUM_, &blocknum, sizeof(int));
		
		buffer.setUpdated(block, true);
		buffer.WriteBack(block);
	}
	
	try{
		buffer.giveMeABlock("CatIndex.bin", 0);
	}
	catch(runtime_error& ex)
	{
		buffer.createFile("CatIndex.bin");
		int block = buffer.giveMeABlock("CatIndex.bin", 0);
		int byte = 3 * sizeof(int);
		int blockoffset = 0;
		int blocknum = 1;
		memcpy(buffer.Buf[block].BufBlock + BYTENUM_, &byte, sizeof(int) );
		memcpy(buffer.Buf[block].BufBlock + BLOCKOFFSET_, &blockoffset, sizeof(int) );
		memcpy(buffer.Buf[block].BufBlock + BLOCKNUM_, &blocknum, sizeof(int));
		buffer.setUpdated(block, true);
		buffer.WriteBack(block);
	}
}

Catalog::~Catalog()
{
	for(int i = 0; i < buffer_blocks.size(); i++)
	    buffer.WriteBack(buffer_blocks[i]);
	
	buffer_blocks.clear();
	cat.clear();
	cat_index.clear();	
}

void Catalog::addTable(Table& tableinfo)
{
	if(findTable(tableinfo.tableName) != -1)
	    throw runtime_error("Error: duplicated table name!");	
	
	int firstblock = buffer.giveMeABlock("Catalog.bin", 0);
	int blocknum;
	memcpy(&blocknum, buffer.Buf[firstblock].BufBlock + BLOCKNUM_, sizeof(int) );  //get the number of block in catalog file
	
	//find the end of the catalog file
	int lastblock;  
	if(blocknum != 1)
		lastblock = buffer.giveMeABlock("Catalog.bin", (blocknum-1) );
	else
	    lastblock = firstblock;
	
	int bytenum;  //get the number of bytes in the last block
	memcpy(&bytenum, buffer.Buf[lastblock].BufBlock + BYTENUM_, sizeof(int));
	int tablesize = tableinfo.tableSize() + sizeof(int); //the total bytes of the table + (int)(0 for invalid, 1 for valid)
	int pos;
	
	/*get the position of the new table record*/
	int sum = bytenum + tablesize + sizeof(int);
	if(sum <= BLOCKSIZE) //the table information can be written in the last block
		pos = bytenum;
	
	else  //table information can't be added to the last block
	{
		blocknum++; //change the blocknum stored in the first block
		memcpy(buffer.Buf[firstblock].BufBlock + BLOCKNUM_, &blocknum, sizeof(int) );
		buffer.setUpdated(firstblock, true);
		
		//get an empty block and initialize
		lastblock = buffer.giveMeABlock("Catalog.bin", blocknum-1);
		buffer.setLocked(lastblock, true);
		buffer_blocks.push_back(lastblock);
		
		int temp_bytenum = 2 * sizeof(int);
		memcpy(buffer.Buf[lastblock].BufBlock + BYTENUM_, &temp_bytenum, sizeof(int) );
		int temp_blockoffset = blocknum - 1;
		memcpy(buffer.Buf[lastblock].BufBlock + BLOCKOFFSET_, &temp_blockoffset, sizeof(int) );
		
		pos = 2 * sizeof(int);
	}
	
	//set valid = 1
	int valid = 1;
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &valid, sizeof(int));
	pos += sizeof(int);
	
	/*copy the information in tableinfo to catalog file*/
	int tmp_byteoffset = (blocknum - 1) * BLOCKSIZE + pos;
	
	//first is the number of bytes of the table info
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &tablesize, sizeof(int) ); 
	pos += sizeof(int);
	
	//then, the length of the table name
	int length = tableinfo.tableName.size();
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &length, sizeof(int) ); 
	pos += sizeof(int);
	
	//then, the name of the table, NO '\0' !!
	memcpy(buffer.Buf[lastblock].BufBlock + pos, tableinfo.tableName.c_str(), length); 
	pos += length;
	
	//then, the number of attributes
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &( tableinfo.attr.attrNum ), sizeof(int) );
	pos += sizeof(int);
	
	int aLength;
	int anum = tableinfo.attr.attrNum;
	for(int i = 0; i < anum; i++)
	{
		//the length of the attribute name
		aLength = tableinfo.attr.attrName[i].size();
		memcpy(buffer.Buf[lastblock].BufBlock + pos, &aLength, sizeof(int) );  
		pos += sizeof(int);
		
		//the name of the attributes WITHOUT '\0'
		memcpy(buffer.Buf[lastblock].BufBlock + pos, tableinfo.attr.attrName[i].c_str(), aLength );   
		pos += aLength;
	}
	
	//type of all the attributes
	memcpy(buffer.Buf[lastblock].BufBlock + pos, tableinfo.attr.type, anum*sizeof(int) ); 
	pos += (anum * sizeof(int) );
	
	//unique of all attributes
	memcpy(buffer.Buf[lastblock].BufBlock + pos, tableinfo.attr.uniqueFlag, anum);  
	pos += anum;
	
	//the primary key of the table
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &( tableinfo.attr.primary), sizeof(int) ); 
	pos += sizeof(int);

	//change the bytenum in the lastblock, add the bytes of the table and the valid four bytes(int)
	bytenum += (tablesize + sizeof(int) );
	memcpy(buffer.Buf[lastblock].BufBlock + BYTENUM_, &bytenum, sizeof(int) );
	
	//bulid index on primary key 
	if(tableinfo.attr.primary != -1)
	{
		string s = tableinfo.tableName + "_" + tableinfo.attr.attrName[tableinfo.attr.primary];
		this->addIndex(tableinfo.tableName, s, tableinfo.attr.attrName[tableinfo.attr.primary]);
	}
	
	cat[tableinfo.tableName] = tmp_byteoffset; 
	//write back the last block
	buffer.setUpdated(lastblock, true);
}

int Catalog::findTable(const string& tablename)
{
	if(cat.find(tablename) != cat.end())
	    return cat[tablename];
	
	//get the blocknum of catalog file
	int block = buffer.giveMeABlock("Catalog.bin", 0);
	int blocknum;
	memcpy(&blocknum, buffer.Buf[block].BufBlock + BLOCKNUM_, sizeof(int));
	
	//read the table in catalog one by one and record, lock the whole catalog file in buffer
	for(int i = 0; i < blocknum; i++)
	{
		block = buffer.giveMeABlock("Catalog.bin", i);
		buffer.setLocked(block, true);
		buffer_blocks.push_back(block);
		
		int bytenum;
		memcpy(&bytenum, buffer.Buf[block].BufBlock + BYTENUM_, sizeof(int));
		char* bufblock = buffer.Buf[block].BufBlock;
		
		int pos = (i == 0) ? (3*sizeof(int)) : (2*sizeof(int));
		while(pos < bytenum)
		{
			int valid, next;
			memcpy(&valid, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			int temp = pos;
			
			int bytes;  //the number of bytes of the table
			memcpy(&bytes, bufblock + pos, sizeof(int));
			next = pos + bytes;
			pos += sizeof(int);
			
			if(valid == 1)
			{	
				int length;  //the length of the table name
				memcpy(&length, bufblock + pos, sizeof(int));
				pos += sizeof(int);
				
				char* name = new char[length];  //the name of the table
				memcpy(name, bufblock + pos, length);
				
				string sname;  //record the tablename
				sname.assign(name, length);
				delete [] name;
				cat[sname] = temp + i * BLOCKSIZE;
			}
			
			pos = next;
		}
		
	}
	
	if(cat.find(tablename) != cat.end())
	    return cat[tablename];
	else
	    return -1;
}

void Catalog::deleteTable(const string& tablename)
{
	int byteoffset;
	if( (byteoffset = findTable(tablename) ) == -1)
	    throw runtime_error("Error: No table named " + tablename);
	
	int blockoffset = byteoffset / BLOCKSIZE;
	byteoffset = byteoffset % BLOCKSIZE;
	
	//update the information in the catalog.h
	int block = buffer.giveMeABlock("Catalog.bin", blockoffset);
	int valid = 0;
	memcpy(buffer.Buf[block].BufBlock + byteoffset - sizeof(int), &valid, sizeof(int) );
	buffer.setUpdated(block, true);
	
	if(cat.find(tablename) != cat.end())
	    cat[tablename] = -1;
	
	this->dropTableIndex(tablename);
}

Table Catalog::getTable(const string& tablename)
{
	int byteoffset;
	if( (byteoffset = findTable(tablename)) == -1)
	    throw runtime_error("Error: No table named " + tablename);
	
	Table table;
	int blockoffset = byteoffset / BLOCKSIZE;
	int pos = byteoffset % BLOCKSIZE + sizeof(int); //from the length of the table on
	
	int block = buffer.giveMeABlock("Catalog.bin", blockoffset);
	char* bufferblock = buffer.Buf[block].BufBlock;
	
	//the length of the table name
	int length;
	memcpy(&length, bufferblock + pos, sizeof(int) );
	pos += sizeof(int);
	
	//the name of the table
	char* c = new char[length];
	memcpy(c, bufferblock + pos, length);
	string s;
	s.assign(c, length);
	table.tableName = s;
	delete []c;
	pos += length;
	
	Attribute* pattr = new Attribute;
	
	//the number of the attributes
	int attrNum;
	memcpy(&attrNum, bufferblock + pos, sizeof(int) );
	pos += sizeof(int);
	pattr->attrNum = attrNum;
    
    //attribute length and name
	int alength;	
	for(int i = 0; i < attrNum; i++)
	{
		memcpy(&alength, bufferblock + pos, sizeof(int));
		pos += sizeof(int);
		
		char* c = new char [alength];
		memcpy(c, bufferblock + pos, alength);
		pos += alength;
		pattr->attrName[i].assign(c, alength);
		delete []c;
	}
	
	//attribute type
	memcpy(pattr->type, bufferblock + pos, attrNum*sizeof(int) );
	pos += attrNum*sizeof(int);
	//attribute unique
	memcpy(pattr->uniqueFlag, bufferblock + pos, attrNum);
	pos += attrNum;
	//attribute primary key
	memcpy(&(pattr->primary), bufferblock + pos, sizeof(int));
	pos += sizeof(int);
	
	table.attr = (*pattr);
	delete pattr;

	return table;
}

/*store in catIndex.bin:
the number of bytes in the block
the blockoffset of the block
the number of blocks in the catalog file in the first block
the valid of index;
the length of Index name; the Index name; 
the length of Table name; the Table name; 
the length of attribute name; the attribute name
 */

void Catalog::addIndex(const string& tablename, const string& indexname, const string& attriname)
{
	if(findIndex(indexname) != -1)
	    throw runtime_error("Error: duplicated index name!");
	if(findDup(tablename, attriname) == true)
	    throw runtime_error("Error: duplicated index bulit on the same attribute!") ;
	Table t = getTable(tablename);
	int i;
	for(i = 0; i < t.attr.attrNum; i++)
		if(t.attr.attrName[i] == attriname)
	         break;
	   
	if(i >= t.attr.attrNum)
	    throw runtime_error("Error: no attribute named " + attriname );
	if(t.attr.uniqueFlag[i] == false)
	    throw runtime_error("Error: attribute is not unique so that no index can built on it!");
	
	int firstblock = buffer.giveMeABlock("CatIndex.bin", 0);
	//bf.setLocked(firstblock, true);
	int blocknum;
	memcpy(&blocknum, buffer.Buf[firstblock].BufBlock + BLOCKNUM_, sizeof(int) );  //get the number of block in catIndex file
	
	//find the end of the catIndex file
	int lastblock;  
	if(blocknum != 1)
		lastblock = buffer.giveMeABlock("CatIndex.bin", (blocknum-1) );
	else
	    lastblock = firstblock;
	
	int bytenum;  //get the number of bytes in the last block
	memcpy(&bytenum, buffer.Buf[lastblock].BufBlock + BYTENUM_, sizeof(int));
	int size = 4*sizeof(int) + tablename.size() + indexname.size() + attriname.size();
	int pos;
	
	/*get the position of the new table record*/
	if(bytenum + size <= BLOCKSIZE) //the table information can be written in the last block
		pos = bytenum;
	
	else  //table information can't be added to the last block
	{
		blocknum++; //change the blocknum stored in the first block
		memcpy(buffer.Buf[firstblock].BufBlock + BLOCKNUM_, &blocknum, sizeof(int) );
		buffer.setUpdated(firstblock, true);
		
		//get an empty block and initialize
		lastblock = buffer.giveMeABlock("CatIndex.bin", blocknum-1);
		buffer.setLocked(lastblock, true);
		this->buffer_blocks.push_back(lastblock);
		
		int temp_bytenum = 2 * sizeof(int);
		memcpy(buffer.Buf[lastblock].BufBlock + BYTENUM_, &temp_bytenum, sizeof(int) );
		int temp_blockoffset = blocknum - 1;
		memcpy(buffer.Buf[lastblock].BufBlock + BLOCKOFFSET_, &temp_blockoffset, sizeof(int) );
		
		pos = 2 * sizeof(int);
	}	
	
	cat_index[indexname] = pos;
	//set valid = 1
	int valid = 1;
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &valid, sizeof(int));
	pos += sizeof(int);
	
	/*copy the information to catIndex file*/
	int ilength = indexname.size();
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &ilength, sizeof(int));
	pos += sizeof(int);
	
	memcpy(buffer.Buf[lastblock].BufBlock + pos, indexname.c_str(), ilength);
	pos += ilength;
	
	int tlength = tablename.size();
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &tlength, sizeof(int));
	pos += sizeof(int);
	
	memcpy(buffer.Buf[lastblock].BufBlock + pos, tablename.c_str(), tlength);
	pos += tlength;
	
	int alength = attriname.size();
	memcpy(buffer.Buf[lastblock].BufBlock + pos, &alength, sizeof(int));
	pos += sizeof(int);
	
	memcpy(buffer.Buf[lastblock].BufBlock + pos, attriname.c_str(), alength);
	pos += alength;
	
	bytenum += size;
	memcpy(buffer.Buf[lastblock].BufBlock + BYTENUM_, &bytenum, sizeof(int));
	
	//write back
	buffer.setUpdated(lastblock, true);
}

int Catalog::findIndex(const string& indexname)
{
	if(cat_index.find(indexname) != cat_index.end())
	    return cat_index[indexname];
	
	//get the blocknum of catIndex file
	int block = buffer.giveMeABlock("CatIndex.bin", 0);
	int blocknum;
	memcpy(&blocknum, buffer.Buf[block].BufBlock + BLOCKNUM_, sizeof(int));
	
	//read the table in catalog one by one and record
	for(int i = 0; i < blocknum; i++)
	{
		block = buffer.giveMeABlock("CatIndex.bin", i);
		buffer.setLocked(block, true);
		this->buffer_blocks.push_back(block);
		
		int bytenum;
		memcpy(&bytenum, buffer.Buf[block].BufBlock + BYTENUM_, sizeof(int));
		char* bufblock = buffer.Buf[block].BufBlock;
		
		int pos = (i == 0) ? (3*sizeof(int)) : (2*sizeof(int));
		while(pos < bytenum)
		{
			int tmp = pos;
			int valid;
			memcpy(&valid, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			int ilength;
			memcpy(&ilength, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			if(valid == 1)
			{
				char* cindex = new char[ilength];
				memcpy(cindex, bufblock + pos, ilength);
				string sindex;
				sindex.assign(cindex, ilength);
				delete[] cindex;
				cat_index[sindex] = tmp + i * BLOCKSIZE;
			}
			pos += ilength;
			
			int tlength;
			memcpy(&tlength, bufblock + pos, sizeof(int));
			pos += sizeof(int) + tlength;
			
			int alength;
			memcpy(&alength, bufblock + pos, sizeof(int));
			pos += sizeof(int) + alength;
		}
	    
	}
	
	if(cat_index.find(indexname) != cat_index.end())
	    return cat_index[indexname];
	else
	    return -1;
}

void Catalog::deleteIndex(const string& indexname)
{
	int byteoffset;
	if( (byteoffset = findIndex(indexname)) == -1)
	    throw runtime_error("Error: index named " + indexname + " doesn't exit");
	
	int blockoffset = byteoffset / BLOCKSIZE;
	byteoffset = byteoffset % BLOCKSIZE;
	
	int block = buffer.giveMeABlock("CatIndex.bin", blockoffset);
	int valid = 0;
	memcpy(buffer.Buf[block].BufBlock + byteoffset, &valid, sizeof(int));
	buffer.setUpdated(block, true);
	
	if(cat_index.find(indexname) != cat_index.end())
	    cat_index[indexname] = -1;
}

bool Catalog::findDup(const string& tablename, const string& attriname)
{
	//get the blocknum of catIndex file
	int block = buffer.giveMeABlock("CatIndex.bin", 0);
	int blocknum;
	memcpy(&blocknum, buffer.Buf[block].BufBlock + BLOCKNUM_, sizeof(int));
	
	//read the table in catalog one by one and record
	for(int i = 0; i < blocknum; i++)
	{
		block = buffer.giveMeABlock("CatIndex.bin", i);
		bool flag = false;
		
		int bytenum;
		memcpy(&bytenum, buffer.Buf[block].BufBlock + BYTENUM_, sizeof(int));
		char* bufblock = buffer.Buf[block].BufBlock;
		
		int pos = (i == 0) ? (3*sizeof(int)) : (2*sizeof(int));
		while(pos < bytenum && flag == false)
		{
			flag = false;
			int valid;
			memcpy(&valid, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			int ilength;
			memcpy(&ilength, bufblock + pos, sizeof(int));
			pos += sizeof(int) + ilength;
			
			int tlength;
			memcpy(&tlength, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			if(tlength == tablename.size())
			{
				char* ctable = new char[tlength];
				memcpy(ctable, bufblock + pos, tlength);
				string stable;
				stable.assign(ctable, tlength);
				delete[] ctable;
				if(stable == tablename)
				    flag = true;
			}
			pos += tlength;
			
			int alength;
			memcpy(&alength, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			if(alength == attriname.size() )
			{
				char* cattri = new char[alength];
				memcpy(cattri, bufblock + pos, alength);
				string sattri;
				sattri.assign(cattri, alength);
				delete[] cattri;
				if(valid == 1 && flag == true && sattri == attriname)
				    return true;
			}
			pos += alength;
			
		}
		
	}
	
	return false;
} 

void Catalog::dropTableIndex(const string& tablename)
{
	//get the blocknum of catIndex file
	int block = buffer.giveMeABlock("CatIndex.bin", 0);
	int blocknum;
	memcpy(&blocknum, buffer.Buf[block].BufBlock + BLOCKNUM_, sizeof(int));
	
	//read the table in catalog one by one and record
	for(int i = 0; i < blocknum; i++)
	{
		block = buffer.giveMeABlock("CatIndex.bin", i);
		buffer.setLocked(block, true);
		this->buffer_blocks.push_back(block);
		
		int bytenum;
		memcpy(&bytenum, buffer.Buf[block].BufBlock + BYTENUM_, sizeof(int));
		char* bufblock = buffer.Buf[block].BufBlock;
		
		int pos = (i == 0) ? (3*sizeof(int)) : (2*sizeof(int));
		while(pos < bytenum)
		{
			int tmp = pos;
			int valid;
			memcpy(&valid, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			int ilength;
			memcpy(&ilength, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			char* cindex = new char[ilength];
			memcpy(cindex, bufblock + pos, ilength);
			string sindex;
			sindex.assign(cindex, ilength);
			delete[] cindex;
			pos += ilength;
			
			int tlength;
			memcpy(&tlength, bufblock + pos, sizeof(int));
			pos += sizeof(int);
			
			char* ctable = new char[tlength];
			memcpy(ctable, bufblock + pos, tlength);
			string stable;
			stable.assign(ctable, tlength);
			delete[] ctable;
			pos += tlength;
			
			int alength;
			memcpy(&alength, bufblock + pos, sizeof(int));
			pos += sizeof(int) + alength;
			
			if(valid == 1 && stable == tablename)
			{
				valid = 0;
				memcpy(bufblock + tmp, &valid, sizeof(int));
				buffer.setUpdated(block, true); 
				cat_index[sindex] = -1;
			}
		}
	  
	}
}
