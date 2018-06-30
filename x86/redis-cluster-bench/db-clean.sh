#!/bin/bash
redis_list=("172.18.11.114:7000" "172.18.11.114:7001" "172.18.11.114:7002" "172.18.11.114:7003" "172.18.11.114:7004" "172.18.11.114:7005")
password="redispassword=="

for info in ${redis_list[@]}
    do
        echo "开始执行:$info"  
        ip=`echo $info | cut -d : -f 1`
        port=`echo $info | cut -d : -f 2`
        #cat key.txt |xargs -t -n1 redis-cli -h $ip -p $port -a $password -c del
        redis-cli -h $ip -p $port keys __test_key__* > key.txt
	    cnt=`echo key.txt | wc -l`
        if [[ $cnt -le 1 ]]; then continue; fi
        cat key.txt |xargs -t -n1 redis-cli -h $ip -p $port -c del
    done
    echo "完成"
