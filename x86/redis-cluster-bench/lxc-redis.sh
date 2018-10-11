# cat ~/.ssh/id_rsa.pub >> ./authorized_keys

 docker run -it \
    --publish-all \
    -p 7000:7000 \
    --hostname=lxc-redis --name=lxc-redis \
    xingw/centos:v2 bash
    #| xargs -it docker exec {} ifconfig
    #--name=latest --hostname=latest \
    #-p 10000:10000 -p 10001:10001 -p 10002:10002 -p 10003:10003 \
    #-p 10004:10004 -p 10005:10005 -p 10006:10006 -p 10007:10007 \
