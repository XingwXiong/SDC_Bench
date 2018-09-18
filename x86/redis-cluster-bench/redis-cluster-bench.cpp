#include <sys/time.h>
#include <bits/stdc++.h>
#include <acl_cpp/lib_acl.hpp>
#include <pthread.h>

using namespace std;

struct Config {
    int client_num;
    vector<pthread_t> clients_pd;
    pthread_t display_pd;
    long long display_las_time;
    long long display_las_index;

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
    long long interval;
    vector<long long> lats;

    pthread_mutex_t lats_mutex;

} config;

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
    if (ret == false ) {
        const acl::redis_result *res = cmd.get_result();
        printf("set key: error: %s\r\n",
                res ? res->get_error() : "unknown error");
        return false;
    }
    //assert(ret == true);
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
    if (ret == false ) {
        const acl::redis_result *res = cmd.get_result();
        printf("get key: error: %s\r\n",
                res ? res->get_error() : "unknown error");
        return false;
    }
    key.clear();
    val.clear();
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
    pthread_mutex_lock(&config.lats_mutex);
    while(config.requests_finished < config.requests) {
        long long lat_bg = ustime();
        pthread_mutex_unlock(&config.lats_mutex);
        func(cmd_string);
        pthread_mutex_lock(&config.lats_mutex);
        ++ config.requests_finished;
        config.lats.push_back(ustime() - lat_bg);
    }
    pthread_mutex_unlock(&config.lats_mutex);
}

static void *show_report(void *) {
    static double pers[] = {0, 50, 80, 95, 99, 99.9, 100};
    static int pers_size = 7; //sizeof(pers) / sizeof(double);
    while(config.display_las_index < config.requests) {
        int requests_finished = config.requests_finished;
        long long cur_time = ustime();
        long long delta_tm = cur_time - config.display_las_time;
        long long delta_sz = requests_finished - config.display_las_index;
        if(delta_tm < (long long)config.interval * 1000000) continue;
        vector<long long> lat_buf;
        long double mean_lat = 0;
        for(int i = config.display_las_index; i < requests_finished; ++i) {
            lat_buf.push_back(config.lats[i]);
            mean_lat += config.lats[i];
        }
        mean_lat = delta_sz ? mean_lat / delta_sz : 0;
        long double qps = delta_tm ? (long double) delta_sz / (delta_tm / 1000000.0) : 0;
        sort(lat_buf.begin(), lat_buf.end());
        printf("==============%s==============\n", config.type.c_str());
        printf("%8s|%8s", "QPS", "MEAN");
        for(int i = 0; i < pers_size; ++ i) {
            printf("|%7.2f%%", pers[i]);
        }
        printf(" (unit: us)\n");
        printf("%8.2Lf|%8.2Lf", qps, mean_lat);
        for (int i = 0; i < pers_size; ++ i) {
            int pos = pers[i] / 100 * max(0ll, delta_sz - 1);
            printf("|%8lld", lat_buf[pos]);
        }
        char file_name[50];
        sprintf(file_name, "redis_cluster_latency_%s.csv", config.type.c_str());
        FILE *fp = fopen(file_name, "a");
        if(fp == NULL) {
            return 0;
        }
        fprintf(fp, "%8.2LF,%8Lf", qps, mean_lat);
        for(int i = 0; i < pers_size; ++ i) {
            int pos = pers[i] / 100 * max(0ll, delta_sz - 1);
            fprintf(fp, ",%8lld", lat_buf[pos]);
        }
        fprintf(fp, "\n");
        fclose(fp);

        printf("\n");
        config.display_las_time = ustime();
        config.display_las_index = requests_finished + 1;
    }
}

void show_total_report() {
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
    long double qps = config.total_tm ? (long double)config.requests_finished / (config.total_tm / 1000000.0) : 0;
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
    printf(" (unit: us)\n");
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
    config.clients_pd.clear();
    config.clients_pd.resize(config.client_num);
    for(int i = 0; i < config.client_num; ++i) {
        pthread_create(&config.clients_pd[i], NULL, processRequests, &i);
    }
    pthread_create(&config.display_pd, NULL, show_report, NULL);
    config.display_las_time = config.start_tm = ustime();
    config.display_las_index = 0;

    for(int i = 0; i < config.client_num; ++i) {
        pthread_join(config.clients_pd[i], NULL);
    }
    pthread_join(config.display_pd, NULL);
    config.total_tm = ustime() - config.start_tm;

    pthread_mutex_destroy(&config.lats_mutex);
    show_total_report();
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
        "Usage: redis-cluster-bench [-h <host>] [-p <port>] [-c <client>] [-n <requests>]  [-t <test-type>] [-a <auth>] [--interval <interval>]\n\n"
        "Examples:\n\n"
        "redis-cluster-bench -h 127.0.0.1 -p 7000 -n 10000000 -c 20 -t set,get --interval 1\n\n");
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
