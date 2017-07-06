#include <iostream>
#include "interpreter.h"
#include "buffer.h"

using namespace std;

BufferManage buffer;

int main() {
    Interpreter A;
    int isQuit = 1;
    cout << "Welcome to the MiniSQL monitor" << endl;
    while(isQuit){
        try{
            isQuit = A.EXEC(); //为0break
        }
        catch(exception& ex){
            cout << ex.what() << endl;
        }
    }
    cout << "Bye" << endl;
    return 0;
}