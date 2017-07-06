<div align = "center"><h1>MiniSQL总体设计报告<h1></div>

## 1. MiniSQL系统概述

### 1.1 背景

#### 1.1.1 编写目的

在文件系统的基础上设计并实现一个精简型单用户SQL引擎(DBMS)MiniSQL。通过对MiniSQL的设计与实现，深入了解模式定义、查询处理、索引管理、存储管理等相关实现技术，加深对数据库系统原理的理解，提高自身的系统编程能力，实践系统软件开发的工程化方法。

#### 1.1.2 项目背景

数据库系统课程设计

### 1.2 功能描述

+ 总功能：允许用户通过字符界面输入SQL语句实现表的建立/删除；索引的建立/删除以及表记录的插入/删除/查找。
+ 数据类型：支持三种基本数据类型：int，char(n)，float，其中char(n)满足1 ≤ n ≤ 255。
+ 表定义：一个表最多可以定义32个属性，各属性可以指定是否为unique；支持单属性的主键定义。
+ 索引的建立和删除：对于表的主属性自动建立B+树索引，对于声明为unique的属性可以通过SQL语句由用户指定建立/删除B+树索引（因此，所有的B+树索引都是单属性单值的）。
+ 查找记录：可以通过指定用and连接的多个条件进行查询，支持等值查询和区间查询。
+ 插入和删除记录：支持每次一条记录的插入操作；支持每次一条或多条记录的删除操作。
+ 语法说明：MiniSQL支持标准的SQL语句格式，每一条SQL语句以分号结尾，一条SQL语句可写在一行或多行，所有的关键字都为小写。具体语法请参照标准SQL的语法。

### 1.3 运行环境和参考配置

本MiniSQL使用C++语言开发，并在其中使用了C++正则表达式相关的库函数，因此需要支持C++11以上（包含\<regex\>头文件）的编译器进行编译，其它方面只要能运行编译获得的可执行文件即可。

### 1.4 参考资料

Abraham Silberschatz Henry F. Korth S. Sudarshan.Database System Concepts (Six Edition).McGraw-Hill Companies

## 2. MiniSQL系统结构设计

### 2.1 总体设计

#### 软件体系结构图

![][image-1]

#### 模块概述

**Interpreter 模块**

Interpreter模块负责读取用户的输入，在将输入格式化之后，利用语法树的相关知识，检测输入语法的正确性并提取出需要的语块信息作为参数传递给API，对输入的解析过程如下图所示，其中input是已经格式化的输入：

![](https://ws2.sinaimg.cn/large/006tNbRwgy1fg9dmkugr6j31fi0son28.jpg)

**API模块**

API模块负责衔接Interpreter模块和下层模块，通过调用Catalog Manager、Record Manager、Index Manager模块提供的接口，实现相应的功能，并封装为API模块的接口。Interpreter模块通过将解析获得的语块信息作为参数来调用API的接口。

**Catalog Manager 模块**

Catalog模块的设计目的是为了存储和管理数据库的元数据。具体包括：表的定义信息（表名，属性数量，主键等），属性的定义信息（数据类型，是否唯一），索引的定义信息（索引名，所属表名，所属属性名）。

本次工程的设计中，Catalog模块所管理的元数据将被存放在两个不同的文件中。其一，命名为“Catalog.bin”，用于存放表的定义信息和表中属性的定义信息；其二，命名为“CatIndex.bin”，用于存放索引的定义信息。

由于Catalog模块的两个文件所占内存不多且使用频繁，更改频繁，因此整个程序运行的过程中，Catalog模块的两个文件将被锁在内存中，直到程序结束才被写回。

**Record Manager 模块**

Record Manager负责对记录表中数据的数据文件进行管理，按照表的创建和删除实现对相应数据文件的创建和删除，同时根据表的信息和记录信息实现相应记录的删除、查询和插入功能。并提供相应的接口供API模块调用。

该模块主要功能是软件与数据文件的交互，每次从interpreter处获取要处理的表名及其属性等主要信息，然后通过Buffer Manager加载所需要的文件数据到内存中，将内存一个或多个block中的数据转换成更加易于比较和判断的元组数据形式。建立或删除表时，建立相应的表文件并通过Record Manager进行写入信息等相应操作。

**Index Manager 模块**

Index Manager模块负责索引的管理。该模块有两个类：Indexmanager类和BpTree类，其中Indexmanager类是模块的接口，BpTree类实现了B+树这一数据结构，Indexmanager类的接口函数通过DB file中的索引文件初始化BpTree类的对象来调用BpTree类的公有成员函数进行关键字的插入、删除、等值查找和区间查找。同时Index Manager模块通过Buffer Manager模块和DB file进行交互，将一个块作为B+树的一个节点进行存取。

**Buffer Manager 模块**

Buffer模块是直接操作DB files的模块，负责缓冲区的管理。作为文件管理模块与硬盘上存储文件的接口，读操作中，文件管理模块根据文件名和文件块的块偏移量，调用Buffer函数获得相应缓冲区的编号，并根据编号对相应缓冲区进行操作。写操作中，文件管理模块根据相应缓冲区的编号，调用Buffer函数将相应缓冲区的内容写回文件。
本次工程的设计中，一个buffer block 的大小被设计为4KB。替换算法采用LRU算法。同时提供内存锁，被锁住的缓冲区块将不会被替换出去。

<hr/>

### 2.2 Interpreter 模块

**注：该模块介绍语法的部分，所有被'\<...\>'包含的内容（包括\<\>）在实际输入中均需被替换。**

**格式化输入**

该部分负责将用户输入进行格式化，首先在interpreter模块中有一个字符串成员变量用于存储格式化后的字符串，一个一个的依次从标准输入中读取字符，其中字母、下划线和句点'.'直接存入成员变量中；星号'\*'逗号','括号'()'以及比较符号'\<\>='在存入成员变量时保证其前后有且仅有一个空格；遇到单引号'则直接跳过；遇到空格' '制表符'\t'和换行符'\n'时，如果此时成员变量的最后一个字符是空格则直接跳过，不是空格则存入一个空格；遇到分号';'则保证存入成员变量时前面有且仅有一个空格；遇到非法字符则报错。

**创建表**

通过input函数得到关键字和符号被单个空格分隔的标准输入流。利用C++标准库中对数据流的操控类和操控函数处理标准输入流。由语法树的总体设计思路可知，进入create table函数后，只需从create table后的空格开始判断语法。

create table的标准语法为：

```sql
create table <表名> （
<属性名> <属性类型> unique ,
<属性名> <属性类型> unique ，
…
<属性名> <属性类型> unique
primary key ( <属性名> ) 
）;
```

其中最后’)’前一行无逗号，其他行用逗号分隔；下划线内容为可选约束；最后一行定义语句要么是属性定义要么是主键约束；主键约束只能有一句。

因此，利用一个循环进行判断，每次判断的大单元为一行定义语句。同时大单元有两个分支，对属性的定义和主键约束，大单元之间用逗号分隔，读到最后一个’)’时退出循环。
在循环中依次处理每个大单元的语法判断，包括字符串比较和使用正则表达式对属性名（及表名）进行字符约束等。

在判断语法的过程中，将表的定义信息，包括属性的定义信息存储在table类中。判断完语法之后以table类为参数调用API接口，执行语句即可。

**删除表**

删除表的标准语法为：

```sql
drop table <表名> ;
```

因此，根据语法树，到达drop table函数时，只需判断table后的空格，获取表名，对表名利用正则表达式做字符检查，判断分号即完成了语法检查的工作。语法检查通过后以table名作为参数调用API接口，执行语句即可。

**创建索引**

创建索引的标准语法为：

```sql
create index <索引名> on <表名>(<属性名>) ;
```

其中用于创建索引的属性必须为unique或primary约束，并且创建的索引名不能与已经存在的索引名重复，否则MiniSQL将无法完成索引的创建并返回错误信息。

因为将创建索引的语句格式化之后，语块的数量是一定的，所以在具体实现中只需要依次读取格式化后的输入中的语块，并依次与标准语法相应位置上的语块进行比对：on和()这样的语块必须一致，否则报错；索引名、表名和属性名位置上的语块则直接读取存入变量中，传递给API由下层模块处理；如果实际输入的语句块的数量与标准语法的语句块数量不一致，也将报错、最后对于索引名还需要利用正则表达式检查索引名中是否包含非法字符。

**删除索引**

删除索引的标准语法为：

```sql
drop index <索引名> ;
```

其中被删除的索引名必须已经存在且与表名相互对应，否则MiniSQL将无法完成索引的删除并返回错误信息。

这里的实现比较简单，只需要读取索引名位置上的语句块存入变量中传递给API进行处理，因为将删除索引的语句格式化之后，语块的数量是一定的，如果实际输入的语句块的数量与标准语法的语句块数量不一致，也将报错。

**执行脚本文件**

执行脚本文件的标准语法为：

```sql
execfile <文件名> ;
```

其中文件名可以由二十六个英文字母、数字、下划线和句点'.'组成，且必须连续，任何其他字符均视作非法。

具体实现中，只需要读取文件名位置上的语句块存入变量中传递给API进行处理，因为将执行脚本文件的标准语法格式化之后，语块的数量是一定的，如果实际输入的语句块的数量与标准语法的语句块数量不一致，也将报错。

**查询**

标准语法为：

```sql
select * <列名> from <表名> ;
select * <列名> from <表名> where <条件>;
```

其中\<条件\>具有以下格式：\<列\>\<op\> \<值\> and \<列\>\<op\> \<值\>…

\<op\>是比较符：=, \<\>, \<, \>, \<=, \>=

**插入**

标准语法为：

```sql
insert into values (<值1>, <值2>, …, <值N>);
```

**删除**

标准语法为：

```sql
delete (*) from <表名> ;
delete (*) from <表名> where <条件>;
```

**退出**

标准语法为：

```sql
quit;
```

<hr/>

### 2.3 API 模块

#### 主要功能

封装Record Manager、Index Manager、Catalog Manager模块的接口。

#### 接口设计

```c++
void createTable(Table& tableinfo);
void dropTable(const string& tablename);
void createIndex(const string& indexName, const string& tableName, const string& attrName);
void dropIndex(const string& indexName);
void insert(const Table& tableInfo, Tuple& line);
int del(const Table& tableInfo, vector<condition> comp);
Table select(Table& tableInfo, vector<int> attrIndex, vector<condition> comp);
```

#### 设计思路和实现方法

```c++
#include "API.h"
#include "base.h"
#include "Catalog.h"
#include "recordmanager.h"
#include "buffer.h"

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
	IndexManager im;
	Catalog ca;

	ca.addIndex(tableName, indexName, attrName);
	im.createIndex();
}

void API::dropIndex(const string& indexName){
	IndexManager im;
	Catalog ca;

	ca.deleteIndex();
	im.dropIndex();
}

void API::insert(const Table& tableInfo, Tuple& line){
	RecordManager rm(&buffer);

	rm.InsertValue(tableInfo, line);
}

int API::del(const Table& tableInfo, vector<condition> comp){
	RecordManager rm(&buffer);

	return rm.Delete(tableInfo, line);
}

Table API::select(Table& tableInfo, vector<int> attrIndex, vector<condition> comp){
	RecordManager rm(&buffer);

	return rm.SelectWithWhere(tableInfo, attrIndex, comp);
}
```

### 2.4 Catalog Manager 模块

#### 主要功能

1. 在“Catalog.bin”文件中，记录新创建表的元数据。同时判断表名是否已经被使用过，如果是，则抛出异常给调用模块；如果表中定义了主键，自动记录主键上创建的索引的信息。
2. 在“Catalog.bin”文件中，记录删除表的操作。
3. 利用“Catalog.bin”文件，返回表的元数据。
4. 在“CatIndex.bin”文件中，记录新创建索引的元数据。如果索引名已经被使用过，或索引所属属性上已经建立过索引，或索引所属属性在所属表中不存在，则抛出异常给调用模块。

#### 接口设计

**外部接口**

```c++
/*利用Table类参数记录新创建表的元数据*/
void addTable(Table& tableinfo); 
/*根据tablename记录删除表的操作*/
void deleteTable(const string& tablename);  
/*根据tablename得到表的元数据（不含索引信息）*/
Table& getTable(const string& tablename); 
/*利用表名，索引名，属性名记录新创建索引的元数据*/
void addIndex(const string& tablename, const string& indexname, const string& attriname);
/*利用索引名记录删除索引的操作*/
void deleteIndex(const string& indexname);
/*创建Catalog模块文件*/
void createFile(const string& file_name);
/*根据文件名和blockoffset得到相应的buffer block*/
int giveMeABlock(const std::string& file_name, int block_offset);
/*将Catalog模块锁在内存中*/
void setLocked(int block_num, bool flag);
/*标记脏数据*/
void setUpdated(int block_num, bool flag);
/*将buffer block中的数据写回文件*/
void WriteBack(int block_num);
```

**内部接口**

```c++
/*根据tablename返回相应table在“Catalog.bin”文件中的byteoffset*/
int findTable(const string& tablename);
/*根据indexname返回相应index在“CatIndex.bin”文件中的byteoffset*/
int findIndex(const string& indexname);
/*根据表名和属性名，判断该属性上是否已经建立过索引*/
bool findDup(const string& tablename, const string& attriname);
/*查询两个索引文件是否存在于硬盘上，没有则建立索引文件*/
void initialize();
/*构造函数：初始化成员变量和调用initialize函数*/
Catalog();
/*析构函数：clear成员变量*/
~Catalog();
```

**成员变量**

```c++
/*记录表名与对应byteoffset*/
map<string, int> cat;
/*记录索引名与对应offset*/
map<string, int> cat_index;
/*记录两个文件所占用的buffer block*/
vector<int> buffer_blocks;
```

#### 设计思路

**“Catalog.bin”文件中每个block的组织形式如下：**

```
该block的字节数bytenum（int）；
该block在文件中的偏移量blockoffset (int);
文件的block总数（只出现在文件的第一个block）(int);
每个表的元数据：
有效位（标记该表有没有被删除）（int：1 for valid, 0 for invalid）；
该表元数据所占字节数（int，不包括有效位所占字节数）；
表的名字的长度（int）;
表的名字(char[表的名字的长度]);
表的属性的个数（int）;
每个属性的元数据：
属性名的长度（int）;
属性名（char[属性名的长度]）；
属性的类型(int[表的属性的个数]) //备注：-1 for int; 0 for float; 1-255 for char*
属性的唯一性（bool[表的属性的个数]） //备注：true for unique
主键的属性下标（int）备注：-1 for no primary key
```

**“CatIndex.bin”文件中每个block的组织形式如下：**

```
该block的字节数bytenum（int）；
该block在文件中的偏移量blockoffset (int);
文件的block总数（只出现在文件的第一个block）(int);
每个索引的元数据：
有效位（标记该索引有没有被删除）（int：1 for valid, 0 for invalid）；
索引名的长度（int）;
索引名（char[索引名的长度]）；
所属表名的长度（int）;
所属表名(char[所属表名的长度]);
所属属性的长度（int）；
所属属性名（char[所属属性的长度]）;
```

#### 实现方法

**在程序调用的开始：**

判断两个文件是否已经存在于硬盘上，如果不存在，调用buffer中的函数创建相关文件。

**在程序运行过程中：**

addTable函数，deleteTable函数，getTable函数都首先调用findTable函数，返回table对应文件中的byteoffset。

在第一次调用findTable函数的时候将读进整个“Catalog.bin”文件，并将文件的所有block锁在内存中，并记录在成员变量buffer\_blocks中（vector类型）。同时，在遍历整个“Catalog.bin”文件的过程中建立表名和表的byteoffset的映射并存在成员变量cat中（map类型）。之后访问findTable就可以直接通过cat变量快速返回table的byteoffset，如果不存在table返回-1.

在addTable函数中，查重后，首先找到插入新表的位置（blockoffset和byteoffset），将Table类中的信息按顺序写入buffer block中，并将valid位置1。修改该block的bytenum，将写入的buffer block setUpdated。同时将新表与其byteoffset记录在cat中。如果文件新增了一个block，则要将文件的第一个block的blocknum增1，同时将该block记录在成员变量buffer\_blocks中（vector类型）并锁在内存中。

在deleteTable函数中，检查table是否存在后，利用findTable快速定位表的位置并将valid位置0.

在getTable函数中，定位后，做与addTable相反的memcpy操作，并返回一个Table类。

基于“CatIndex.bin”文件，对Index的元数据进行操作的思路与Table大同小异。主要是增加了一个根据表名和属性名，判断该属性上是否已经建立过索引操作，需要每次addIndex的时候从头遍历“CatIndex.bin”文件直到匹配表名和属性名。

**在程序结束时：**

将两个文件相关的所有buffer\_block写回文件中。

<hr/>

### 2.5 Record Manager 模块

#### 主要功能

1.  数据文件的创建与删除：根据表名建立或删除原有文件，并根据新建立的表中的属性，判断是否有unique属性的键，调用Index Manager建立相关索引文件。删除时同样将相应索引信息一并删除。
2.  数据的删除与查找：删除和查找数据时先判断是否有unique属性键的等值查找，如果有且对应索引存在，调用Index Manager通过索引快速查找。若不满足条件就对整表进行遍历。对搜索到的符合条件的元组，如要实行删除操作则采用懒惰删除将该条元组标记为“已删除”；如要实行查找操作就将该元组插入到结果中，根据想要显示的属性生成一张新的表并对最终生成的表进行返回操作。
3.  数据的插入：插入数据时根据该数据的主键与unique属性，利用现有的索引在原表中查询是否有重复，如果没有重复则将元组数据转换成字符串数据写入到内存中，由Buffer Manager将内存中数据写回文件。

#### 接口设计

**外部接口**

```c++
bool tableCreate(Table& tableInfo);
//建立数据文件
bool tableDrop(Table& tableInfo);
//删除数据文件
Table SelectWithWhere(Table& tableInfo, vector<int>attrIndex, vector<condition> comp);
//记录的查询
Table Delete(Table& tableInfo, vector<condition> comp);
//记录的删除
void InsertValue(Table& tableInfo, Tuple& line);
//记录的插入
```

**内部接口**

```c++
bool MeetCondition(Table& tableInfo, Tuple* line, vector<condition> comp);
//判断元组是否符合条件
Table PrintAnswer(Table& tableInfo, vector<int>attrIndex);
//按用户需求属性投影符合条件元组
Tuple* FindWithIndex(Table& tableInfo, int uniqueNum, Data* uniqueKey, int tupleLength, int* posKey);
//在索引中查找
Tuple* Line2Tuple(string line, Table& tableInfo);
//字符串转元组
char* Tuple2Line(Table& tableInfo, Tuple& line, int tupleLength);
//元组转字符串
```

#### 设计思路

文件按照表名进行组织，每个文件的前12个byte记录表中现有的块数，元组数（包含懒惰删除的元组），元组的长度。为了保证稳定性，元组采用定长在文件中进行储存，字符串类型数据按最大长度进行转换储存，int，float类型数据拷贝到对应长度的字符串中。在数据文件中，每条元组前有1个byte标记该元组是否被删除。删除时采用懒惰删除的办法，对用户提出删除的元组进行删除标记。

#### 实现方法

**Create Table:**

```c++
bool tableCreate(Table& tableInfo);
```

根据表名创建一个新的数据文件，所需表的参数由Interpreter（其属性信息由catalog）提供，根据属性信息计算出数据文件中实际每条元组的长度，将buffer中块数，元组数（当前元组数为零），元组长度写入内存中数据文件对应的块中。由Unique属性生成对应的索引文件名，调用Index Manager建立对应索引。由Buffer Manager将内存写入数据文件。

**Drop Table**

```c++
bool tableDrop(Table& tableInfo);
```

删除table对应的数据文件，并把内存中所有相关的block设置为无效。删除对应的索引文件。

**Select**

```c++
Table SelectWithWhere(Table& tableInfo, vector<int>attrIndex, vector<condition> comp);
```

函数第一个参数是表头信息，第二个参数是需要投影的属性vector，第三个参数是where条件vector。
选择操作的条件有以下三种情况：
Where条件为空，对整表调用PrintAnswer函数，按用户所需属性进行投影。
Where条件为等值查找，且该属性已建索引，调用FindWithIndex函数利用索引返回目标元组在数据文件中的偏移量，根据偏移量从内存中读取出该条记录的字符串，转换成Tuple，按用户所需属性进行投影。
Where条件非等值查找，或该属性无索引，调用MeetCondition函数判断每条元组是否符合一个或多个条件。将符合条件的元组加入到待PrintAnswer处理的表中。

**Delete**

```c++
Table Delete(Table& tableInfo, vector<condition> comp);
```

查找的过程与Select操作相同，采用懒惰删除的方式，将待删除元组的删除标记位置为1。删除一个块中的多条后Buffer Manager将其一次性写回。

**Insert**

```c++
void InsertValue(Table& tableInfo, Tuple& line);
```

对Interpreter传入的参数Tuple中的primary key和Unique key依次调用Index Manager判断是否有重复，如果没有重复，调用Tuple2Line函数将其转换成待插入的字符串，在原表数据中找到懒惰删除留下的空缺位置将其插入，若无空缺位置，将采用元组数和元组长度数据计算出数据文件尾部元组偏移量，并将数据成功插入。插入后调用Buffer Manager将其立即写回。

<hr/>

### 2.6 Index Manager 模块

#### 主要功能

1. 创建索引
2. 删除索引
3. 向索引中插入关键字
4. 从索引中删除关键字
5. 在索引中进行等值查找和区间查找
6. 向表中插入记录时检查unique约束

#### 接口设计

**外部接口**

```c++
void createIndexFile(std::string fileName, int keyType); //初始化索引文件
bool insertKey(std::string fileName, Data *key, int row); //向索引插入关键字
bool deleteKey(std::string fileName, Data *key); //从索引删除关键字
int findKey(std::string fileName, Data *key); //在索引中进行等值查询
std::vector<int> findKeyRange(std::string fileName, Data *leftKey, Data *rightKey); //在索引中进行区间查询
void createIndex(std::string fileName); //创建索引
void dropIndex(std::string fileName); //删除索引
```

**内部接口**

```c++
BpTree(std::string fileName);
~BpTree();
bool insert(Data *key, int row); //B+树插入
bool lazyDelete(Data *key); //B+树懒惰删除
int find(Data *key); //B+树等值查找
std::vector<int> findRange(Data *leftKey, Data *rightKey); //B+树区间查找
char* initBlock(int isLeaf, int keyNum, int blockOffset); //初始化节点块
int searchInNode(char *block, Data *key); //节点内搜索关键字
void split(char *fatherBlock, int childOffset, int insertPos); //分裂节点
int search(Data *key, int *leafBlockOffset); //树中搜索关键字
```

**成员变量**

```c++
public:
    int number; //索引文件中存在的block的数量
private:
    std::string indexFileName; //索引文件的文件名
    int keyType; //用于构建索引的属性的类型，其值为keyType枚举类中的值
    int keyLength; //用于构建索引的属性的类型占用的内存字节数
    int rootBlockOffset; //在索引文件中存储根节点的block的偏移量
    int minDegree; //B+树的最小度，详情参考算法导论B树部分
```

#### 设计思路

索引模块主要是B+树的实现。每一个索引都在DB file中以一个独立文件的形式存储，而索引文件中的数据以一个个独立的块（block）为单位进行存储，一个块的大小是4096个字节，块首与文件开头之间相距的字节数/4096即为块偏移量，每一个块都可以在文件中由块偏移量精确定位。

索引文件的第0个块存储整个索引的一些总体信息，第0到3字节记录索引的关键字类型，-1表示int，0表示float，1-255表示char(n)，第4到7字节记录关键字所占用的字节数，第8到11字节记录该索引的B+树根节点的块偏移量，通过该偏移量即可访问表示B+树的根节点的块，第12到15字节记录索引文件中块的数量，第16到19字节记录该索引的B+树的最小度（下文会详细解释这个概念），第20到第23字节记录该索引是否被建立，0表示被建立，1表示被删除，即没有建立，剩余第24到4095字节中为无效数据。如下图所示：

![][image-2]

在索引文件中除了第0个块被保留用于存储索引的总体信息之外，其余每一个块均对应B+树中的一个节点，其中内节点和叶节点的块头信息的存储是一致的：第0到3字节记录该块中存储的是否为叶节点，0为内节点，1为叶节点，第4到7字节记录该节点中的关键字个数，第8到11字节记录该块在索引文件中的块偏移量。除了块头信息之外，内节点和叶节点在块中的存储方式有所不同。

首先是内节点。B+树的内节点中指向子节点的“指针”比关键字个数多一个，然而在具体实现中，为了高效实现关键字在节点中的插入，我将一个节点中的所有关键字串联成一个“链表”，因此除了指向子节点的“指针”之外，还应该有一个指向下一个关键字的“指针”，又因为B+树采用懒惰删除的方式，因此在叶节点中还应该有一个标记来记录一个关键字是否被删除，虽然内节点中的关键字并不需要这个标记，但为了具体实现的方便，我们仍将这个标记保留作为无效数据。综上所述，块中除了块头信息之外，还有两个主要的存储单元——关键字和“关键字信息”，其中“关键字信息”占12个字节，前4个字节记录子节点的块偏移量，中间4个节点为无效数据，最后4个节点记录下一个关键字在块中的偏移量。由于“关键字信息”比关键字多一个，所以除了第一个“关键字信息”之外，我们可以将其余所有“关键字信息”均看作与其左侧紧邻的关键字配对（如下图箭头所示），任何添加、移动、删除操作均作为一个整体进行处理，除第一个“关键字信息”之外所有“关键字信息”中第8到11字节均记录其左侧紧邻关键字在“链表”中的下一个关键字在块中的偏移量，而第一个“关键字信息”中第8到11字节记录“链表”中最小关键字在块中的偏移量。如下图所示：

![][image-3]

其次是叶节点，这里主要描述与内节点内存安排的不同之处。叶节点没有子节点，因此除了第一个“关键字信息”之外，其余“关键字信息”中第0到3字节存储其左侧紧邻关键字对应的记录在表文件中的偏移量，同时上文已经提到，由于采用懒惰删除，“关键字信息”的第4到7字节需要记录其左侧紧邻关键字是否被删除，0表示未被删除，1表示被删除，而第一个“关键字信息”因为没有关键字与其匹配，所以第一个“关键字信息”中第0到3字节不需要存储记录在表文件中的偏移量，而是记录该叶节点右侧相邻兄弟叶节点的块偏移量。如下图所示：

![][image-4]

#### 实现方法

BpTree类对B+树的具体实现和数据库系统概念书中所述的不同，而更像是算法导论中的B树改进而来的。首先我定义了一个最小度的概念，每一棵B+树有一个最小度，记为**t**，一个内节点中最少有t - 1个关键字和t个子节点，最多有2t - 1个关键字和2t个子节点，一个叶节点最少有t - 1个关键字和t - 1个相应记录在表文件中的偏移量，最多有2t - 1个关键字和2t - 1个相应记录在表文件中的偏移量。与数据库系统概念书中不同的主要是对于插入过程中分裂节点的处理，这里遇到需要分裂节点的情况时我并不是先将关键字插入至叶节点然后自底向上分裂节点，而是直接在自顶向下插入的过程中判断是否需要分裂。假设当前处于第m层中的A节点，而第m层的下一层记为第n层，在第m层的A节点中搜索向下插入的位置，找到插入的位置（假设为第n层中的B节点）后，首先判断第n层的B节点的关键字个数，如果关键字个数为2t - 1.即B节点已满，则先分裂B节点，然后判断应该讲关键字下降至分裂出的哪个节点，最后再下降至第n层。在插入过程的最开始，首先需要判断根节点是否已满，如果根节点中的关键字个数为2t - 1则先分裂根节点，因此构造出一个新的根节点，B+树的高度由此增加。

<hr/>

### 2.7 Buffer模块

#### 主要功能

1. 数据块的写入、写出
2. 实现缓冲区的替换算法（LRU）
3. 记录块状态（是否被修改过）
4. 锁定缓冲区某块，不允许替换出去

#### 接口设计

**外部接口**

```c++
/*根据文件名和block偏移量获得对应的Buffer block*/
int giveMeABlock(const std::string& file_name, int block_offset);
/*根据buffer block num和byte offset 以及length得到对应字符串*/
std::string giveMeAString(int block_num, int byte_offset, int length);
/*将buffer block中的内容写回文件*/
void WriteBack(int block_num);
/*根据文件名创建新文件*/
void createFile(const string& file_name);
/*将对应Buffer block的IsUpdated属性设置为flag*/
void setUpdated(int block_num, bool flag);
/*将对应Buffer block的IsLocked属性设置为flag*/
void setLocked(int block_num, bool flag);
/*将整个文件相关buffer block置为invalid，用于删除文件*/
void setInvalid(const string& file_name);
/*根据文件名和文件的block数返回对应的buffer blocs，返回数组存储在blocks变量中*/
void readFileIn(int* blocks, const string& file_name, int block_num);
/*根据buffer block数组和buffer block number将整个文件相关buffer block写回文件*/
void WriteFileBack(int* blocks, int block_num);
```

**内部接口**

```c++
/*将对应的文件的Block从硬盘中读入buffer，并返回相应的Buffer block的编号*/
int readMemToBuf(const std::string& file_name, int block_offset);
/*判断对应的文件的block是否已经在buffer中*/
int IsInBuf(const std::string& file_name, int block_offset);
/*得到一块新的buffer block，如果没有空Block，利用LRU算法替换获得新的block*/
int giveMeAnEmptyBlock();
```

#### 设计思路

将BufferManage模块中的Buffer block设计成一个类，命名为buffer。buffer类的成员变量如下所示

```c++
public:
    char BufBlock[BLOCKSIZE];
private:
    std::string fileName;
    int blockOffset;
    bool IsValid;
    bool IsUpdated;
    bool IsLocked;
    time_t RecentTime;
    friend class BufferManage;
```

每一个BufBlock为4KB，同时还记录了该块对应的文件名和文件中的blockoffset；该块是否有效（IsValid），是否被锁住（IsLocked），最近使用时间（RecentTime），这三个变量主要是用在得到一块新的块，用于读取文件；该块是否被修改过（IsUpdated），这个变量主要用在将block写回文件的操作过程中。

**得到文件块对应的buffer block操作：**

首先判断文件块是否已经存在于buffer block中。不存在则需要从硬盘中读入。此时便首先需要得到一块新的Buffer block。遍历整个缓冲区同时记录最远使用的且没有被上锁的buffer block（LRU算法）。在遍历的过程中如果遇到invalid的块，说明是空的，则返回该块。否则，返回遍历过程中记录下的替换块（被锁住的块不会被替换出去）。如果既没有空的块，也没有能够替换出去的块，就说明buffer已经满了，抛出异常。

**将buffer block写回文件操作：**

只将被修改过的Block写回文件，同时clear所有被要求写回的block。

## 3. 测试方案和测试样例

### 3.1 模块测试

#### Interpreter 模块

**input()格式化输入函数测试**

这个函数的测试主要是检查能否将输入按照预先设定的标准进行格式化，以及能否检测非法字符，测试用例如下：

```sql
select * from table     ;
select* from table where name='yunanlong' and id>= 111111;
select *from table where name ='yunanlong' and id <>222222;
create table student (
id int,name char(n) ,
score float
)
;
create index idx on table ( attr) ;
insert into table ('attr', 111  ,    2.5 ) ;
drop table stu#dent;
```

**EXEC\_CREATE\_INDEX函数测试**

该函数的测试主要是检查能否从格式化后的输入中提取索引名、表名和属性名，并对错误的语法进行处理，测试用例如下：

```sql
create index idxname on tablename (attrname);
create index on tablename (attrname);
create index idxname on (attrname);
create index idxname on tablename attrname;
create index idxname on tablename ();
create index idxname on tablename (attrname;
create index ;
create index idxname onno tablename (attrname);
```

**EXEC\_DROP\_INDEX函数测试**

该函数的测试主要是检查能否从格式化后的输入中提取索引名，并对错误的语法进行处理，测试用例如下：

```sql
drop index idxname;
drop index ;
drop index ( idxname);
drop index idxname idxname;
```

**EXEC\_FILE函数测试**

该函数的测试主要是检查能否从格式化后的输入中提取文件名，并对错误的语法进行处理，测试用例如下：

```sql
execfile filename;
execfile ;
execfile filename filename;
execfile filename.sql;
execfile filename .sql;
```

**EXEC\_SELECT函数测试**

检查能否正确提取投影属性和选择条件，并成功处理错误的输入

```sql
select * from tablename;
select id from tablename;
select id, age from tablename;
select * from tablename where id = ‘31501’ and age > 18  ;
select * from tablename where id=‘31501’ and grade<98.5;
select id, grade from tablename where id > ‘31501’;
select * from tablename where age >14 and age < 16 ;
select *;
select * from;
select * from tablename where;
```

**EXEC\_INSERT函数测试**

检查能否正确提取表名和记录每个属性值，并成功处理错误的输入

```sql
insert into tablename values ( ‘alice’, 15, 97.6);
insert into tablename values(‘bob’,15,100);
insert into tablename values( ‘marvel’, 17,   78.5);
insert into ;
insert into tablename values;
insert into tablename values();
insert into tablename values ( ‘’ ‘’);
insert into tablename values (;
insert into tablename values);
insert into tablename values ([not enough attributes]);
```

**EXEC\_DELETE函数测试**

检查能否正确提取选择条件，并成功处理错误的输入

```sql
delete * from tablename;
delete from tablename;
delete * from tablename where id = ‘31501’ and age > 18  ;
delete * from tablename where id=‘31501’ and grade<98.5;
delete from tablename where id > ‘31501’;
delete * from tablename where age >14 and age < 16 ;
delete;
delete *;
delete * from;
delete from;
delete from where;
delete from tablename where id;
delete from tablename where id >;
```

**EXEC\_CREATE\_TABLE函数和EXEC\_DROP\_TABLE函数测试**

```c++
#include<string>
#include<iostream>
#include<sstream>
#include<stdexcept>
#include<regex>
#include"string.h"
#include "base.h"
using namespace std;

bool InvertINT(string s, int& x)
{
    x = 0;
    for (int i = 0; i < s.size(); i++)
    {
        if (s[i] > '9' || s[i] < '0')
            return false;
        x = x * 10 + s[i] - '0';
    }
    return true;
}


int main()
{
    string query = "";
    string ts;
    cin >> ts;  
    do{
        query += ts;
        query += " ";
        cin >> ts;
    }while(ts != ";");

    query += ";";
 
    if(query[12] != ' ')
        throw runtime_error( "Interpreter: syntex error near \"create table\"; \n") ;
 
    Attribute attri;
 
    string s = query.substr(13, query.size() - 13);
    istringstream is(s);
    string tablename;
    is >> tablename;
 
    string t;
    is >> t;
    if(t != "(")
        throw runtime_error("Interpreter: syntex error near \"create table\"; \n");

    int attrinum = 0;
    while(is >> t) /* get the information of the attributes*/
    {
        if(t == "primary")  //primary key
        {
            is >> t;
            if(attrinum == 0 || t != "key")
                throw runtime_error("Interpreter: syntex error near primary; \n");
            is >> t;
            if(t != "(")
                throw runtime_error("Interpreter: syntex error near primary key; \n");
         
            is >> t;  //primary key attribute
            int i;
            for(i = 0; i < attrinum; i++)
                if(t == attri.attrName[i])
                {
                    attri.primary = i;
                    attri.uniqueFlag[i] = true;
                    break;
                }
            if(i >= attrinum)
                throw runtime_error("Interpreter: no attribute named " + t + "\n");
                
            is >> t;
            if(t != ")")
                throw runtime_error("Interpreter: syntex error near primary key.\n");

            is >> t;
            if (t != ")")
                throw runtime_error("Interpreter: syntex error near char.\n");
            else
                break;
        }
     
        smatch result;
        regex exp("[^a-zA-Z_]");
        if(regex_search(t, result, exp) != 0)
            throw runtime_error("Interpreter: syntex error near (; \n");
        attri.attrName[attrinum] = t;  //attribute name
     
        is >> t; //attribute type
        if(t == "int") attri.type[attrinum] = -1;
        else if(t == "float") attri.type[attrinum] = 0;
        else if (t == "char")
        {
            is >> t;
            if (t != "(")
                throw runtime_error("Interpreter: syntex error near char. \n");
            is >> t;
            int num;
            if (InvertINT(t, num) == false)
                throw runtime_error("Interpreter: syntex error near char;\n");
            attri.type[attrinum] = num;

            is >> t;
            if (t != ")")
                throw runtime_error("Interpreter: syntex error near char.\n");
        }
        else
            throw runtime_error("Interpreter: syntex error near " + t);
     
        is >> t;
        if(t == "unique")
        {
            attri.uniqueFlag[attrinum] = true;
            is >> t;
        }
        else
            attri.uniqueFlag[attrinum] = false;
     
        if(t == ",")
            attrinum++;
        else if(t == ")")
        {
            attrinum++;
            break;
        }
        else
            throw runtime_error("Interpreter: syntex error near " + t);
    }
 
    is >> t;
    if(t != ";")
        throw runtime_error("Interpreter: syntex error near " + t + "\n");
 
    attri.attrNum = attrinum;
    Table tableinfo;
    tableinfo.tableName = tablename;
    tableinfo.attr = attri;
 
    cout << "Interpreter: successful!" << endl;
}
```

#### API 模块

API模块主要起到连接Interpreter 模块和下层模块的作用，仅仅只是接收Interpreter 模块传递的参数并调用下层模块的接口，所以我们将API模块的测试并入综合测试中。

#### Catalog Manager 模块

Catalog模块主要负责元数据的信息管理，以及建立表和索引时的查重操作，删除表和索引时的相关操作。因此测试代码将针对Catalog模块管理的两个文件：Catalog.bin（负责记录表和属性的元数据）, CatIndex.bin（负责记录索引的元数据）展开，具体测试代码如下：

```c++
#include <iostream>
#include <string>
#include "Catalog.h"
#include "buffer.h"
#include "base.h"
#include<stdexcept>
using namespace std;

BufferManage buffer;
Catalog catlog;

int main()  
{   
    try{
        /*添加表操作*/ 
        Table t1;
        Attribute a1;
        a1.attrNum = 3;
        a1.attrName[0] = "ID";
        a1.attrName[1] = "Name";
        a1.attrName[2] = "age";
        a1.type[0] = -1, a1.uniqueFlag[0] = true;
        a1.type[1] = 20, a1.uniqueFlag[1] = true;
        a1.type[2] = -1, a1.uniqueFlag[2] = false;
        a1.primary = 0;
        t1.attr = a1;
        t1.tableName = "student";
     
        catlog.addTable(t1);
        cout << "end1" << endl;
     
        Table t2;
        Attribute a2;
        a2.attrNum = 2;
        a2.attrName[0] = "tID";
        a2.attrName[1] = "Name";
        a2.type[0] = -1, a2.uniqueFlag[0] = true;
        a2.type[1] = 20, a2.uniqueFlag[1] = false;
        a2.primary = 0;
        t2.attr = a2;
        t2.tableName = "teacher";
     
        catlog.addTable(t2);
        cout << "end2" << endl;
     
        /*添加和删除索引操作*/ 
        catlog.addIndex("student", "student_ID", "Name");
        cout << "end10" << endl;
        catlog.addIndex("student", "nameIndex", "ID");
        cout << "end11" << endl;
        catlog.addIndex("student", "nameIndex", "Name");
        cout << "end12" << endl;
        catlog.addIndex("student", "ageIndex", "age");
        cout << "end13" << endl;
        catlog.deleteIndex("nameIndex");
        cout << "end14" << endl;
        catlog.deleteIndex("nameIndex");
        cout << "end15" << endl;
        catlog.deleteIndex("name");
        cout << "end16" << endl;
        catlog.addIndex("student", "nameIndex", "Name");
        cout << "end3" << endl;
     
        /*读取元数据信息操作 */ 
        Table t3 = catlog.getTable("student");
        cout << t3.tableName << endl;
        int n = t3.attr.attrNum;
        for(int i = 0; i < n; i++)
        {
            cout << t3.attr.attrName[i] << " " << t3.attr.type[i] << " " 
            << t3.attr.uniqueFlag[i] << endl;
        }
        cout << t3.attr.primary << endl;
        cout << "end4" << endl;
     
        /*添加表查重操作和删除表操作*/ 
        catlog.addTable(t1);
        catlog.deleteTable("student");
        cout << "end6" << endl;
       
        t1.attr.primary = -1;
        catlog.addTable(t1);
        cout << "end5" << endl;
    }
    catch(exception& ex)
    {
        cout << ex.what() << endl;
    }
    return 0;
}
```

#### Record Manager

**注：该模块需要通过Index Manager进行新插入记录中unique key的插入与删除，以及索引数据文件的建立与删除。需要通过Buffer Manager模块对数据文件进行存取操作。因此需结合Index Manager和Buffer Manager模块进行测试。**

模块测试主要检查以下几点：

1. 正确建立/删除数据文件并根据unique 属性调用Index Manager建立/删除相应相应索引文件

2. 正确将获取到的记录转换成字符数组以备插入
3. 正确将读取的字符数组转换成可用于输出的记录形式
4. 插入多条数据并成功调用Index Manager建立索引
5. 正确比较int、float、char型关键字
6. 采用索引查找或循环查找形式争取搜索到待删除/选择记录
7. 插入重复关键字正确处理
8. 正确在懒惰删除所得空闲位置插入新记录

测试代码如下:

```c++
#include <iostream>
#include <string>
#include "indexmanager.h"
#include "buffer.h"
#include "base.h"
#include "recordmanager.h"

using namespace std;

BufferManage buffer;

int main() {

    //The test of RecordManager;
    //1.The test of create table;
    Table TableInfo;

    TableInfo.tableName = "Student";
    TableInfo.attr.attrNum = 3;
    TableInfo.attr.attrName[0] = "ID";
    TableInfo.attr.attrName[1] = "Age";
    TableInfo.attr.attrName[2] = "Grade";
    TableInfo.attr.type[0] = 10;
    TableInfo.attr.type[1] = -1;
    TableInfo.attr.type[2] = 0;
    TableInfo.attr.uniqueFlag[0] = 1;
    TableInfo.attr.uniqueFlag[1] = 0;
    TableInfo.attr.uniqueFlag[2] = 0;
    TableInfo.attr.primary = 0;
    {
        RecordManager RM(&buffer);
        RM.tableCreate(TableInfo);
    }
    string file_name = TableInfo.tableName + ".table";


    FILE *fp = fopen(file_name.c_str(), "rb");
    int Number[3];
    fread(Number, sizeof(int), 3, fp);
    cout << Number[0] << endl;
    cout << Number[1] << endl;
    cout << Number[2] << endl;
    fclose(fp);

    //The test of drop table;
    RM.tableDrop(TableInfo);

    //The test of insertion;
    Tuple Line;
    Line.element.push_back(new Datac("3150100"));
    Line.element.push_back(new Datai(19));
    Line.element.push_back(new Dataf(95.55));
    Tuple Line2;
    Line2.element.push_back(new Datac("3150101"));
    Line2.element.push_back(new Datai(20));
    Line2.element.push_back(new Dataf(97.11));


    char *charInsert = new char[20];
    //charInsert = RM.Tuple2Line(TableInfo, Line, 20);
    //cout << "Wrong" <<endl;

    {
        RecordManager RM(&buffer);
        RM.InsertValue(TableInfo, Line);
        RM.InsertValue(TableInfo, Line2);
    }

    char Valid[20];
    char Valid2[20];

    fp = fopen(file_name.c_str(), "rb");
    fseek(fp, (long) (1 * BLOCKSIZE), SEEK_SET);
    fread(&Valid, sizeof(char), 20, fp);
    fread(&Valid2, sizeof(char), 20, fp);
    fclose(fp);


    //The test of Deletion:
    vector<condition> comp;
    condition C1;
    //C1.message = new Datac("3150100");
    //C1.message = new Datai(19);
    C1.message = new Dataf(95.55);
    C1.flag = E;
    C1.number = 2;
    comp.push_back(C1);
    /*{
        RecordManager RM(&buffer);
        RM.Delete(TableInfo, comp);
    }*/

    fp = fopen(file_name.c_str(), "rb");
    fseek(fp, (long)(1 * BLOCKSIZE), SEEK_SET);
    fread(&Valid,sizeof(char),20,fp);
    fclose(fp);


    //The test of selection
   /* vector<condition> comp;
    condition C1;
    C1.message = new Datac("3150100");
    C1.flag = NE;
    C1.number = 0;
    comp.push_back(C1);*/


    vector<int> attrIndex;
    attrIndex.push_back(2);
    attrIndex.push_back(1);
    attrIndex.push_back(0);
    {
        RecordManager RM(&buffer);
        Table outPut = RM.SelectWithWhere(TableInfo, attrIndex, comp);
        outPut.outputTable();
    }

    return 0;
  
}
```

#### Index Manager 模块

**注：该模块需要通过Buffer Manager模块对DB file进行存取操作，所以需要结合Buffer Manager模块进行测试。**

该模块的测试主要检查：

1. 能否将int、float、char(n)类型的关键字以及相应的记录位置插入模块中的B+树
2. 插入关键字后能否对int、float、char(n)类型进行等值查找
3. 插入关键字后能否对int、float、char(n)类型进行区间查找
4. 能否删除B+树中已经存在的int、float、char(n)类型的关键字
5. 删除B+树中不存在的int、float、char(n)类型的关键字是否能够正确处理
6. 能否插入被删除的int、float、char(n)类型的关键字
7. 插入重复的关键字是否能够正确处理

为此我编写了一个main函数用于测试索引模块，测试用例来源于随机输入：

```c++
#include <iostream>
#include <string>
#include "indexmanager.h"
#include "buffer.h"
#include "base.h"

using namespace std;

BufferManage buffer;

int main() {
    IndexManager im;
    im.createIndexFile("bptreetest1", 10);
    string j, k;
    int i = 0;
    while(1){
        cout << "input" << endl;
        cin >> j;
        if(!j.compare("-1")){
            break;
        }
        Datac k(j);
        cout << i << endl;
        cout << im.insertKey("bptreetest1", &k, i++) << endl;
    }
    
    while(1){
        cout << "delete" << endl;
        cin >> j;
        if(!j.compare("-1")){
            break;
        }
        Datac k(j);
        cout << im.deleteKey("bptreetest1", &k) << endl;
    }
    
    while(1){
        cout << "input" << endl;
        cin >> j;
        if(!j.compare("-1")){
            break;
        }
        Datac k(j);
        cout << i << endl;
        cout << im.insertKey("bptreetest1", &k, i++) << endl;
    }
    
    while(1){
        cout << "find" << endl;
        cin >> j;
        if(!j.compare("-1")){
            break;
        }
        Datac k(j);
        cout << im.findKey("bptreetest1", &k) << endl;
    }
    
    while(1){
        cout << "findRange" << endl;
        cin >> j;
        cin >> k;
        if(!j.compare("-1")){
            break;
        }
        Datac left(j), right(k);
        vector<int> result = im.findKeyRange("bptreetest1", &left, &right);
        for(int m = 0; m < result.size(); m++){
            cout << result[m] << endl;
        }
    }
    
    return 0;
}
```

#### Buffer Manager 模块

Buffer主要负责文件管理模块与硬盘文件的交互接口，负责文件的读入和写出。同时还要实现buffer block的状态管理和替换算法。因此测试代码将针对以上三点展开设计，详细如下：

```c++
#include "buffer.h"
#include "string.h"
#include<stdio.h> 
#include<iostream>
#include<string>
using namespace std;

int main()
{   
    /*创建测试文件*/
    BufferManage bf;
    bf.createFile("test.bin");

    /*获得buffer block*/
    int block_name = bf.giveMeABlock("test.bin", 0);
    cout << block_name << endl;
    int block_num = bf.giveMeABlock("test.bin", 1);
    cout << block_num << endl;

    /*将数据写入buffer block 后写回文件*/
    char str[20] = "hello world!";  
    memcpy(&(bf.Buf[block_name]), str, 20);
    //检测buffer block的写入
    printf("%s\n", bf.Buf[block_name].BufBlock ); 
 
    bool flag = true;
    bf.setUpdated(block_name, flag);
    bf.WriteBack(block_name);
 
    memcpy(&(bf.Buf[block_num]), str, 20);
    bf.setUpdated(block_num, flag);
    bf.WriteBack(block_num);

    /*读入文件*/
    int block = bf.giveMeABlock("test.bin", 1);
    cout << block << endl;
    cout << bf.Buf[block].BufBlock << endl;
    bf.setLocked(block, true);
 
    int block1 = bf.giveMeABlock("test.bin", 0);
    cout << block1 << endl;
    cout << bf.Buf[block1].BufBlock << endl;

    /*替换算法*/
    int block2 = bf.giveMeABlock("test.bin", 2);
    cout << block2 << endl;
    int block3 = bf.giveMeABlock("test.bin", 3);
    cout << block3 << endl;
 
}
```

### 3.2 综合测试

在小组各成员完成对各自编写的模块和部分函数的测试之后，我们对MiniSQL进行综合测试，综合测试的方案和部分测试用例如下：

**注：由于在interpreter模块的测试中已经测试了MiniSQL检查语法错误的功能，所以综合测试中不再对此进行测试。**

1. 小组成员共同检查各模块与其他模块接口调用的正确性，确保正确使用了其他模块的接口。
2. 首先我们需要建立一个表，同时**测试create table的功能**，测试用例如下：
 ```sql
 //正确建表
 create table student (
 id int,
 name char(10),
 score float,
 primary key (id)
 );
     
 //正确建表，并进行unique约束
 create table book (
 id int,
 name char(20) unique,
 price float,
 primary key (id)
 );
     
 //测试对于重复建表是否能够返回错误
 create table student (
 id int,
 name char(10)
 );
     
 //测试对于表中属性重复是否能够返回错误
 create table teacher(
 name char(10),
 name char(20),
 salary int
 );
 ```
3. 接下来我们需要编写程序生成一个student.sql文件，文件中全部是insert语句，向表中插入记录的同时可以**测试execfile的功能**，执行：

 ```sql
 execfile student.sql;
 ```
4. 我们需要**测试insert功能**，对于insert功能的测试可以结合**对select部分功能的测试**，执行如下语句，检查输出的记录是否和插入的记录相同：
 ```sql
 select * from student;
 ```
5. 然后我们**测试select的功能**，测试用例如下：

 ```sql
 //测试对于int类型的等值查询
 select * from student where id = 125346;
     
 //测试对于char(n)类型的等值查询
 select * from student where name = "nick";
     
 //测试对于float类型的等值查询
 select * from student where score = 95.5;
     
 //测试对于int类型的区间查询
 select * from student where id <> 111111;
     
 //测试对于char(n)类型的区间查询
 select * from student where name <= "Lily";
     
 //测试对于float类型的区间查询
 select * from student where score > 92;
     
 //测试同一属性上的多条件查询
 select * from student where score >= 90 and score < 100;
     
 //测试不同属性上的多条件查询
 select * from student where id > 222222 and score <= 89;
     
 //测试在不存在的表中进行查询是否会报错
 select * from teacher;
     
 //测试where条件涉及不存在的属性是否会报错
 select * from student where age = 20;
 ```
6. 我们再**对primary key和unique约束进行测试**，首先测试primary key，向student表中插入一条id冲突的记录，看是否能够返回错误。然后编写程序生成book.sql文件，文件中全是insert语句，因为之前已经测试了insert功能，所以这里我们可以保证insert能够正确执行。脚本文件执行结束后，我们向book表中插入一条name冲突的语句，看是否能够返回错误。
7. 接着是**测试delete功能**，测试用例如下：

 ```sql
 //测试能否删除
 delete from student where id = 222222;
 select * from student where id = 222222;
     
 //测试删除不存在的记录
 delete from student where id = 222222;
     
 //测试插入已经删除的记录
 insert into student (222222, Bryant, 81);
     
 //测试删除整个表中的记录
 delete from student;
 select * from student;
 ```
8. **测试create index功能**，测试用例如下：

 ```sql
 //测试能否正确建立索引
 create index nameidx on book(name);
     
 //测试能否检查重复建立索引
 create index nameidxidx on book(name);
     
 //测试对非unique属性建立索引能否返回错误
 create index priceidx on book(price);
     
 //测试在不存在的表或属性上建立索引能否返回错误
 create index nameidxidxidx on teacher(idx);
 ```
9. **测试drop index功能**，因为之前已经测试了create index检查重复建立索引的功能，所以这里可以利用这一点来测试，测试用例如下：

 ```sql
 //测试能否正确删除索引
 drop index nameidx;
 create index nameidx on book(name);
     
 //测试删除不存在的索引是否会返回错误
 drop index nameidxidxidx;
 ```
10. **测试drop table功能**，测试用例如下：

```sql
//测试能否正确删除表
drop table student;
select * from student;

//测试删除不存在的表是否会返回错误
drop table student;
```

11. **测试quit功能**，执行quit语句，看程序能否正常退出。

## 4. 分组与设计分工

**本组成员**

+ 余南龙
+ 康自蓉
+ 胡诗佳

**本系统的分工如下：**

+ Interpreter 模块：康自蓉 胡诗佳 余南龙
+ API 模块：康自蓉 胡诗佳 余南龙
+ Catalog Manager 模块：康自蓉
+ Record Manager 模块：胡诗佳
+ Index Manager 模块：余南龙
+ Buffer Manager 模块：康自蓉
+ 单元测试：由每个小组成员对各自负责的模块完成单元测试
+ 综合测试：小组成员协作进行综合测试

[image-1]:https://ws3.sinaimg.cn/large/006tNc79gy1fg7bjzc9avj31180vaaeo.jpg
[image-2]:https://ws1.sinaimg.cn/large/006tKfTcgy1fg5jy5iv1mj31720o60us.jpg
[image-3]:https://ws3.sinaimg.cn/large/006tNc79gy1fg64zu94thj31700oadj5.jpg
[image-4]:https://ws2.sinaimg.cn/large/006tNc79gy1fg65b8jthaj31720oan0x.jpg