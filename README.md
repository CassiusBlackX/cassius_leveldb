# 使用方法
```shell
mkdir build && cd build
cmake ..
make ${you-target-name}
```
编辑项目根目录的CMakeLists.txt,每一个`add_executable`对应一个`target_name`,每个target之间有空行和注释隔开.

如果需要打开一定的特性,需要选择性的开启编译开关,建议只进行取消注释特定编译开关,不要新加

编译开关说明:
+ `CMAKELISTS_PATH`: 宏,用来获取项目根目录CMakeLists.txt的路径,方便在文件内部使用该宏,确定一些其它文件的路径
+ `LOG_SST`: 记录每个sst的相关信息,使用方法同`src/log_sst.cpp`,需要复制`compatcion_info_index`以及后面四行全局变量.并在最后打印.同时需要包括头文件zal_utils.h 针对此编译开关对应的所有修改,只要全局搜索LOG_SST宏就可以找到
+ `LOG_COMPACTION`: 当size或seek compaction被触发的时候,该编译开关若被打开,则会打印“triggered XXX compaction"的信息.如果需要追求性能测试,则应当关闭.主要是为了能够真的感知到数据库正在运行,不然就什么输出都没有了
+ `PRINT_LEVEL`: 当每次调用到`LogAndApply`函数的时候会打印出每个level的table的索引,因为一般触发不到level3以上的sst，所以在源码中写死了只打印到level3，如果有需要可以去修改`LogAndApply`函数中的`PRINT_LEVEL`部分
+ 