#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;

struct Config {
    unsigned int id;
    unsigned int interval;
    bool quiet;
    char dir_name[50];
} config;

struct CPU_OCCUPY {       //定义一个cpu occupy的结构体
    char name[20];             //定义一个char类型的数组名name有20个元素
    unsigned int user;        //定义一个无符号的int类型的user
    unsigned int nice;        //定义一个无符号的int类型的nice
    unsigned int system;    //定义一个无符号的int类型的system
    unsigned int idle;         //定义一个无符号的int类型的idle
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
};

double cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n) {
    double od, nd;
    double id, sd;
    double cpu_use ;

    // od = (double) (o->user + o->nice + o->system + o->idle + o->softirq + o->iowait + o->irq); //第一次(用户+优先级+系统+空闲)的时间再赋给od
    // nd = (double) (n->user + n->nice + n->system + n->idle + n->softirq + n->iowait + n->irq); //第二次(用户+优先级+系统+空闲)的时间再赋给od
    od = (double) (o->user + o->nice + o->system + o->idle); //第一次(用户+优先级+系统+空闲)的时间再赋给od
    nd = (double) (n->user + n->nice + n->system + n->idle); //第二次(用户+优先级+系统+空闲)的时间再赋给od

    id = (double) (n->idle);    //用户第一次和第二次的时间之差再赋给id
    sd = (double) (o->idle) ;    //系统第一次和第二次的时间之差再赋给sd
    if((nd - od) != 0)
        cpu_use = 100.0 - ((id - sd)) / (nd - od) * 100.00; //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used
    else cpu_use = 0;
    return cpu_use;
}

void get_cpuoccupy (CPU_OCCUPY *cpust) {
    FILE *fd;
    int n;
    char buff[256];
    CPU_OCCUPY *cpu_occupy;
    cpu_occupy = cpust;

    fd = fopen ("/proc/stat", "r");
    fgets (buff, sizeof(buff), fd);

    sscanf (buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice, &cpu_occupy->system, &cpu_occupy->idle , &cpu_occupy->iowait, &cpu_occupy->irq, &cpu_occupy->softirq);

    fclose(fd);
}

double getCpuRate() {
    CPU_OCCUPY cpu_stat1;
    CPU_OCCUPY cpu_stat2;
    double cpu;
    get_cpuoccupy((CPU_OCCUPY *)&cpu_stat1);
    sleep(config.interval);

    //第二次获取cpu使用情况
    get_cpuoccupy((CPU_OCCUPY *)&cpu_stat2);

    //计算cpu使用率
    cpu = cal_cpuoccupy ((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);

    return cpu / 100.0;
}

bool parseOptions(int argc, const char **argv) {
    int i;
    for(i = 1; i < argc; ++i) {
        bool lastarg = (i == argc - 1);
        if(!strcmp(argv[i], "--id")) {
            if(lastarg) goto invalid;
            config.id = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "--interval")) {
            if(lastarg) goto invalid;
            config.interval = atoi(argv[++ i]);
        } else if(!strcmp(argv[i], "--dir")) {
            if(lastarg) goto invalid;
            strcpy(config.dir_name, argv[++ i]);
        } else if(!strcmp(argv[i], "--queit")) {
            config.quiet = true;
        } else {
            goto usage;
        }
    }
    return true;
invalid:
    printf("Invalid option \"%s\" or option argument missing\n\n", argv[i]);

usage:
    printf(
        "Usage: ./cpu_monitor [--id <id>] [--interval <interval>] [--dir <dir_name>] [--quiet]\n\n"
        "Examples:\n\n"
        "./cpu_monitor --id 0 --interval 1\n\n");

    return false;
}


int main (int argc , const char **argv) {
    char path[50];
    config.interval = 1;
    config.id = 1;
    config.quiet = false;
    sprintf(config.dir_name, "/mnt/5/perinfo"); 
    if(false == parseOptions(argc, argv)) exit(-1);
    sprintf(path, "%s/cpu_usage_%d.csv", config.dir_name, config.id);
    FILE* fd = fopen(path, "a+");
    while(1) {
        double cpu_pec = getCpuRate();
        if(!config.quiet) {
            fprintf(stdout, "%.3f\n", cpu_pec); 
            fflush(stdout);
        }
        fprintf(fd, "%.3f\n", cpu_pec); 
        fflush(fd);
    }
    fclose(fd);
    return 0;
}
