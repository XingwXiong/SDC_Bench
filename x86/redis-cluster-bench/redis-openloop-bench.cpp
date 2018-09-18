#include <unistd.h>
#include <sys/time.h>
#include <bits/stdc++.h>
#include <acl_cpp/lib_acl.hpp>
#include <pthread.h>
#include "dist.h"

using namespace std;

struct Config {
    int client_num;
    vector<pthread_t> clients_pd;
    pthread_t display_pd;
    long long display_las_time;
    long long display_las_index;

    enum Status{WARMUP, RUNNING, FINISHED} status; // 0--warmup 1--running 2--finished
    int sem_cnt;
    string ip_addr;
    int port;
    string auth;
    int requests_finished;
    int requests;
    int qps;
    int warmup_requests;

    vector<string> task; //-t get,set
    string type; // current task type
    long long start_tm;
    long long total_tm;
    long long interval;
    int round, total_round;
    vector<long long> lats;

    pthread_mutex_t lats_mutex;
    FILE* fd_rep, *fd_lat; 
} config;
ExpDist dist; // exponential_distribution

acl::redis_client_cluster cluster; // 定义 redis 客户端集群管理对象

static long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

static long long mstime(void) {
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long long)tv.tv_sec) * 1000;
    mst += tv.tv_usec / 1000;
    return mst;
}

static long long nstime() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
}

static void sleepUntilUs(long long targetUs) {
    long long curUs = ustime();
    while(curUs < targetUs) {
        long long diffUs = targetUs - curUs;
        struct timespec ts = {(time_t)diffUs / (1000 * 1000), (time_t)diffUs % (1000 * 1000) * 1000};
        nanosleep(&ts, NULL);
        curUs = ustime();
    }
}

static void sleepUntilNs(long long targetNs) {
    long long curNs = nstime();
    while(curNs < targetNs) {
        long long diffNs = targetNs - curNs;
        struct timespec ts = {(time_t)diffNs / (1000 * 1000 * 1000), (time_t)diffNs % (1000 * 1000 * 1000)};
        nanosleep(&ts, NULL);
        curNs = nstime();
    }
}

acl::string gen_key() {
    //const int size = 3;
    const int size = 2;
    acl::string ret = "__test_key__";
    for(int i = 0; i < size; ++i) {
        ret += (char)(rand() % 10 + '0');
    }
    return ret;
}

acl::string gen_val() {
    const int size = 8;
    acl::string ret = "__test_val__";
    for(int i = 0; i < size; ++i) {
        ret += (char)(rand() % 10 + '0');
    }
    return ret;
}

bool test_set(acl::redis_string &cmd) {
    acl::string key = gen_key();
    acl::string val = gen_val();
    bool ret = cmd.set(key, val);
    if (ret == false) {
        const acl::redis_result *res = cmd.get_result();
        printf("set key: error: %s\r\n",
               res ? res->get_error() : "unknown error");
        return false;
    }
    assert(ret == true);
    key.clear();
    val.clear();
    cmd.clear();
    return true;
}

bool test_get(acl::redis_string &cmd) {
    acl::string key = gen_key();
    acl::string val = gen_val();
    //assert(cmd.get(key, val) == true);
    bool ret = cmd.get(key, val);
    if (ret == false) {
        const acl::redis_result *res = cmd.get_result();
        printf("get key: error: %s\r\n",
               res ? res->get_error() : "unknown error");
        return false;
    }
    assert(ret == true);
    key.clear();
    val.clear();
    cmd.clear();
    return true;
}

void *processRequests(void *e) {
    bool (* func)(acl::redis_string & cmd) = NULL;
    if(config.type == "SET") {
        func = &test_set;
    } else if(config.type == "GET") {
        func = &test_get;
    } else {
        fprintf(stderr, "type error: no such type %s\n", config.type.c_str());
        exit(-1);
    }
    acl::redis_string cmd_string;
    cmd_string.set_cluster(&cluster, 100); // max_conn = 100
    
    // warmup_request
    pthread_mutex_lock(&config.lats_mutex);
    while(config.requests_finished < config.warmup_requests) {
        long long lat_bg = dist.nextArrivalNs();
        if(nstime() < lat_bg) sleepUntilNs(lat_bg);
        pthread_mutex_unlock(&config.lats_mutex);
        func(cmd_string);
        pthread_mutex_lock(&config.lats_mutex);
        ++ config.requests_finished;
    }
   // cout << "finished" << endl;
    -- config.sem_cnt;
    pthread_mutex_unlock(&config.lats_mutex);
    while(config.sem_cnt > 0); 
    pthread_mutex_lock(&config.lats_mutex);
  //  cout << "begin" << endl;
    // init
    config.requests_finished = 0;
    config.status = Config::RUNNING;
    // processRequsts
    while(config.requests_finished < config.requests) {
        //if(config.round >= config.total_round) break;
        long long lat_bg = dist.nextArrivalNs();
        if(nstime() < lat_bg) sleepUntilNs(lat_bg);
        pthread_mutex_unlock(&config.lats_mutex);
        lat_bg = ustime();
        func(cmd_string);
        long long lat_ed = ustime();
        pthread_mutex_lock(&config.lats_mutex);
        ++ config.requests_finished;
        if(config.round >= config.total_round) continue;
        config.lats.push_back(lat_ed - lat_bg);
        config.total_tm = lat_ed - config.start_tm;
    }
    pthread_mutex_unlock(&config.lats_mutex);
}

void show_total_report() {
    printf("============================%s============================\n", config.type.c_str());
    long double mean_lat = 0;
    for(auto &lat : config.lats) {
        mean_lat += lat;
        fprintf(config.fd_lat, "%lld\n", lat);
    }
    fflush(config.fd_lat);
    mean_lat = config.requests_finished ? mean_lat / config.requests_finished : 0;
    long double qps = config.total_tm ? (long double)config.requests_finished / (config.total_tm / 1000000.0) : 0;
    sort(config.lats.begin(), config.lats.end());
    double pers[] = {0, 50, 80, 95, 99, 99.9, 100};
    int pers_size = 7; //sizeof(pers) / sizeof(double);
    printf("%8s|%8s", "QPS", "MEAN");
    for(int i = 0; i < pers_size; ++ i) {
        printf("|%7.2f%%", pers[i]);
    }
    printf(" (unit: us)\t(%10d requests)\n", (int)config.lats.size());
    printf("%8.2Lf|%8.2Lf", qps, mean_lat);
    for (int i = 0; i < pers_size; ++ i) {
        int pos = pers[i] / 100 * max(0, (int)config.lats.size() - 1);
        printf("|%8lld", config.lats[pos]);
    }
    printf("\n");
    fflush(stdout);
    printf("============================%s============================\n", "FINISHED");
}

static void *show_report(void *) {
    static double pers[] = {0, 50, 80, 95, 99, 99.9, 100};
    static int pers_size = 7; //sizeof(pers) / sizeof(double);
    while(config.status == Config::WARMUP);
    pthread_mutex_lock(&config.lats_mutex);
    config.lats.clear();
    config.start_tm = config.display_las_time = ustime();
    config.display_las_index = 0; 
    pthread_mutex_unlock(&config.lats_mutex);
    sleepUntilNs(config.interval * 1e9); 
    
    while(config.requests_finished < config.requests &&  config.display_las_index < config.requests) {
        //int requests_finished = config.requests_finished;
        int requests_finished = config.lats.size();
        long long cur_time = ustime();
        long long delta_tm = cur_time - config.display_las_time;
        long long delta_sz = requests_finished - config.display_las_index;
        if(delta_tm < (long long)config.interval * 1000000) continue;
        if(config.round >= config.total_round) break;
        config.round += 1;
        vector<long long> lat_buf;
        long double mean_lat = 0;

        for(int i = config.display_las_index; i < requests_finished; ++i) {
            lat_buf.push_back(config.lats[i]);
            mean_lat += config.lats[i];
        }
        assert(lat_buf.size() != 0);
        mean_lat = delta_sz ? mean_lat / delta_sz : 0;
        long double qps = delta_tm ? (long double) delta_sz / (delta_tm / 1000000.0) : 0;
        sort(lat_buf.begin(), lat_buf.end());
        printf("==============%s==============\n", config.type.c_str());
        printf("%8s|%8s", "QPS", "MEAN");
        for(int i = 0; i < pers_size; ++ i) {
            printf("|%7.2f%%", pers[i]);
        }
        printf(" (unit: us)\t(%10d requests)\n", (int)lat_buf.size());
        printf("%8.2Lf|%8.2Lf", qps, mean_lat);
        for (int i = 0; i < pers_size; ++ i) {
            int pos = pers[i] / 100 * max(0, (int)lat_buf.size() - 1);
            printf("|%8lld", lat_buf[pos]);
        }
        printf("\n");
        fprintf(config.fd_rep, "%8.2LF,%8.2Lf", qps, mean_lat);
        for(int i = 0; i < pers_size; ++ i) {
            int pos = pers[i] / 100 * max(0, (int)lat_buf.size() - 1);
            fprintf(config.fd_rep, ",%8lld", lat_buf[pos]);
            fflush(config.fd_rep);
        }
        fprintf(config.fd_rep, "\n");
        fflush(config.fd_rep);
        config.display_las_time = ustime();
        config.display_las_index = requests_finished + 1;
    }
    //config.status = Config::FINISHED;
    show_total_report();
}

void benchmark(const string &type) {
    char file_name[50];
    config.type = type;
    config.requests_finished = 0;
    config.lats.clear();
    pthread_mutex_init(&config.lats_mutex, NULL);
    config.clients_pd.clear();
    config.clients_pd.resize(config.client_num);
    config.start_tm = 0;
    config.display_las_index = 0;
    config.warmup_requests = config.qps; // default
    config.status = Config::WARMUP;

    sprintf(file_name, "data/%s.lat", config.type.c_str());
    config.fd_lat = fopen(file_name, "w+");
    
    //sprintf(file_name, "data/%s.qps", config.type.c_str());
    //config.fd_qps = fopen(file_name, "w+");
    
    sprintf(file_name, "data/%s_%lld_report.csv", config.type.c_str(), config.interval);
    config.fd_rep = fopen(file_name, "w+");
    
    config.sem_cnt = config.client_num;
    for(int i = 0; i < config.client_num; ++i) {
        pthread_create(&config.clients_pd[i], NULL, processRequests, &i);
    }
    pthread_create(&config.display_pd, NULL, show_report, NULL);

    //dist = ExpDist(config.qps * 1e-6, time(NULL), config.start_tm);
    dist = ExpDist(config.qps * 1e-9, time(NULL), nstime());
    for(int i = 0; i < config.client_num; ++i) {
        pthread_join(config.clients_pd[i], NULL);
    }
    pthread_join(config.display_pd, NULL);

    pthread_mutex_destroy(&config.lats_mutex);
    //show_total_report();
    
    fclose(config.fd_rep);
    //fclose(config.fd_qps);
    fclose(config.fd_lat);
}

bool parseOptions(int argc, const char **argv) {
    int i;
    for(i = 1; i < argc; ++i) {
        bool lastarg = (i == argc - 1);
        if(!strcmp(argv[i], "-c")) {
            if(lastarg) goto invalid;
            config.client_num = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "-h")) {
            if(lastarg) goto invalid;
            config.ip_addr = string(argv[++ i]);
        } else if(!strcmp(argv[i], "-p")) {
            if(lastarg) goto invalid;
            config.port = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "-t")) {
            if(lastarg) goto invalid;
            config.task.clear();
            for(char *tmp = strtok((char *)argv[++ i], ","); tmp != NULL; tmp = strtok(NULL, ",")) {
                config.task.push_back(tmp);
            }
        } else if(!strcmp(argv[i], "-n")) {
            if(lastarg) goto invalid;
            config.requests = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "-r")) {
            if(lastarg) goto invalid;
            config.total_round = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "--qps")) {
            if(lastarg) goto invalid;
            config.qps = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "--warmup")) {
            if(lastarg) goto invalid;
            config.warmup_requests = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "--interval")) {
            if(lastarg) goto invalid;
            config.interval = atoi(argv[++ i]);
        } else {
            goto usage;
        }
    }
    return true;
invalid:
    printf("Invalid option \"%s\" or option argument missing\n\n", argv[i]);

usage:
    printf(
        "Usage: redis-openloop-bench [-h <host>] [-p <port>] [-c <client>] [-n <requests>]  [-t <test-type>] [-a <auth>] [--qps <qps>] [--warmup <warmup>] [--interval <interval>] [-r <total-rounds>] \n\n"
        "Examples:\n\n"
        "redis-openloop-bench -h 127.0.0.1 -p 7000 -n 10000000 -c 50 -t set,get --qps 100000 --warmup 100000 --interval 1 -r 1200\n\n");
    return false;
}

int main(int argc, const char **argv) {
    config.client_num = 1;
    config.ip_addr = "127.0.0.1";
    config.port = 6379;
    config.requests = 100000;
    config.requests_finished = 0;
    config.task.push_back("SET");
    config.task.push_back("GET");
    config.interval = 1;
    config.qps = 100;
    config.round = 0;
    config.total_round = 10;
    if(parseOptions(argc, argv) == false) exit(-1);

    // 添加一个 redis 服务结点，可以多次调用此函数添加多个服务结点，
    // 因为 acl redis 模块支持 redis 服务结点的自动发现及动态添加
    // 功能，所以添加一个服务结点即可
    int max_conns = 200;
    string redis_url = config.ip_addr + ":" + to_string(config.port);
    cluster.set(redis_url.c_str(), max_conns);
    
    for(auto &type : config.task) {
        transform(type.begin(), type.end(), type.begin(), ::toupper);
        benchmark(type);
    }

    return 0;
}
