#!/bin/bash

# 设置工作目录和日志目录
WORKLOAD_DIR="../ycsb/workloads"
LOG_DIR="../log"
EXECUTABLE="../build/ycsb_compaction_trace"  # 替换为你的可执行文件路径

# 确保日志目录存在
mkdir -p "$LOG_DIR"

# 初始化索引
index=0

# 查找现有的日志文件，确定下一个索引
while [ -f "$LOG_DIR/${index}.log" ]; do
    index=$((index + 1))
done

# 生成随机的 proportion 变量值
generate_proportions() {
    while true; do
        insertproportion=$(awk -v min=0.65 -v max=0.99 'BEGIN{srand(); print min+rand()*(max-min)}')
        readproportion=$(awk 'BEGIN{srand(); print rand()*(1-0.65)}')
        updateproportion=$(awk 'BEGIN{srand(); print rand()*(1-0.65)}')
        deleteproportion=$(awk 'BEGIN{srand(); print rand()*(1-0.65)}')

        total=$(awk -v i=$insertproportion -v r=$readproportion -v u=$updateproportion -v d=$deleteproportion 'BEGIN{print i+r+u+d}')
        
        if (( $(echo "$total <= 1" | bc -l) )); then
            deleteproportion=$(awk -v i=$insertproportion -v r=$readproportion -v u=$updateproportion 'BEGIN{print 1-i-r-u}')
            break
        fi
    done
}

# 循环生成100次 proportion 变量值并执行
for ((i=0; i<100; i++)); do
    # 生成 proportion 变量值
    generate_proportions
    zipfian_const=$(awk -v min=0.5 -v max=1.00 'BEGIN{srand(); print min+rand()*(max-min)}')

    # 创建 workloads_temp.ini 文件
    cat <<EOL > "$WORKLOAD_DIR/workloads_temp.ini"
recordcount=1048576
operationcount=3145728
insertproportion=$insertproportion
readproportion=$readproportion
updateproportion=$updateproportion
scanproportion=0
deleteproportion=$deleteproportion
workload=zal.test.io.ssd
requestdistribution=zipfian
zipfianconst=$zipfian_const
zeropadding=20
fieldlength=986
fieldcount=1
EOL

    # 打印 proportion 变量值到日志文件
    echo "insertproportion=$insertproportion;" > "$LOG_DIR/${index}.log"
    echo "readproportion=$readproportion;" >> "$LOG_DIR/${index}.log"
    echo "updateproportion=$updateproportion;" >> "$LOG_DIR/${index}.log"
    echo "scanproportion=0;" >> "$LOG_DIR/${index}.log"
    echo "deleteproportion=$deleteproportion;" >> "$LOG_DIR/${index}.log"
    echo "zipfian_const=$zipfian_const;" >> "$LOG_DIR/${index}.log"
    echo "" >> "$LOG_DIR/${index}.log"

    # 运行可执行文件并将输出重定向到日志文件
    "$EXECUTABLE" >> "$LOG_DIR/${index}.log" 2>&1

    # 确保之前的数据库已经不在了
    rm -rf testdb/

    # 增加索引
    index=$((index + 1))
    echo "Iteration $((i + 1)) completed."
done

echo "Completed 100 iterations of workload generation and execution."