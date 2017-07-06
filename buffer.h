#ifndef _BUFFER_H
#define _BUFFER_H

#include <time.h>
#include <cstring>
#include <string>
#include "base.h"

const int BLOCKNUM = 15; // 15 block/1 buffer

class Buffer{
public:
	Buffer(){
		clearBufBlock();
	}
	~Buffer(){
	}
public:
	char BufBlock[BLOCKSIZE];
private:
    std::string fileName;
	int blockOffset;
	bool IsValid;
	bool IsUpdated;
	bool IsLocked;
	time_t RecentTime;
private:
	void clearBufBlock(){
		IsValid = false;
		IsUpdated = false;
		IsLocked = false;
	//	memset(BufBlock, 0, BLOCKSIZE);
	}
	
friend class BufferManage;
}; 

class BufferManage{
public:
	BufferManage();
	~BufferManage();
    int giveMeABlock(const std::string& file_name, int block_offset);
    std::string giveMeAString(int block_num, int byte_offset, int length);
	void WriteBack(int block_num);   //write Buf[block_num] back to file
	void createFile(const std::string& file_name);
	void readFileIn(int* blocks, const std::string& file_name, int block_num);
	void WriteFileBack(int* blocks, int block_num);
	void setInvalid(const std::string& file_name);
	void setUpdated(int block_num, bool flag);
	void setLocked(int block_num, bool flag);
public:
	Buffer Buf[BLOCKNUM];
private:
    int readMemToBuf(const std::string& file_name, int block_offset);
    int IsInBuf(const std::string& file_name, int block_offset);
    int giveMeAnEmptyBlock();
};

#endif
