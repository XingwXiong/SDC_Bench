# cat ~/.ssh/id_rsa.pub >> ./authorized_keys

 docker run -it \
    -v /home/xingw/SDC_Bench/x86/dwarf-sim:/root/dwarf-sim \
    --publish-all\
    --name=dwarf --hostname=dwarf \
    xingw/centos:v2 bash
    #| xargs -it docker exec {} ifconfig
    #--name=latest --hostname=latest \
    #-p 10000:10000 -p 10001:10001 -p 10002:10002 -p 10003:10003 \
    #-p 10004:10004 -p 10005:10005 -p 10006:10006 -p 10007:10007 \
