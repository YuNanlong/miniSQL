#include"interpreter.h"
#include"base.h"
#include"Catalog.h"
#include"API.h"
#include<iostream>
#include<string>
#include<stdexcept>
#include<sstream>
#include<regex>
#include <fstream>
#include <cstdlib>

using namespace std;

API api;

Interpreter::Interpreter(){
}

Interpreter::~Interpreter(){
}

void Interpreter::input()
{
    string temp;
    char c;
    int flag = 0, flag1 = 0; //flag用于标记当前temp的最后一个字符是不是空格
                         //flag1用于标记是否存在未闭合的单引号
    
    query.clear();
    while(1){
        c = cin.get();
        if(flag1 == 1){
            temp += c;
            if(c == '\'' | c == '\"'){
                temp += ' ';
                flag = 1;
                flag1 = 0;
            }
            continue;
        }
        if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '.'|| (c >= '0' && c <= '9') || c == '-'){
            temp += c;
            flag = 0;
        }
        else if(c == '*' || c == ',' || c == '(' || c == ')'){
            if(flag == 1){
                ;
            }
            else{
                temp += ' ';
            }
            temp += c;
            temp += ' ';
            flag = 1;
        }
		else if(c == '>' || c == '<' || c == '='){
			if(flag == 1){
				if(temp[temp.length() - 2] == '>'
				|| temp[temp.length() - 2] == '<'
				|| temp[temp.length() - 2] == '='){
					temp[temp.length() - 1] = c;
					temp += ' ';
				}
				else{
					temp += c;
					temp += ' ';
				}
			}
			else{
				temp += ' ';
				temp += c;
				temp += ' ';
				flag = 1;
			}
		}
        else if(c == '\'' | c == '\"'){
            flag1 = 1;
            if(flag == 0){
                temp += ' ';
            }
            else{
                ;
            }
            temp += c;
        }
        else if(c == '\n' || c == '\t' || c == ' ' || c == '\r'){
            if(flag == 1){
                ;
            }
            else{
                flag = 1;
                temp += ' ';
            }
        }
        else if(c == ';'){
            if(flag == 1){
                ;
            }
            else{
                temp += ' ';
            }
            temp += c;
            break;
        }
        else{
            cout << "illegal character" << endl;
            while(cin.get() != ';'){
                ;
            }
            temp.clear();
        }
    }
    int start = temp.find_first_not_of(' ', 0);
    query = temp.substr(start, temp.length() - start);
    return ;
}

void Interpreter::debugPrint(){
    cout << query << endl;
}

int Interpreter::EXEC()
{
	while (1)
	{
        cout << "MiniSQL> ";
		input();
        
        if (query.substr(0, 6) == "create"){
			EXEC_CREATE();
        }
        else if (query.substr(0, 4) == "drop"){
			EXEC_DROP();
        }
        else if (query.substr(0, 6) == "select"){
			EXEC_SELECT();
        }
        else if (query.substr(0, 6) == "insert"){
			EXEC_INSERT();
        }
        else if (query.substr(0, 6) == "delete"){
			EXEC_DELETE();
        }
        else if (query.substr(0, 4) == "quit"){
			return 0;
        }
        else if (query.substr(0, 8) == "execfile"){
			EXEC_FILE();
        }
        else{
			throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use at the beginning of the query");
        }
	}
    return 1;
}

void Interpreter::EXEC_CREATE()
{
	if(query[6] != ' ')
	    throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'create'");
	
	if(query.substr(7, 5) == "table")
	    EXEC_CREATE_TABLE();
	
	else if(query.substr(7, 5) == "index")
	    EXEC_CREATE_INDEX();
	
	else
	    throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'create'");
}

void Interpreter::EXEC_CREATE_TABLE()
{
	if (query[12] != ' ')
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'table'");

	Attribute attri;

	string s = query.substr(13, query.size() - 13);
	istringstream is(s);
	string tablename;
	is >> tablename;

	string t;
	is >> t;
	if (t != "(")
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

	int attrinum = 0;
	while (is >> t) /* get the information of the attributes*/
	{
		if (t == "primary")  //primary key
		{
			is >> t;
			if (attrinum == 0 || t != "key")
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
			is >> t;
			if (t != "(")
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

			is >> t;  //primary key attribute
			int i;
			for (i = 0; i < attrinum; i++)
			if (t == attri.attrName[i])
			{
				attri.primary = i;
				attri.uniqueFlag[i] = true;
				break;
			}
			if (i >= attrinum)
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

			is >> t;
			if (t != ")")
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

			is >> t;
			if (t != ")")
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
			else
				break;
		}

		smatch result;
		regex exp("[^a-zA-Z0-9_]");
		if (regex_search(t, result, exp) != 0)
			throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
		attri.attrName[attrinum] = t;  //attribute name

		is >> t; //attribute type
		if (t == "int") attri.type[attrinum] = -1;
		else if (t == "float") attri.type[attrinum] = 0;
		else if (t == "char")
		{
			is >> t;
			if (t != "(")
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
			is >> t;
			int num;
			if (InvertINT(t, num) == false)
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
			attri.type[attrinum] = num;

			is >> t;
			if (t != ")")
				throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
		}
		else
			throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

		is >> t;
		if (t == "unique")
		{
			attri.uniqueFlag[attrinum] = true;
			is >> t;
		}
		else
			attri.uniqueFlag[attrinum] = false;

		if (t == ",")
			attrinum++;
		else if (t == ")")
		{
			attrinum++;
			break;
		}
		else
			throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");
	}

	is >> t;
	if (t != ";")
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

	attri.attrNum = attrinum;
	Table tableinfo;
	tableinfo.tableName = tablename;
	tableinfo.attr = attri;

	api.createTable(tableinfo);
	cout << "Interpreter: successful!" << endl;
}

void Interpreter::EXEC_CREATE_INDEX(){
    if(query[12] != ' '){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'index\'");
    }
    
    int cursor = 13, nextCursor;
    string keyWord, leftBrace, rightBrace;
    
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string indexName = query.substr(cursor, nextCursor - cursor);
    smatch result;
    regex exp("[^a-zA-Z0-9_]");
    if(regex_search(indexName, result, exp) == 1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + indexName + "'");
    }
    
    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    keyWord = query.substr(cursor, nextCursor - cursor);
    if(keyWord.compare("on") != 0){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + keyWord + "\'");
    }
    
    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string tableName = query.substr(cursor, nextCursor - cursor);
    
    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    leftBrace = query.substr(cursor, nextCursor - cursor);
    if(leftBrace.compare("(") != 0){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + leftBrace + "\'");
    }
    
    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string attributeName = query.substr(cursor, nextCursor - cursor);
    
    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    rightBrace = query.substr(cursor, nextCursor - cursor);
    if(rightBrace.compare(")") != 0){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + rightBrace + "\'");
    }
    
    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor != -1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + query.substr(cursor, nextCursor - cursor) + "\'");
    }
    
    api.createIndex(indexName, tableName, attributeName);
    cout << "Interpreter: successful!" << endl;
	//cout << "indexName: " << indexName << " " << "tableName: " << tableName << " " << "attributeName: " << attributeName << endl;
}

void Interpreter::EXEC_DROP()
{
	if (query[4] != ' ')
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'drop'");

	if (query.substr(5, 5) == "table")
		EXEC_DROP_TABLE();

	else if (query.substr(5, 5) == "index")
		EXEC_DROP_INDEX();

	else
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'drop'");
}

void Interpreter::EXEC_DROP_TABLE()
{
	if (query[10] != ' ')
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'table'");

	string s = query.substr(11, query.size() - 11);
	istringstream is(s);
	string tablename;
	is >> tablename;

	smatch result;
	regex exp("[^a-zA-Z0-9_]");
	if (regex_search(tablename, result, exp) != 0)
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + tablename + "'");

	string t;
	is >> t;
	if (t != ";")
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + t + "'");

	api.dropTable(tablename);
	cout << "Interpreter: successful!" << endl;
}

void Interpreter::EXEC_DROP_INDEX(){
    if(query[10] != ' '){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near 'index'");
    }
    int cursor = 11, nextCursor;
    
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near ';'");
    }
    string indexName = query.substr(cursor, nextCursor - cursor);   

    cursor = query.find_first_not_of(' ', nextCursor);
    nextCursor = query.find_first_of(' ', cursor);
    if(nextCursor != -1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + query.substr(cursor, nextCursor - cursor) + "\'");
    }
    
	api.dropIndex(indexName);
	cout << "Interpreter: successful!" << endl;
	//cout << "indexName: " << indexName << endl;
}

void Interpreter::EXEC_SELECT(){
    if(query[6] != ' '){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'select\'");
    }
    
    int spos = 7, epos;
    
	epos = query.find_first_of(' ', spos);
    if(epos == -1){
		throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string star = query.substr(spos, epos - spos);
    if(star.compare("*") != 0){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + star + "\'");
    }
    
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string from = query.substr(spos, epos - spos);
    if(from.compare("from") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + from + "\'");
    }
    
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string tableName = query.substr(spos, epos - spos);
    //prepare the table:
    smatch result;
    regex exp("[^a-zA-Z0-9_]");
    if(regex_search(tableName, result, exp) == 1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + tableName + "'");
    }
    Catalog ca;
    Table tableinfo = ca.getTable(tableName);

    //prepare the attributes need to be show:
    vector<int> attrIndex;
    attrIndex.clear();
    for(int i = 0; i < tableinfo.attr.attrNum; i++)
    {
        attrIndex.push_back(i);
    }
    //prepare the condition list:
    vector<condition> comp;
    comp.clear();

    //go and find a next string:
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        //call API, 这里相当于没有where条件
        Table outPut = api.select(tableinfo, attrIndex, comp);
        outPut.outputTable();
        cout << "Interpreter: successful!" << endl;
        return;
    }
    string where = query.substr(spos, epos - spos);
    if(where.compare("where") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + where + "\'");
    }
    
    //DEBUG
    //cout << "tableName: " << tableName << endl;

	spos = query.find_first_not_of(' ', epos);
    string whereString = query.substr(spos, query.length()-spos);
    if(whereString.compare(";") == 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + where + "\'");
    }
	comp = ListCondition(tableinfo, whereString);
    Table outPut = api.select(tableinfo, attrIndex, comp);
    outPut.outputTable();
    cout << "Interpreter: successful!" << endl;
    return;
	
    /*
	//prepare the attribute vector need to be show
	vector<int> attrIndex;
	attrIndex.clear();
	if(showNumber == -1)
	{
		for(int i = 0; i < tableInfo.attr.attrNum; i++)
		{
			attrIndex.push_back(i);
		}
	}
	else if(showNumber == 0)
		throw runtime_error("Interpreter: syntex error near \"select\";\n");
	int mem[tableInfo.attr.attrNum];
	memset(mem, 0, sizeof(int) * tableInfo.attr.attrNum);
	else
	{
		for(int i = 0; i < showNumber; i++)
		{
			int j;
			for(j = 0; j < tableInfo.attr.attrNum; j++)
			{
				if(tableInfo.attr.attrName[j] == allAttrShow[i])
				{
					if(mem[j] != 0)
						throw runtime_error("Interpreter: syntex error near \"select\";\n");
					attrIndex.push_back(j);
					mem[j] = 1;
					break;
				}
			}
			if(j == tableInfo.attr.attrNum)
				throw runtime_error("Interpreter: syntex error near \"select\";\n");
		}
	}


     */
	
}
     

void Interpreter::EXEC_DELETE(){
    if(query[6] != ' '){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'delete\'");
    }
    
    int spos = 7, epos;
    
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string from = query.substr(spos, epos - spos);
    if(from.compare("from") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + from + "\'");
    }
    
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string tableName = query.substr(spos, epos - spos);
    //prepare the table:

    Catalog ca;
    Table tableinfo = ca.getTable(tableName);

    //prepare the condition list:
    vector<condition> comp;
    comp.clear();
    
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        //call API, 这里相当于没有where条件
        int count = api.del(tableinfo, comp);
        if(count > 0)
            cout << "Interpreter: successful!" << endl;
        else
            cout << "Nothing was deleted!" << endl;
        return;
    }
    string where = query.substr(spos, epos - spos);
    if(where.compare("where") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + where + "\'");
    }

    spos = query.find_first_not_of(' ', epos);
    string whereString = query.substr(spos, query.length()-spos);
    if(whereString.compare(";") == 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + where + "\'");
    }
    comp = ListCondition(tableinfo, whereString);
    int count = api.del(tableinfo, comp);
    if(count > 0)
        cout << "Interpreter: successful!" << endl;
    else
        cout << "Nothing was deleted!" << endl;
    cout << count << " rows affected." << endl;
    return;
    //DEBUG
    //cout << "tableName: " << tableName << endl;


}

void Interpreter::EXEC_INSERT()
{
    if(query[6] != ' '){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'insert\'");
    }
    
    int spos = 7, epos;
    
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string into = query.substr(spos, epos - spos);
    if(into.compare("into") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + into + "\'");
    }
    
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string tableName = query.substr(spos, epos - spos);
    //prepare the table:
    Catalog ca;
    Table tableinfo = ca.getTable(tableName);

    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string values = query.substr(spos, epos - spos);
    if(values.compare("values") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + values + "\'");
    }
    
    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    string leftBrace = query.substr(spos, epos - spos);
    if(leftBrace.compare("(") != 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + leftBrace + "\'");
    }
    spos = query.find_first_not_of(' ', epos);
    string lineString = query.substr(spos, query.length() - spos);
    if(lineString.compare(";") == 0){
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + leftBrace + "\'");
    }
    Tuple line = LinePreparation(tableinfo, lineString);
    api.insert(tableinfo, line);
    //cout << "Interpreter: successful!" << endl;
    cout << "1 rows affected." << endl;
    return;

    //DEBUG
    //cout << "tableName: " << tableName << endl;


}


void Interpreter::EXEC_FILE()
{
    if(query[8] != ' '){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'execfile\'");
    }

    int spos, epos = 8;
    string fileName;

    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos == -1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
    }
    fileName += query.substr(spos, epos - spos);
    smatch result;
    regex exp("[^.a-zA-Z0-9_]"); //.句号也可以是文件名的一部分
    if(regex_search(fileName, result, exp) == 1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near '" + fileName + "'");
    }

    spos = query.find_first_not_of(' ', epos);
    epos = query.find_first_of(' ', spos);
    if(epos != -1){
        throw invalid_argument("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + query.substr(spos, epos - spos) + "\'");
    }

    ifstream in(fileName);
    if(!in){
        throw invalid_argument("Failed to open file '" + fileName + "'");
    }
    string temp;
    char c;
    int flag, flag1, flag2 = 0; //flag用于标记当前temp的最后一个字符是不是空格
                                //flag1用于标记是否存在未闭合的单引号
                                //flag2用于标记是否读到了文件末尾
    while(in.peek() != EOF){
        try{
        flag = 0;
        flag1 = 0;
        query.clear();
        temp.clear();
        while(1){
            c = in.get();
            if(c == EOF){
                flag2 = 1;
                break;
            }
            if(flag1 == 1){
                temp += c;
                if(c == '\'' || c == '\"'){
                    temp += ' ';
                    flag = 1;
                    flag1 = 0;
                }
                continue;
            }
            if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '.'|| (c >= '0' && c <= '9') || c == '-'){
                temp += c;
                flag = 0;
            }
            else if(c == '*' || c == ',' || c == '(' || c == ')'){
                if(flag == 1){
                    ;
                }
                else{
                    temp += ' ';
                }
                temp += c;
                temp += ' ';
                flag = 1;
            }
            else if(c == '>' || c == '<' || c == '='){
                if(flag == 1){
                    if(temp[temp.length() - 2] == '>'
                       || temp[temp.length() - 2] == '<'
                       || temp[temp.length() - 2] == '='){
                        temp[temp.length() - 1] = c;
                        temp += ' ';
                    }
                    else{
                        temp += c;
                        temp += ' ';
                    }
                }
                else{
                    temp += ' ';
                    temp += c;
                    temp += ' ';
                    flag = 1;
                }
            }
            else if(c == '\'' || c == '\"'){
                flag1 = 1;
                if(flag == 0){
                    temp += ' ';
                }
                else{
                    ;
                }
                temp += c;
            }
            else if(c == '\n' || c == '\t' || c == ' ' || c == '\r'){
                if(flag == 1){
                    ;
                }
                else{
                    flag = 1;
                    temp += ' ';
                }
            }
            else if(c == ';'){
                if(flag == 1){
                    ;
                }
                else{
                    temp += ' ';
                }
                temp += c;
                break;
            }
            else{
                cout << "illegal character" << endl;
                while(cin.get() != ';'){
                    ;
                }
                temp.clear();
            }
        }
        if(flag2 == 1){
            break;
        }
        int start = temp.find_first_not_of(' ', 0);
        query = temp.substr(start, temp.length() - start);

        if (query.substr(0, 6) == "create"){
            EXEC_CREATE();
        }
        else if (query.substr(0, 4) == "drop"){
            EXEC_DROP();
        }
        else if (query.substr(0, 6) == "select"){
            EXEC_SELECT();
        }
        else if (query.substr(0, 6) == "insert"){
            EXEC_INSERT();
        }
        else if (query.substr(0, 6) == "delete"){
            EXEC_DELETE();
        }
        else if (query.substr(0, 4) == "quit"){
            break;
        }
        else if (query.substr(0, 8) == "execfile"){
            EXEC_FILE();
        }
        else{
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use at the beginning of the query");
        }
        }
        catch(exception& ex){
            cout << ex.what() << endl;
        }
    }
}


vector<condition> Interpreter::ListCondition(Table& tableInfo, string whereString)
{
	int spos, epos;
	int iM;
	float fM;
	string sM;
	vector<condition> conditionList;
	conditionList.clear();
	epos = 0;
    for(;;)
    {
        whereString = whereString.substr(epos, whereString.length() - epos);
        spos = whereString.find_first_not_of(" ", 0);
        epos = whereString.find_first_of(" ", spos);
        if(epos == -1)
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");


        //judge the attribute name:
        string attrA = whereString.substr(spos, epos - spos);
        if(attrA.compare("and") == 0){
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + attrA + "\'");
        }
        int i;
        condition* temp = new condition;
        for(i = 0; i < tableInfo.attr.attrNum; i++)
        {
            if(attrA == tableInfo.attr.attrName[i])
            {
                temp->number = i;
                break;
            }
        }
        if(i == tableInfo.attr.attrNum){
            delete temp;
            throw runtime_error("No such attribute called \'" + attrA + "\'");
        }

        //judge the character:
        spos = whereString.find_first_not_of(" ", epos);
        epos = whereString.find_first_of(" ", spos);
        if(epos == -1){
            delete temp;
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
        }
        string character = whereString.substr(spos,epos - spos);
        if(character.compare("and") == 0){
            delete temp;
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + character + "\'");
        }
        if     (character == "=")	temp->flag = E;
        else if(character == "<=")  temp->flag = SE;
        else if(character == ">=")  temp->flag = BE;
        else if(character == "<" )  temp->flag = S;
        else if(character == ">" )  temp->flag = B;
        else if(character == "<>")  temp->flag = NE;
        else {
            delete temp;
            throw runtime_error("No such character called \'" + character + "\'");
        }

        // judge the condition:
        spos = whereString.find_first_not_of(" ", epos);
        epos = whereString.find_first_of(" ", spos);
        if(epos == -1){
            delete temp;
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
        }
        string condition = whereString.substr(spos,epos - spos);
        if(condition.compare("and") == 0){
            delete temp;
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + condition + "\'");
        }
        if(tableInfo.attr.type[i] == -1 && InvertINT(condition, iM))//INT
        {
            Data* Message = new Datai(iM);
            temp->message = Message;
            conditionList.push_back(*temp);
        }
        else if(tableInfo.attr.type[i] == 0 && InvertFLOAT(condition, fM))//FLOAT
        {
            Data* Message = new Dataf(fM);
            temp->message = Message;
            conditionList.push_back(*temp);
        }
            /*
        else if(tableInfo.attr.type[i] > 0 && condition.length()-2 <= tableInfo.attr.type[i] && condition.c_str()[0] =='\'' && condition.c_str()[condition.length()-1] == '\'')//STRING
        {

            Data* Message = new Datac(condition.substr(1, condition.length()-2));
            temp->message = Message;
            conditionList.push_back(*temp);
        }
             */
        else if(tableInfo.attr.type[i] > 0 && ((condition.c_str()[0] == '\''&& condition.length() - 2 <= tableInfo.attr.type[i]) || (condition.c_str()[0] == '\"'&& condition.length()-2 <=tableInfo.attr.type[i]) || (condition == "\'") ||(condition == "\"" )))
        {

            epos = whereString.find_first_of("\'\"", spos + 1);
            if(epos == -1)
            {
                throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
            }
            epos++;
            condition = whereString.substr(spos, epos - spos);
            /*
            while(condition.c_str()[condition.length() - 1] != '\'')
            {
                
                spos = whereString.find_first_not_of(" ", epos);
                epos = whereString.find_first_of(" ", spos);
                if(epos == -1)
                {
                    throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
                }
                condition += " " + whereString.substr(spos, epos - spos);

            }
             */
            if(condition.length() - 2 <= tableInfo.attr.type[i])
            {
                Data* Message = new Datac(condition.substr(1, condition.length()-2));
                temp->message = Message;
                conditionList.push_back(*temp);
            }
            else
            {
                throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + condition + "\'");
            }
        }
        else {
            delete temp;
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + condition + "\'");
        }

        //judge "and" and ";":
        spos = whereString.find_first_not_of(" ", epos);
        epos = whereString.find_first_of(" ", spos);
        if(epos == -1){
            break;
        }
        string And = whereString.substr(spos, epos - spos);
        if(And.compare("and") != 0){
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + And + "\'");
        }
        epos = whereString.find_first_not_of(" ", epos);

    }

	return conditionList;

}

Tuple Interpreter::LinePreparation(Table& tableInfo, string lineString)
{
	//..."31501XXXXX , Alice , 19 , 95.5 , Math ) ;"
	int spos, epos;
	int iM;
	float fM;
	Tuple temp;
	temp.element.clear();
    epos = 0;
    int i;
	for(i = 0; i < tableInfo.attr.attrNum; i++)
	{
		//"  '31501XXXXX' , 'Alice' , 19 , 95.5 , 'Math'  "
		spos = lineString.find_first_not_of(" ", epos);
		epos = lineString.find_first_of(" ", spos);
		if(epos == -1){
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
        }
		string tempWord = lineString.substr(spos, epos - spos);
		if(tableInfo.attr.type[i] == -1 && InvertINT(tempWord, iM))//INT
		{
			Data* Message = new Datai(iM);
			temp.element.push_back(Message);
		}
		else if(tableInfo.attr.type[i] == 0 && InvertFLOAT(tempWord, fM))//FLOAT
		{
			Data* Message = new Dataf(fM);
			temp.element.push_back(Message);
		}
            /*
		else if(tableInfo.attr.type[i] > 0 && tempWord.length()-2 <= tableInfo.attr.type[i] && tempWord.c_str()[0] =='\'' && tempWord.c_str()[tempWord.length()-1] == '\'')
		{
			Data* Message = new Datac(tempWord.substr(1, tempWord.length() - 2));
			temp.element.push_back(Message);
		}
             */
        else if((tableInfo.attr.type[i] > 0) && (((tempWord.c_str()[0] == '\'')&& (tempWord.length() - 2 <= tableInfo.attr.type[i])) ||((tempWord.c_str()[0] == '\"')&& (tempWord.length()-2 <=tableInfo.attr.type[i]) )|| (tempWord == "\'")|| (tempWord == "\"")))
        {

            epos = lineString.find_first_of("\'\"", spos + 1);
            if(epos == -1)
            {
                throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
            }
            epos++;
            tempWord = lineString.substr(spos, epos - spos);
            /*
            while(tempWord.c_str()[tempWord.length() - 1] != '\'')
            {
                spos = lineString.find_first_not_of(" ", epos);
                epos = lineString.find_first_of(" ", spos);
                if(epos == -1)
                {
                    throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
                }
                tempWord += " " + lineString.substr(spos, epos - spos);

            }*/
            if(tempWord.length() - 2 <= tableInfo.attr.type[i])
            {
                Data* Message = new Datac(tempWord.substr(1, tempWord.length() - 2));
                temp.element.push_back(Message);
            }
            else
            {
                throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + tempWord + "\'");
            }
        }
		else{
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + tempWord + "\'");
        }
        // judge the ")" and the ",":
        spos = lineString.find_first_not_of(" ", epos);
        epos = lineString.find_first_of(" ", spos);
        if(epos == -1){
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'");
        }
        string stop = lineString.substr(spos, epos - spos);
        if(stop.compare(")") == 0)
            break;
        else if(stop.compare(",") == 0){
            epos = lineString.find_first_not_of(" ", epos);
        }
        else{
            throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \'" + stop + "\'");
        }

	}
	if(i < tableInfo.attr.attrNum - 1 )
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \')\'");
    spos = lineString.find_first_not_of(" ", epos);
    epos = lineString.find_first_of(" ", spos);
	if(epos != -1) {
        throw runtime_error("You have an error in your SQL syntax; check the manual that corresponds to your miniSQL server version for the right syntax to use near \';\'233");
	}
	return temp;
}


bool InvertINT(string s, int& x)
{
    x = 0;
    for(int i = 0; i < s.length(); i++)
    {
        if(s[i] > '9' || s[i] < '0')
            return false;
        x = x * 10 + s[i] - '0';
    }
    return true;
}

bool InvertFLOAT(string s, float& x)
{
    x = 0;
    int intHalf;
    int dot = s.find_first_of('.', 0);
    if(dot == -1){
        if(!InvertINT(s, intHalf))
            return false;
        x = intHalf;
        return true;
    }
    string intStr = s.substr(0, dot);

    if(!InvertINT(intStr, intHalf))
        return false;
    x += intHalf;
    float deciHalf = 0;
    for(int i = s.length() - 1; i > dot; i--)
    {
        if(s[i] > '9' || s[i] < '0')
            return false;
        deciHalf = (deciHalf + s[i] - '0') / 10; 
    }
    x += deciHalf;
    return true;
}
