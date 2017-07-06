#include <iostream>
#include "base.h"

Tuple::~Tuple(){
    for(int i = 0; i < element.size(); i++){
        if(element[i] != NULL){
            if(element[i]->type == -1){
                delete (Datai*)element[i];
            }
            else if(element[i]->type == 0){
                delete (Dataf*)element[i];
            }
            else{
                delete (Datac*)element[i];
            }
        }
    }
}
/* 打印一个tuple */
void Tuple::outputTuple(){
    for(int i = 0; i < element.size(); i++){
        if(element[i] == NULL){
            std::cout << "NULL" ;
        }
        else if(element[i]->type == -1){
            std::cout << ((Datai*)element[i])->val ;
        }
        else if(element[i]->type == 0){
            std::cout << ((Dataf*)element[i])->val ;
        }
        else{
            std::cout << ((Datac*)element[i])->val ;
        }
        if(i!= element.size() - 1)
            std::cout << "|";
    }
    std::cout << std::endl;
}

Table::~Table(){
    for(int i = 0; i < tuple.size(); i++){
        delete tuple[i];
    }
}
/* 打印整张表 */
void Table::outputTable(){
    for(int i = 0; i < attr.attrNum; i++){
        std::cout << attr.attrName[i] << '\t';
    }
    std::cout << std::endl;
    for(int i = 0; i < tuple.size(); i++){
        tuple[i]->outputTuple();
    }
    std::cout << "Query OK: " << tuple.size() << " rows in set." << std::endl;
}
