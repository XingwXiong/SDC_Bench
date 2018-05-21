# coding: UTF-8
import time, math, random, threading, http.client
#import requests, time, math, random, threading, http.client


WARMUP=200
QPS=100
REQUESTS=200
THREADS=10
HOST="10.30.5.47"
PORT="8000"
#HOST="172.18.11.103"
#PORT=8000
tot_cnt = 0
sec_cnt = 0
loop_num = 0
warm_cnt = 0
start_time = 0
pre_time = 0

thread_lock = threading.Lock()


def mstime():
    return math.ceil(time.time() * 1000)

def ustime():
    return math.ceil(time.time() * 1000000)

def get_url(_type, index):
    keys = ["name", "age", "sex", "phone", "id"]
    vals = ["test", "21", "male", "18812341234", "123456"]
    r = random.randint(0, len(keys) - 1)
    switcher = {
        "PING": "PING",
        "SET": "SET/%s/%s_%d" % (keys[r], vals[r], index),
        "GET": "GET/%s" % keys[r],
        "INCR": "INCR/%s" % keys[r],
        "TYPE": "TYPE/%s" % keys[r],
    }
    return "/%s" % switcher.get(_type, "")
    #return "http://%s:%d/%s" % (HOST, PORT, switcher.get(_type, ""))

def thread_exec(thread_name, _type, latency):
    global tot_cnt, sec_cnt, loop_num, warm_cnt, start_time, pre_time
    while True :
        if tot_cnt >= REQUESTS:
            return
        if sec_cnt >= QPS:
            #print("mstime()=%f" % mstime());
            #print("pre_time=%f" % pre_time);
            #print("delta=%f" % (mstime() - pre_time))
            #print("val=%f" % (1.0 - (mstime() - pre_time) / 1000))
            sleep_time = (1.0 - (mstime() - pre_time) / 1000)
            if sleep_time > 0:
                time.sleep(1.0 - (mstime() - pre_time) / 1000)
            thread_lock.acquire()
            loop_num += 1
            pre_time = mstime()
            sec_cnt = 0
            thread_lock.release()
            continue;
        if warm_cnt >= WARMUP and pre_time + 1000 <= mstime():
            thread_lock.acquire()
            loop_num += 1
            sec_cnt = 0
            pre_time = mstime()
            thread_lock.release()
            #print("[Info: Your QPS has reached the climax!]")
            continue;
        url = get_url(_type, tot_cnt)
        #requests.get(url)
        conn = http.client.HTTPConnection(HOST + ':' + PORT)
        conn.request("GET", url)
        res = conn.getresponse()
        if res.status != 200:
            print("Err: status: %d" % res.status)
        if warm_cnt < WARMUP:
            thread_lock.acquire()
            warm_cnt += 1
            thread_lock.release()
            if warm_cnt == WARMUP:
                pre_time = start_time = mstime()
            continue
        latency.append(mstime() - start_time)
        thread_lock.acquire()
        tot_cnt += 1
        sec_cnt += 1
#        print(thread_name)
        #print("tot_cnt: %d" % tot_cnt)
        thread_lock.release()

def benchmark(_type="DEBUG"):
    global tot_cnt, sec_cnt, loop_num, warm_cnt, start_time, pre_time
    print("==============Parameters==============")
    print("WARMUP:\t\t%d\nQPS:\t\t%d\nREQUESTS:\t%d\nTHREADS:\t%d\nHOST:\t\t%s\nPORT:\t\t%s\n" % (WARMUP,
        QPS, REQUESTS, THREADS, HOST, PORT))
    threads=[]
    latency=[]
    tot_cnt = sec_cnt = loop_num = warm_cnt = start_time = 0
    for i in range(THREADS):
        thread_name = "thread_%d" % i
        t = threading.Thread(target=thread_exec, args=(thread_name,_type, latency,))
        t.setDaemon(True)
        threads.append(t)
    pre_time = mstime()
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    totlatency = mstime() - start_time
    show_latency_report(_type, totlatency, latency)

def show_latency_report(_type, totlatency, latency):
    latency.sort()
    print("==============TYPE: %s==============" % _type)
    print("  %d requests completed in %.2f seconds" % (REQUESTS, totlatency/1000))
    curlat = 0
    reqpersec = REQUESTS / (totlatency / 1000)
    # print("latency: ", latency)
    i = 0
    for item in latency:
        if (math.ceil(item / 1000) != curlat) or (item == REQUESTS - 1):
            curlat = math.ceil(item / 1000)
            perc = (i + 1) * 100 / REQUESTS
            print("%.2f%% <= %d milliseconds" % (perc, curlat))
        i = i + 1
    print("%.2f requests per second\n" % reqpersec)


if __name__ == "__main__":
    benchmark("SET")
    benchmark("GET")
    #benchmark("INCR")
    #benchmark("TYPE")
