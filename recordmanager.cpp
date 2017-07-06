#include <iostream>
#include <cmath>
#include "recordmanager.h"

const int TUPLETOTAL_ = 0; 
const int NEXTVACANCY_ = 4; //store the tuple sequence number of next vacancy
const int TUPLELENGTH_ = 8; 
const char EMPTY_ = '#';
const char VALID_ = 'G';
const float PRECISE_ = 0.000001;

using namespace std;

bool RecordManager::tableCreate(const Table& tableInfo)
{
	string FileName = tableInfo.tableName + ".table";
    //debug:
    //cout << FileName << endl;
	IndexManager IM;
	//ofstream fout(FileName.c_str(), ios::out);
	//fout.close();
	pBuffer->createFile(FileName);
	
	int blockNum = pBuffer->giveMeABlock(FileName,0);
	int tupleTotal = 0;//0
	int nextVacancy = -1;//4
	int tupleLength = 0;//8
	for(int i = 0; i < tableInfo.attr.attrNum; i++ )
	{
		//Count the tuple Length
		if(tableInfo.attr.type[i]==-1)
			tupleLength += sizeof(int);
		else if(tableInfo.attr.type[i]==0)
			tupleLength += sizeof(float);
		else
			tupleLength += tableInfo.attr.type[i] + 1; 
		//Create the Index file
		if(tableInfo.attr.uniqueFlag[i] == 1)
		{
			string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] + ".index";
			IM.createIndexFile(indexFileName, tableInfo.attr.type[i]);
		}
	}
	tupleLength += 1;//set whether the tuple is valid
	memcpy(pBuffer->Buf[blockNum].BufBlock + TUPLETOTAL_, &tupleTotal, sizeof(int) );
	memcpy(pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, &nextVacancy, sizeof(int) );
	memcpy(pBuffer->Buf[blockNum].BufBlock + TUPLELENGTH_, &tupleLength, sizeof(int) );
	pBuffer->setUpdated(blockNum, true);
	pBuffer->WriteBack(blockNum);

	return true;
}

bool RecordManager::tableDrop(const Table& tableInfo)
{
    //debug:
    //extern int errno;
    //errno = 0;
	string FileName = tableInfo.tableName + ".table";
	if(remove( FileName.c_str()))
        //cout << errno << endl;
		return false;
	else
		pBuffer->setInvalid(FileName);
    //cout << "File deleting..." << endl;
	//
	for(int i = 0; i < tableInfo.attr.attrNum; i++ )
	{ 
		//Remove the Index file
		if(tableInfo.attr.uniqueFlag[i] == 1)
		{
			string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] + ".index";
			if(remove(indexFileName.c_str()))
				return false;
		}
	}
	//
	return true;

}

Table RecordManager::SelectWithWhere(Table& tableInfo, vector<int> attrIndex, vector<condition> comp)
{
	string FileName = tableInfo.tableName + ".table";

	int blockNum = pBuffer->giveMeABlock(FileName,0);
	pBuffer->setLocked(blockNum, true);
	int tupleLength, nextVacancy, tupleTotal;
	int posInBlock;
	memcpy(&tupleLength, pBuffer->Buf[blockNum].BufBlock + TUPLELENGTH_, sizeof(int) );
	memcpy(&nextVacancy, pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, sizeof(int) );
	memcpy(&tupleTotal, pBuffer->Buf[blockNum].BufBlock + TUPLETOTAL_, sizeof(int) );
	if(comp.size() == 1 && tableInfo.attr.uniqueFlag[comp[0].number] == 1 && comp[0].message != NULL && comp[0].flag == E)
	{
		pBuffer->setLocked(blockNum, false);
        //
		int posKey;
		Tuple* temp = FindWithIndex(tableInfo, comp[0].number, comp[0].message, tupleLength, &posKey);
		if(temp)
			tableInfo.tuple.push_back(temp);
		return PrintAnswer(tableInfo, attrIndex);
	}
	int blockSeq = 0;
    int tupleEachBlock = (BLOCKSIZE-(BLOCKSIZE % tupleLength)) / tupleLength;

	for(int tupleNum = 0; tupleNum < tupleTotal; tupleNum++)
	{	
		if(tupleNum >= tupleEachBlock * blockSeq)
		{
			pBuffer->setLocked(blockNum, false);
            blockSeq++;
			blockNum = pBuffer->giveMeABlock(FileName, blockSeq);
			pBuffer->setLocked(blockNum, true);
		}
        posInBlock = tupleNum % tupleEachBlock * tupleLength;
        char* line = new char[tupleLength];
        memcpy(line, pBuffer->Buf[blockNum].BufBlock + posInBlock, tupleLength);
		if(line[0] == EMPTY_)
			continue;
		Tuple* temp = Line2Tuple(line, tableInfo);
		if(temp != NULL)
		{
			if(!comp.size())
			{
				tableInfo.tuple.push_back(temp);
			}
			else if (MeetCondition(tableInfo, temp, comp))
			{
				tableInfo.tuple.push_back(temp);
			}
		}
        delete []line;
	}
	pBuffer->setLocked(blockNum, false);
	return PrintAnswer(tableInfo,attrIndex);
}

int RecordManager::Delete(const Table& tableInfo, vector<condition> comp)
{

	string FileName = tableInfo.tableName + ".table";
	IndexManager IM;
	int count = 0;
	int blockNum = pBuffer->giveMeABlock(FileName,0);

	pBuffer->setLocked(blockNum, true);
	int tupleLength, nextVacancy, tupleTotal;
	//int offSet = 12;
	int posInBlock;
	memcpy(&tupleLength, pBuffer->Buf[blockNum].BufBlock + TUPLELENGTH_, sizeof(int) );
	memcpy(&nextVacancy, pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, sizeof(int) );
	memcpy(&tupleTotal, pBuffer->Buf[blockNum].BufBlock + TUPLETOTAL_, sizeof(int) );
    int tupleEachBlock = (BLOCKSIZE-(BLOCKSIZE % tupleLength)) / tupleLength;
	if(comp.size() == 1 && tableInfo.attr.uniqueFlag[comp[0].number] == 1 && comp[0].message != NULL && comp[0].flag == E)
	{
		int posKey = -1;
		Tuple* temp = FindWithIndex(tableInfo, comp[0].number, comp[0].message, tupleLength, &posKey);
        if(temp == NULL)
            throw runtime_error("deletion failed: temp is null!no such tuple!\n");
		if(posKey > 0)
		{
            //cout << "posKey=" << posKey << endl;
			pBuffer->setLocked(blockNum, false);

			int blockNum2 = pBuffer->giveMeABlock(FileName, posKey / BLOCKSIZE );
			pBuffer->setLocked(blockNum2, true);
            posInBlock = posKey % BLOCKSIZE;
            pBuffer->Buf[blockNum2].BufBlock[posInBlock] = EMPTY_;
			memcpy(pBuffer->Buf[blockNum2].BufBlock + posInBlock + 1, &nextVacancy, sizeof(int));
			nextVacancy = (posKey / BLOCKSIZE - 1) * tupleEachBlock + posInBlock/ tupleLength;
			for(int i = 0; i < tableInfo.attr.attrNum; i++)
			{
                //cout << "count "<< i << endl;
				if(tableInfo.attr.uniqueFlag[i] == 1)
				{
                    //cout << "deleteKey" << endl;
					string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] + "." + "index";
					if(IM.deleteKey(indexFileName, temp->element[i]) == false)
                    {
                        //cout <<"delete: 0 "<< endl;
                        throw runtime_error("RecordManager: delete key failed.\n");
                    }
                    //else cout <<"delete: 1 " << endl;
				}
			}
			pBuffer->setUpdated(blockNum2, true);
			pBuffer->setLocked(blockNum2, false);
            pBuffer->WriteBack(blockNum2);
			blockNum = pBuffer->giveMeABlock(FileName, 0);
			memcpy(pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, &nextVacancy, sizeof(int) );
			pBuffer->setUpdated(blockNum, true);
            pBuffer->WriteBack(blockNum);
			count++;
		}
        else
            throw runtime_error("deletion failed: no such tuple!!\n");
		return count;

	}
	int blockSeq = 0;
	int blockOffSet = 0;
	
	for(int tupleNum = 0; tupleNum <tupleTotal; tupleNum++)
	{
        if(tupleNum >= tupleEachBlock * blockSeq)
        {
            pBuffer->setLocked(blockNum, false);
            blockSeq++;
            blockNum = pBuffer->giveMeABlock(FileName, blockSeq);
            pBuffer->setLocked(blockNum, true);
        }
        posInBlock = tupleNum % tupleEachBlock * tupleLength;
        char* line = new char[tupleLength];
        memcpy(line, pBuffer->Buf[blockNum].BufBlock + posInBlock, tupleLength);
		if(line[0] == EMPTY_)
			continue;
		Tuple* temp = Line2Tuple(line, tableInfo);
		if(temp != NULL)
		{
			if(!comp.size() || MeetCondition(tableInfo, temp, comp))
			{
				pBuffer->Buf[blockNum].BufBlock[posInBlock]= EMPTY_;
				memcpy(pBuffer->Buf[blockNum].BufBlock +posInBlock + 1, &nextVacancy, sizeof(int));
				nextVacancy = tupleNum;
				pBuffer->setUpdated(blockNum, true);

				for(int i = 0; i < tableInfo.attr.attrNum; i++)
				{
					if(tableInfo.attr.uniqueFlag[i] == 1)
					{
						string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] +  ".index";
						if(IM.deleteKey(indexFileName, temp->element[i]) == false)
							throw runtime_error("RecordManager: delete key failed.\n"); 
					}
				}
				count++;
			}
		}
        delete []line;
	}
	pBuffer->setLocked(blockNum, false);
	blockNum = pBuffer->giveMeABlock(FileName, 0);
	memcpy(pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, &nextVacancy, sizeof(int) );
	pBuffer->setUpdated(blockNum, true);

	return count;
	
}

bool RecordManager::MeetCondition(const Table& tableInfo, Tuple* line, vector<condition> comp)
{
	for(int i = 0; i < comp.size(); i++)
	{
		if(comp[i].message==NULL)
			continue;
		else if(line->element[comp[i].number]->type == -1)
		{
			int goal = ((Datai*)(comp[i].message))->val;
			int now = ((Datai*)(line->element[comp[i].number]))->val;
			switch(comp[i].flag)
			{
				case E:	if(!(now == goal))return false;break;
				case SE:if(!(now <= goal))return false;break;
				case BE:if(!(now >= goal))return false;break;
				case S:	if(!(now <  goal))return false;break;
				case B:	if(!(now >  goal))return false;break;
				case NE:if(!(now != goal))return false;break;
				default: break;
			}
		}
		else if(line->element[comp[i].number]->type == 0)
		{
			float goal = ((Dataf*)(comp[i].message))->val;
			float now = ((Dataf*)(line->element[comp[i].number]))->val;
			switch(comp[i].flag)
			{
                
				case E:	if(!(fabs(now - goal) < PRECISE_))return false;break;
				case SE:if(!((now < goal) || (fabs(now - goal) < PRECISE_)))return false;break;
				case BE:if(!((now > goal) || (fabs(now - goal) < PRECISE_)))return false;break;
				case S:	if(!((now < goal) && (fabs(now - goal) > PRECISE_)))return false;break;
				case B:	if(!((now > goal) && (fabs(now - goal) > PRECISE_)))return false;break;
				case NE:if(!(fabs(now - goal) > PRECISE_))return false;break;
				default: break;
			}
		}
		else if(line->element[comp[i].number]->type > 0)
		{
			string goal = ((Datac*)(comp[i].message))->val;
			string now = ((Datac*)(line->element[comp[i].number]))->val;
			switch(comp[i].flag)
			{
				case E:	if(!(now == goal))return false;break;
				case SE:if(!(now <= goal))return false;break;
				case BE:if(!(now >= goal))return false;break;
				case S:	if(!(now <  goal))return false;break;
				case B:	if(!(now >  goal))return false;break;
				case NE:if(!(now != goal))return false;break;
				default: break;
			}
		}
		else
		{
			throw runtime_error("RecordManager: The attribute flag is wrong when judge with condition\n");
		}
	}
	return true;
}

Table RecordManager::PrintAnswer(Table& tableInfo, vector<int>attrIndex)
{
	Table printOut;
	Attribute printAttr;
	printAttr.attrNum = attrIndex.size();
	Tuple* newTuple = NULL;
	for(int i = 0; i < printAttr.attrNum ; i++)
	{
		printAttr.attrName[i] = tableInfo.attr.attrName[attrIndex[i]];
		printAttr.type[i] = tableInfo.attr.type[attrIndex[i]];
		printAttr.uniqueFlag[i] = tableInfo.attr.uniqueFlag[attrIndex[i]];
	}
	printOut.attr = printAttr;
	printOut.tableName = tableInfo.tableName;
	for(int i = 0; i < tableInfo.tuple.size(); i++)
	{
		newTuple = new Tuple;
		for(int j = 0; j < attrIndex.size(); j++)
		{
			int k = attrIndex[j];
			Data* newData = NULL;
			if(tableInfo.attr.type[k] == -1)
			{
                int value = ((Datai*)((tableInfo.tuple[i])->element[k]))->val;
				newData = new Datai (value);
			}
			else if(tableInfo.attr.type[k] == 0)
			{
                float value = ((Dataf*)((tableInfo.tuple[i])->element[k]))->val;
				newData = new Dataf (value);
			}
			else if(tableInfo.attr.type[k] > 0)
			{
                string value = ((Datac*)((tableInfo.tuple[i])->element[k]))->val;
				newData = new Datac (value);
			}
			newTuple->element.push_back(newData);
		}
		printOut.tuple.push_back(newTuple);

	}
	return printOut;
} 

void RecordManager::InsertValue(const Table& tableInfo, Tuple& line)
{
	string FileName = tableInfo.tableName + ".table";
	IndexManager IM;
	int blockNum = pBuffer->giveMeABlock(FileName, 0);
    pBuffer->setLocked(blockNum, true);
	int tupleLength, nextVacancy, tupleTotal;
	int offSet;
	int posInBlock;
	memcpy(&tupleLength, pBuffer->Buf[blockNum].BufBlock + TUPLELENGTH_, sizeof(int) );
	memcpy(&nextVacancy, pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, sizeof(int) );
	memcpy(&tupleTotal, pBuffer->Buf[blockNum].BufBlock + TUPLETOTAL_, sizeof(int) );
    int tupleEachBlock = (BLOCKSIZE-(BLOCKSIZE % tupleLength)) / tupleLength;
    /*
	for(int i = 0; i < tableInfo.attr.attrNum; i++)
	{
		if(tableInfo.attr.uniqueFlag[i] == true)
		{
			int posKey;
			if(FindWithIndex(tableInfo, i, line.element[i],tupleLength,&posKey))
			{
				throw runtime_error("ERROR: The unique value has rebundancy!! The insertion failed.");
			}
		}
	}
     */
	//Add tuple to the vacancy
	char* charInsert;
	charInsert = Tuple2Line(tableInfo, line, tupleLength);
	if(nextVacancy != -1)
	{
		int tupleNum = nextVacancy;
        posInBlock = tupleNum % tupleEachBlock * tupleLength;
        offSet = (tupleNum / tupleEachBlock + 1) * BLOCKSIZE + posInBlock;
        //try to insert the key into index:
        for(int i = 0; i < tableInfo.attr.attrNum; i++)
        {
            if(tableInfo.attr.uniqueFlag[i] == 1)
            {
                string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] + ".index";
                if(IM.insertKey(indexFileName, line.element[i], offSet) == false)
                {
                    for(int j = 0; j < i; j ++)
                    {
                        string indexFileName2 = tableInfo.tableName + "." + tableInfo.attr.attrName[j] + ".index";
                        IM.deleteKey(indexFileName2, line.element[j]);
                    }
                    string Error =  "ERROR: The unique value has rebundancy!! The insertion failed.";
                    throw runtime_error(Error);
                }
            }
        }
		blockNum = pBuffer->giveMeABlock(FileName, tupleNum / tupleEachBlock + 1);
		memcpy(&nextVacancy, pBuffer->Buf[blockNum].BufBlock + posInBlock + 1, sizeof(int));
		memcpy(pBuffer->Buf[blockNum].BufBlock + posInBlock, charInsert, tupleLength);
		pBuffer->setUpdated(blockNum, true);
        pBuffer->setLocked(blockNum, false);
        //pBuffer->WriteBack(blockNum);
		blockNum = pBuffer->giveMeABlock(FileName, 0);
		memcpy(pBuffer->Buf[blockNum].BufBlock + NEXTVACANCY_, &nextVacancy, sizeof(int) );
		pBuffer->setUpdated(blockNum, true);
        //pBuffer->WriteBack(blockNum);

	}
	else{
		tupleTotal += 1;
        int tupleNum = tupleTotal - 1;
        posInBlock = tupleNum % tupleEachBlock * tupleLength;
        offSet = (tupleNum / tupleEachBlock + 1) * BLOCKSIZE + posInBlock;
        //try to insert the key into index:
        for(int i = 0; i < tableInfo.attr.attrNum; i++)
        {
            if(tableInfo.attr.uniqueFlag[i] == 1)
            {
                string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] + ".index";
                if(IM.insertKey(indexFileName, line.element[i], offSet) == false)
                {
                    for(int j = 0; j < i; j ++)
                    {
                        string indexFileName2 = tableInfo.tableName + "." + tableInfo.attr.attrName[j] + ".index";
                        IM.deleteKey(indexFileName2, line.element[j]);
                    }
                    string Error =  "ERROR: The unique value has rebundancy!! The insertion failed.";
                    throw runtime_error(Error);
                }
            }
        }
		memcpy(pBuffer->Buf[blockNum].BufBlock + TUPLETOTAL_, &tupleTotal, sizeof(int));
		pBuffer->setUpdated(blockNum, true);
        pBuffer->setLocked(blockNum, false);
        //pBuffer->WriteBack(blockNum);
		blockNum = pBuffer->giveMeABlock(FileName, tupleNum / tupleEachBlock + 1);
		memcpy(pBuffer->Buf[blockNum].BufBlock + posInBlock, charInsert, tupleLength);
		pBuffer->setUpdated(blockNum, true);
        //pBuffer->WriteBack(blockNum);

	}
    /*
	for(int i = 0; i < tableInfo.attr.attrNum; i++)
	{
		if(tableInfo.attr.uniqueFlag[i] == 1)
		{
			string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[i] + ".index";
			if(IM.insertKey(indexFileName, line.element[i], offSet) == false)
            {
                string Error = "RecordManager: Insert the i-th key failed.\n";
                throw runtime_error(Error);
            }
		}
	}
     */
	return;
}

Tuple* RecordManager::FindWithIndex(const Table& tableInfo, int uniqueNum, Data* uniqueKey, int tupleLength, int* posKey)
{
	IndexManager IM;
	string FileName = tableInfo.tableName + ".table";
	string indexFileName = tableInfo.tableName + "." + tableInfo.attr.attrName[uniqueNum] + ".index";
    //debug
    /*cout << indexFileName << endl;
    cout << uniqueNum << endl;

    if(tableInfo.attr.type[uniqueNum] == -1)
        cout << ((Datai* )uniqueKey)->val << endl;
    else if(tableInfo.attr.type[uniqueNum] == 0)
        cout << ((Dataf*)uniqueKey)->val << endl;
    else if(tableInfo.attr.type[uniqueNum] > 0)
        cout << ((Datac*)uniqueKey)->val << endl;
    else
        cout << "The attribute type is wrong!!!" <<endl;
		*/
	*posKey = IM.findKey(indexFileName, uniqueKey);
	//cout << *posKey << endl; //Debug
	if(*posKey < 0)
		return NULL;
    int blockSeq = *posKey / BLOCKSIZE;
	int blockNum = pBuffer->giveMeABlock(FileName, blockSeq);
	//string line = pBuffer->giveMeAString(blockNum, *posKey % BLOCKSIZE, tupleLength);
    //The use of string has some error??
    char* line = new char[tupleLength];
    int posInBlock = *posKey % BLOCKSIZE;
    memcpy(line, pBuffer->Buf[blockNum].BufBlock + posInBlock, tupleLength);
	//These two lines may be of no use;
	if(line[0] == EMPTY_)
		return NULL;
	//The above two lines
	Tuple* temp =Line2Tuple(line, tableInfo);
    delete []line;
	return temp;
}

Tuple* RecordManager::Line2Tuple(char* line,const Table& tableInfo)
{
	int pos = 1;
	Tuple* temp = new Tuple;
	for(int attrI = 0; attrI < tableInfo.attr.attrNum; attrI++)
	{
		if(tableInfo.attr.type[attrI] == -1)
		{
			int value;
			memcpy(&value, line + pos, sizeof(int));
			pos += sizeof(int);
			temp->element.push_back(new Datai(value));
		}
		else if(tableInfo.attr.type[attrI] == 0)
		{
			float value;
			memcpy(&value, line + pos, sizeof(float));
			pos += sizeof(float);
			temp->element.push_back( new Dataf(value));
		}
		else
		{
			char value[100];
			int stringLen = tableInfo.attr.type[attrI]+1;
			memcpy(value, line + pos, stringLen);
			pos += stringLen;
			temp->element.push_back(new Datac(string(value)));
		}
	}
	if(temp->element.size()==0)
	{
		delete temp;
		return NULL;
	}
	else
		return temp;
}

char* RecordManager::Tuple2Line(const Table& tableInfo,Tuple& line, int tupleLength)
{
	char* charInsert = new char[tupleLength];
	charInsert[0]=VALID_;
	int pos = 1;
	for(int i = 0; i < tableInfo.attr.attrNum; i++)
	{
		if(tableInfo.attr.type[i] == -1)
		{
			int newData = ((Datai*)(line.element[i]))->val;
			memcpy(charInsert + pos, &newData, sizeof(int));
			pos += sizeof(int);
		}
		else if(tableInfo.attr.type[i] == 0)
		{
			float newData = ((Dataf*)(line.element[i]))->val;
			memcpy(charInsert + pos, &newData, sizeof(float));
			pos += sizeof(float);
		}
		else 
		{
			string newData = ((Datac*)(line.element[i]))->val;
			memcpy(charInsert + pos, newData.c_str(), tableInfo.attr.type[i] + 1);
			pos +=tableInfo.attr.type[i] + 1;
		}
	}
	return charInsert;

}
