# YCSB使用说明

## worklaods
workloads：目录下有各种workload的模板，可以基于workload模板进行自定义修改。

默认的6种测试场景如下：
1. `workloada`：读写均衡型，50%/50%，Reads/Writes
2. `workloadb`：读多写少型，95%/5%，Reads/Writes
3. `workloadc`：只读型，100%，Reads
4. `workloadd`：读最近写入记录型，95%/5%，Reads/insert
5. `workloade`：扫描小区间型，95%/5%，scan/insert
6. `workloadf`：读写入记录均衡型，50%/50%，Reads/insert

## 参数
### 命令参数
+ `command`：常用 load（用于压测前的数据准备）和 run（执行压测） 
+ `database`：压测数据库名称
+ `options`：-P（指定workload文件）-p key=value（覆盖workload中属性）-threads （进程数）-s(执行过程中是否打印状态信息)

### 属性参数
+ `recordcount`: load阶段加载到数据库的纪录条数 (default: 0) ，run阶段操作的数据范围（注：run阶段该值不能大于load阶段的值，否则会出现 Nothing updated for key的错误，该错误会影响update操作的正确性）
+ `operationcount`: run阶段执行的操作总数
+ `readallfields`: 查询时是否读取所有字段true或者读取一个字段false(default: true)
+ `fieldcount`: 每条记录的字段个数 (default: 10)
+ `fieldlength`: 每个字段的数据长度 (default: 100)  value长度补齐
+ `zeropadding`: 每个key的长度补齐
+ `readproportion`: 读操作比例 (default: 0.95)
+ `updateproportion`: 更新操作比例 (default: 0.05)
+ `insertproportion`: 插入操作比例 (default: 0)
+ `insertstart`: 第一个插入值的偏移量(default: 0)
+ `scanproportion`: 扫描作业比例 (default: 0)
+ `readmodifywriteproportion`: 读取一条记录修改它并写回的比例 (default: 0)
+ `requestdistribution`: 请求的分布规则 uniform, zipfian or latest (default: uniform)
+ `mongodb`.url：待测试mongo实例的连接地址
+ `mongodb`.database：测试时使用的数据库名称(default: ycsb)注：该参数在实际使用过程为生效，但也不报错，版本原因？
+ `table`: 测试表的名称 (default: usertable)