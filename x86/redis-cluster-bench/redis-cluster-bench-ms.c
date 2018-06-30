#include <sys/time.h>
#include <bits/stdc++.h>
#include <acl_cpp/lib_acl.hpp>
#include <pthread.h>

using namespace std;

struct Config {
    int client_num;
    vector<pthread_t> clients;
    vector<pair<string, int>> urls;
    string ip_addr;
    int port;
    string auth;
    int requests_finished;
    int requests;

    vector<string> task;
    string type;
    long long start_tm;
    long long total_tm;
    vector<long long> lats;

    pthread_mutex_t lats_mutex;

} config;

acl::redis_client_cluster cluster; // 定义 redis 客户端集群管理对象
acl::redis_string cmd_string; // redis 字符串类 (STRING) 操作对象

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

acl::string gen_key() {
    const int size = 3;
    acl::string ret = "__test_key__";
    for(int i = 0; i < size; ++i) {
        ret += (char)(rand() % 10 + '0');
    }
    return ret;
}

acl::string gen_val() {
    const int size = 20;
    acl::string ret = "__test_val__";
    for(int i = 0; i < size; ++i) {
        ret += (char)(rand() % 10 + '0');
    }
    return ret;
}

static bool test_set(acl::redis_string &cmd) {
    acl::string key = gen_key();
    acl::string val = gen_val();
    assert(cmd.set(key, val) == true);
    key.clear();
    val.clear();
    return true;
}

static bool test_get(acl::redis_string &cmd) {
    acl::string key = gen_key();
    acl::string val = gen_val();
    assert(cmd.get(key, val) == true);
    key.clear();
    val.clear();
    return true;
}

static void *processRequests(void *e) {
    bool (* func)(acl::redis_string & cmd) = NULL;
    if(config.type == "SET") {
        func = &test_set;
    } else if(config.type == "GET") {
        func = &test_get;
    } else {
        fprintf(stderr, "type error: no such type %s\n", config.type.c_str());
        exit(-1);
    }
    pthread_mutex_lock(&config.lats_mutex);
    while(config.requests_finished < config.requests) {
        long long lat_bg = mstime();
        func(cmd_string);
        ++ config.requests_finished;
        config.lats.push_back(mstime() - lat_bg);
    }
    pthread_mutex_unlock(&config.lats_mutex);
}

void show_report() {
    printf("==============%s==============\n", config.type.c_str());
    char file_name[50];
    sprintf(file_name, "%s.lat", config.type.c_str());
    printf("Latency File:\t %s\n", file_name);
    FILE *lat_fd = fopen(file_name, "w+");
    sprintf(file_name, "%s.qps", config.type.c_str());
    printf("QPS File:\t %s\n", file_name);
    FILE *qps_fd = fopen(file_name, "w+");

    long double mean_lat = 0;
    for(auto &lat : config.lats) {
        fprintf(lat_fd, "%lld\n", (long long)lat);
        mean_lat += lat;
    }
    mean_lat = config.requests_finished ? mean_lat / config.requests_finished : 0;
    long double qps = config.total_tm ? (long double)config.requests_finished / (config.total_tm / 1000.0) : 0;
    fprintf(qps_fd, "%.2Lf\n", qps);
    fclose(lat_fd);
    fclose(qps_fd);
    sort(config.lats.begin(), config.lats.end());
    double pers[] = {0, 50, 80, 95, 99, 99.9, 100};
    int pers_size = 7; //sizeof(pers) / sizeof(double);
    printf("%8s%8s", "QPS", "MEAN");
    for(int i = 0; i < pers_size; ++ i) {
        printf("%7.2f%%", pers[i]);
    }
    printf(" (unit: ms)\n");
    printf("%8.2Lf%8.2Lf", qps, mean_lat);
    for (int i = 0; i < pers_size; ++ i) {
        int pos = pers[i] / 100 * max(0, config.requests_finished - 1);
        printf("%8lld", config.lats[pos]);
    }
    printf("\n");
    fflush(stdout);
}

void benchmark(const string &type) {
    config.type = type;
    config.requests_finished = 0;
    config.lats.clear();
    pthread_mutex_init(&config.lats_mutex, NULL);
    config.clients.clear();
    config.clients.resize(config.client_num);
    for(int i = 0; i < config.client_num; ++i) {
        pthread_create(&config.clients[i], NULL, processRequests, NULL);
    }

    config.start_tm = mstime();

    for(int i = 0; i < config.client_num; ++i) {
        pthread_join(config.clients[i], NULL);
    }

    config.total_tm = mstime() - config.start_tm;

    pthread_mutex_destroy(&config.lats_mutex);
    show_report();
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
        } else {
            goto usage;
        }
    }
    return true;
invalid:
    printf("Invalid option \"%s\" or option argument missing\n\n", argv[i]);

usage:
    printf(
        "Usage: redis-cluster-bench [-h <host>] [-p <port>] [-c <client>] [-n <requests>]  [-t <test-type>] [-a <auth>]\n\n"
        "Examples:\n\n"
        "redis-cluster-bench -h 127.0.0.1 -p 7000 -n 10000000 -c 20 -t set,get\n\n");
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

    if(parseOptions(argc, argv) == false) exit(-1);

    // 添加一个 redis 服务结点，可以多次调用此函数添加多个服务结点，
    // 因为 acl redis 模块支持 redis 服务结点的自动发现及动态添加
    // 功能，所以添加一个服务结点即可
    int max_conns = 100;
    string redis_url = config.ip_addr + ":" + to_string(config.port);
    cluster.set(redis_url.c_str(), max_conns);
    cmd_string.set_cluster(&cluster, max_conns);

    for(auto &type : config.task) {
        transform(type.begin(), type.end(), type.begin(), ::toupper);
        benchmark(type);
    }

    return 0;
}
