# README

这是2017春夏学期数据库系统miniSQL大程

团队成员：余南龙 康自蓉 胡诗佳

详细设计及分工请见MiniSQL总体设计报告

运行本程序需要有支持C++11的编译器，需要支持\<regex\>库

使用命令行进行编译

```shell
cd 工程所在文件夹
g++ API.cpp base.cpp bptree.cpp buffer.cpp Catalog.cpp indexmanager.cpp interpreter.cpp main.cpp recordmanager.cpp
./a.out
```

文件目录中有已经生成的a.out可执行文件，也可以直接运行

如果输入的语句中包含非法字符而非法字符后又没有分号，需要输入分号后才能开始新的输入，非法字符后直到其后第一个分号之间的内容均无效































