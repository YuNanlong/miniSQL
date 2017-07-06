#ifndef BASE_H
#define BASE_H

#include <string>
#include <vector>

#define BLOCKSIZE 4096 //DB files和buffer中一个block的大小是4096个字节
#define ISLEAF 0 //在存储B+树节点的block中isLeaf变量的首地址偏移量
#define KEYNUM 4 //在存储B+树节点的block中keyNum变量的首地址偏移量
#define DELFLAG 8 //在存储B+树节点的block中delFlag变量的首地址偏移量
#define BLOCKOFFSET 12 //在存储B+树节点的block中blockOffset变量的首地址偏移量
#define POINTERLEN 12 //在存储B+树节点的block中指向child节点的指针占用4个字节，
                    //在叶节点中，标记某个key所在的记录是否被删除，1表示被删除，0表示未被删除
                    //指向当前指针对应的key的下一个key的next指针
#define INDEXBLOCKINFO 16 //在存储B+树节点的block中所有节点基本信息所占的字节数

class Data{
public:
    int type;

    virtual ~Data(){}
};

class Datai: public Data{
public:
    int val;

    Datai(int i)
        :val(i) {type = -1;}
    virtual ~Datai(){}
};

class Dataf: public Data{
public:
    float val;

    Dataf(float f)
        :val(f) {type = 0;}
    virtual ~Dataf(){}
};

class Datac: public Data{
public:
    std::string val;

    Datac(std::string str)
        :val(str){
        type = val.length();
        if(0 == type){
            type = 1;
        }
    }
    virtual ~Datac(){}
};
/* Pos类表示了一个tuple在存储表的文件中的位置 */
class Pos{
public:
    std::string tableFileName; //存储表的文件的文件名
    int blockOffset; //tuple所在的block在表文件中的偏移量
    int pos; //tuple在所在的block中的偏移量

    //根据一个tuple所在的block在表文件中的偏移量和tuple在block中的偏移量计算出tuple相对于整个文件的偏移量
    int transOffset(){
        return blockOffset * BLOCKSIZE + pos;
    }
};

class Tuple: public Pos{
public:
    std::vector<Data *> element;

    ~Tuple();
    void outputTuple();
};

class Index{
public:
    int indexNum; //number of index
    bool indexPos[32]; //attri index built 
    std::string indexName[32]; //name of index
    Index()
    {
    	indexNum = 0;
    	for(int i = 0; i < 32; i++)
    	    indexPos[i] = false;
	}
	~Index(){
	}
};

class Attribute{
public:
    int attrNum; //the number of attris
    std::string attrName[32];
    int type[32]; //type of attris; -1 --int, 0 --float, 1-255 --char
    bool uniqueFlag[32]; //if the corresponding attributes is unique
    int primary; //number of primary key, -1 means no primary key
    Attribute()
    {
    	attrNum = 0;
    	primary = -1;
    	for(int i = 0; i < 32; i++)
    	{
    		type[i] = -1;
			uniqueFlag[i] = false;
		} 
	}
};

class Table{
public:
    std::string tableName;
    Attribute attr;
    Index index;
    std::vector<Tuple *> tuple;
    int blockNum;
    
    Table()
    {
    	blockNum = 0;
    	tuple.clear();
	}
	
	int tableSize()
	{
		int num = 4; //the number of bytes of the table
		num += 4;  //the length of the table name
		num += tableName.size();
		num += 4;  //the number of the attributes
	    for(int i = 0; i < attr.attrNum; i++)
	    {
	    	num += 4; //the length of the attributes
	    	num += attr.attrName[i].size();
		}
		num += attr.attrNum * 4; //type of all the attributes
		num += attr.attrNum;  //unique of all the attributes
		num += 4; //the index of primary key
		
		return num;
	}

    ~Table();
    void outputTable();
};

/*class Exception: public std::exception{
public:
    Exception(std::string info)
        :exceptionInfo(info){}
    std::string getExceptionInfo(){
        return exceptionInfo;
    }
private:
    std::string exceptionInfo;
};*/

typedef enum{
    E, SE, BE, S, B, NE
} Symbol;

struct condition{
    Data* message;
    Symbol flag;
    int number;
};

#endif // BASE_H
