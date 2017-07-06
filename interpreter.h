#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include <string>
#include <vector>
#include "base.h"

class Interpreter{
private:
	std::string query;
public:
	Interpreter();
	~Interpreter();
	void input();
	int EXEC();
	void EXEC_CREATE();
	void EXEC_CREATE_TABLE();
	void EXEC_CREATE_INDEX();
	void EXEC_DROP();
	void EXEC_DROP_TABLE();
	void EXEC_DROP_INDEX();
	void EXEC_SELECT();
	void EXEC_INSERT();
	void EXEC_DELETE();
	void EXEC_FILE();
    void debugPrint();

	std::vector<condition> ListCondition(Table& tableInfo, std::string whereString);
	Tuple LinePreparation(Table& tableInfo, std::string lineString);
};

bool InvertINT(std::string s, int& x);
bool InvertFLOAT(std::string s, float& x);

#endif
