#!/usr/bin/env python
import os, sys
import re
import time
import subprocess
import psutil

path=sys.argv[1]
interval=int(sys.argv[2])
total_round=int(sys.argv[3]);
#perf_event=('branch-instructions', 'branch-misses', 'cache-misses', 'cache-references', 'cpu-cycles', 'instructions')
#perf_event=('uncore/event=0x2c,umask=0x07/','uncore/event=0x2f,umask=0x07/')
perf_event=('uncore/event=0x2c,umask=0x07/','uncore/event=0x2f,umask=0x07/','branch-instructions', 'branch-misses', 'cache-misses', 'cache-references', 'cpu-cycles', 'instructions', 'raa24', 'r412e', 'r280')
'''
[ 2]    raa24                                   L2_RQST.MISS
[ 4]    r412e                                   L3_miss
[ 9]    r280                                    L1I.MISS
'''
perf_str='sudo perf stat -a -e %s sleep %d' % (','.join(perf_event), interval)
print perf_str
'''
            24,733      cache-misses              #    0.796 % of all cache refs      (44.37%)
         3,107,313      cache-references                                              (44.55%)
     1,118,408,978      cpu-cycles                                                    (44.78%)
       293,701,774      instructions              #    0.26  insn per cycle                                              (56.06%)
         3,991,559      raa24                                                         (56.20%)
            24,040      r412e                                                         (56.31%)
         2,446,619      r280                                                          (56.00%)
'''
perf_pats=(r'\s*?([\d,]+)\s+',              # first column
    r'\s*?([\d.]+) seconds time elapsed',   # total time
    r'cache-misses.*?\(([\d.]*?)%\)',       # cache-miss
    r'instructions.*?([\d.]*?)\s+?insn per cycle', # IPC
    r'raa24\s*?\(([\d.]*?)%\)',             # l2 misses
    r'r412e\s*?\(([\d.]*?)%\)',             # l3 misses
    r'r280\s*?\(([\d.]*?)%\)',              # l1I misses
)

'''
 Performance counter stats for 'system wide':

            24,527      uncore/event=0x2c,umask=0x07/                                   
             9,502      uncore/event=0x2f,umask=0x07/                                   

       1.001445395 seconds time elapsed
'''

def popen(cmd):
#    print cmd
    p = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return out + err

def get_cpu_info(bg_cpu, ed_cpu):
    bg_sum = bg_cpu.user + bg_cpu.nice + bg_cpu.system + bg_cpu.idle + bg_cpu.iowait + bg_cpu.irq + bg_cpu.softirq
    ed_sum = ed_cpu.user + ed_cpu.nice + ed_cpu.system + ed_cpu.idle + ed_cpu.iowait + ed_cpu.irq + ed_cpu.softirq
    dw = ed_sum - bg_sum
    up1 = dw - ((ed_cpu.idle + ed_cpu.iowait) - (bg_cpu.idle + bg_cpu.iowait))
    up2 = ed_cpu.iowait - bg_cpu.iowait
    return up1 / dw, up2 / dw

table_head=('cpu_utilization', 'iowait_utilization', 'mem_bandwidth', 'disk_band_width', 'net_recv', 'net_sent')

if __name__ == '__main__':
#    if os.path.exists(path): raise Exception('log file already exists', path)
    print sys.argv
    print "Example: ./xxw_perf.py sys.log 1 1200"
    f = open(path, 'w+')
    f_arch = open('sys-arch.log', 'w+')
    #f.write('\t'.join(table_head) + '\n')
    #print('\t'.join(table_head) + '\n')
    f.write("%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\n" % ('cpu(%)', 'iowait(%)', 'mem(MB/s)', \
    'disk_w(MB/s)', 'disk_r(MB/s)', 'disk_tot(MB/s)', \
    'net_r(Mbps)', 'net_s(Mbps)', 'net_tot(Mbps)'))
    f_arch.write("%-13s\t%-13s\t%-13s\t%-13s\t%-13s\n" % ('ipc', 'l1i_miss', 'l2_miss', 'l3_miss', 'cache_miss'))
    print("[  #]%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s\t%-13s" % ('cpu(%)', 'iowait(%)', 'mem(MB/s)', \
    'disk_w(MB/s)', 'disk_r(MB/s)', 'disk_tot(MB/s)', \
    'net_r(Mbps)', 'net_s(Mbps)', 'net_tot(Mbps)', 'ipc', 'l1i_miss', 'l2_miss', 'l3_miss', 'cache_miss'))
    for r in range(0, total_round):
        bg_tm = time.time()
        bg_cpu = psutil.cpu_times()
	bg_disk= psutil.disk_io_counters()
        bg_net = psutil.net_io_counters(True)['enp2s0f0']
        out = popen(perf_str)
        ed_net = psutil.net_io_counters(True)['enp2s0f0']
	ed_disk= psutil.disk_io_counters()
        ed_cpu = psutil.cpu_times()
	ed_tm = time.time()

        cpu_utilization, iowait_utilization = get_cpu_info(bg_cpu, ed_cpu)
        
	# parse result
        unc = [int(item.replace(',', '')) for item in re.findall(perf_pats[0], out)]
        #print(out)
        #print(unc)
        tim = float(re.search(perf_pats[1], out).group(1))
        cache_miss = float(re.search(perf_pats[2], out).group(1))
        ipc = float(re.search(perf_pats[3], out).group(1))
        l2_miss = float(re.search(perf_pats[4], out).group(1))
        l3_miss = float(re.search(perf_pats[5], out).group(1))
        l1i_miss = float(re.search(perf_pats[6], out).group(1))
        #print(cache_miss, ipc, l2_miss, l3_miss, l1i_miss)
        mem_bandwidth = float(unc[0] + unc[1]) * 64 / 1024 /1024 / tim
        disk_write = float(ed_disk.write_bytes - bg_disk.write_bytes) / 1024 / 1024 / (ed_tm - bg_tm)
        disk_read  = float(ed_disk.read_bytes - bg_disk.read_bytes) / 1024 / 1024 / (ed_tm - bg_tm)
        disk_total = disk_write + disk_read
        net_resv = float(ed_net.bytes_recv - bg_net.bytes_recv) * 8 / 1024 / 1024 / (ed_tm - bg_tm)
        net_sent = float(ed_net.bytes_sent - bg_net.bytes_sent) * 8 / 1024 / 1024 / (ed_tm - bg_tm)
        net_total= net_resv + net_sent

        f.write('%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\n' % (cpu_utilization * 100, iowait_utilization * 100, mem_bandwidth, \
        disk_write, disk_read, disk_total, net_resv, net_sent, net_total))
        f_arch.write('%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\n' % (ipc, l1i_miss, l2_miss, l3_miss, cache_miss))
        print('[%3d]%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f\t%-13.2f' % (r, cpu_utilization * 100, iowait_utilization * 100, mem_bandwidth, \
        disk_write, disk_read, disk_total, net_resv, net_sent, net_total, ipc, l1i_miss, l2_miss, l3_miss, cache_miss))
        #print('%13.2f\t%13.2f\t%13.2f\t%13.2f\t%13.2f\n' % (psutil.cpu_percent(), cpu_utilization, iowait_utilization, mem_bandwidth, disk_band_width))
        f.flush()
        f_arch.flush()
    f.close()
    f_arch.close()
