#include <stdio.h>
#include <time.h>
#include <string>
#include <stdexcept>
#include <vector>
#include "base.h"
#include "buffer.h"
#include "base.h"

using namespace std;

//initialization
BufferManage::BufferManage()
{
	for(int i = 0; i < BLOCKNUM; i++)
	    Buf[i].clearBufBlock();
}

//write all updated blocks back
BufferManage::~BufferManage()
{
	for(int i = 0; i < BLOCKNUM; i++)
	    WriteBack(i);
}

//find an empty block, if no, use LRU to get an empty one
int BufferManage::giveMeAnEmptyBlock()
{
	int LRU;
	bool temp = false; 
	
	for(int i = 0; i < BLOCKNUM; i++) //find invalid block, LRU block, MRU block
	{
		if(Buf[i].IsValid == false)
			return i;	
		
		else if(Buf[i].IsValid == true && Buf[i].IsLocked == false)
		{
			if(temp == false)
			{
				LRU = i;
				temp = true;
			}
			else
			{
				if(Buf[i].RecentTime < Buf[LRU].RecentTime)
					LRU = i;
			}
		}
	}
	
	if(temp == true)
	{
		WriteBack(LRU);
		return LRU;
	}
	
	else
	{
		throw runtime_error("Error: buffer blocks run out !");
	}
}

//read file block from disk to buffer
int BufferManage::readMemToBuf(const string& file_name, int block_offset)
{
	int empty = giveMeAnEmptyBlock();  //get an empty block in buffer
	
	FILE* fp;
	if( (fp = fopen(file_name.c_str(), "rb") ) == NULL)   //open the corresponding file
	{
		string ex_info = "file named " + file_name + " open failed!";
		throw runtime_error( ex_info );
	}
	
	int t;
	if( (t = fseek(fp, (long)(block_offset * BLOCKSIZE), SEEK_SET) ) == -1) //move the file point to the correct position
	{
		string ex_info = "move fp in " + file_name + " failed!";
		throw runtime_error(ex_info );
	}
	
	if( (t = fread(&Buf[empty], BLOCKSIZE, 1, fp) ) != 1 && ferror(fp) != 0 )  //read file into buffer block
	{
		string ex_info = "read file named " + file_name + " failed!";
		throw runtime_error(ex_info);
	}
	
	/*if(this->usedBlockNum <= BLOCKNUM - 3)  //spatial locality
	{
		int empty2 = giveMeAnEmptyBlock(flag, file_name, block_offset + 1);
		fseek(fp, (long)( (block_offset+1) * BLOCKSIZE), SEEK_SET);
		fread(&Buf[empty2], BLOCKSIZE, 1, fp);
		Buf[empty2].fileName = file_name;
		Buf[empty2].blockOffset = block_offset + 1;
	}*/
	
	fclose(fp);
	
	Buf[empty].IsValid = true;
	Buf[empty].RecentTime = time(NULL);
	Buf[empty].fileName = file_name;
	Buf[empty].blockOffset = block_offset;
	
	return empty;
}

//judge whether the file block is in buffer
int BufferManage::IsInBuf(const string& file_name, int block_offset)
{
	for(int i = 0; i < BLOCKNUM; i++)
	    if(Buf[i].IsValid == true && Buf[i].fileName == file_name && Buf[i].blockOffset == block_offset)
	        return i;
	
	return -1;
}

//outer interface: get the file block in disk from buffer
int BufferManage::giveMeABlock(const string& file_name, int block_offset)
{
	int num;
	if( (num = IsInBuf(file_name, block_offset) ) >= 0 )
	{	
		Buf[num].RecentTime = time(NULL);
		return num;
	}
	
	else
	{
		int block =  readMemToBuf(file_name, block_offset);
		return block;
	}
}

//write buffer block back to the file and clear the block
void BufferManage::WriteBack(int block_num)
{	
	if(Buf[block_num].IsUpdated == true)
	{
		FILE* fp;
		string file_name = Buf[block_num].fileName;
		
		if( (fp = fopen(file_name.c_str(), "rb+")) == NULL)   //open the corresponding file, file must exit
		{
			string ex_info = "file named " + file_name + " open failed!";
			throw runtime_error( ex_info );
		}
		int t;
		if( (t = fseek(fp, (long)(Buf[block_num].blockOffset * BLOCKSIZE), SEEK_SET) ) == -1) //move the file point to the correct position
		{
			string ex_info = "move fp in " + Buf[block_num].fileName + " failed!";
			throw runtime_error(ex_info );
		}
		
		fwrite(&Buf[block_num], BLOCKSIZE, 1, fp);
		
		fclose(fp);
	}
	
	Buf[block_num].clearBufBlock();
	return ;
}

void BufferManage::setUpdated(int block_num, bool flag)
{
	Buf[block_num].IsUpdated = flag;
}

void BufferManage::setLocked(int block_num, bool flag)
{
	Buf[block_num].IsLocked = flag;
}

void BufferManage::createFile(const string& file_name)
{
	FILE* fp;
	if( (fp = fopen(file_name.c_str(), "wb") ) == NULL )
	{
		throw runtime_error("file named " + file_name + " open failed!");
	}
	
	fclose(fp);
} 

void BufferManage::readFileIn(int* blocks, const string& file_name, int block_num)
{
	int num;
	for(int i = 0; i < block_num; i++)
	{
		num = giveMeABlock(file_name, i);
		blocks[i] = num;
	}
}

void BufferManage::WriteFileBack(int* blocks, int block_num)
{
	for(int i = 0; i < block_num; i++)
	    WriteBack(blocks[i]);
}

void BufferManage::setInvalid(const string& file_name)
{
	for(int i = 0; i < BLOCKNUM; i++)
	    if(Buf[i].fileName == file_name)
	        Buf[i].clearBufBlock();
}

string BufferManage::giveMeAString(int block_num, int byte_offset, int length)
{
	
	char temp[length+1];
	for(int i = 0; i < length; i++)
		temp[i] = Buf[block_num].BufBlock[byte_offset + i];
	temp[length] = '\0';
	
	string s(temp);
	return s;
}
