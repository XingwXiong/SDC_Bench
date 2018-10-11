#!/usr/bin/python

from __future__ import print_function

import sys
import os
import numpy as np
import pandas as pd
from scipy import stats

if __name__ == '__main__':
    def getLatPct(latsFile,user_p_num):
        assert os.path.exists(latsFile)
        f = open(latsFile, 'r')
        user_p_num = float(user_num)
        #lats = np.fromfile(f, dtype=np.uint64)
        lats = [int(l.strip()) for l in f.readlines()]
        #p95 = stat.scoreatpercentile(sjrnTimes, 95)
        p_num = stats.scoreatpercentile(lats, user_p_num)
        maxLat = max(lats)
        minLat = min(lats)
        print("user_p_num th latency %.3f ms | max latency %.3f ms" \
                % (p_num, maxLat))
        print("MEAN\t", end="")
        p_table = (50, 80, 90, 95, 98, 99, 99.5, 99.9)
        for p in p_table:
            print(("%5.2f\t" % p), end="")
        print("")
        print(("%5.2f\t" % np.mean(lats)), end="")
        for p in p_table:
            p_val = stats.scoreatpercentile(lats, p)
            print(("%5.2f\t" % p_val), end="")
        print("")

    def getArchMean(archFile):
        assert os.path.exists(archFile)
        df = pd.read_table(archFile, sep=' ')
        l1i_miss    =(1000*df['L1I.MISS']/df['ins']).mean()
        l2_miss     =(1000*df['L2_RQST.MISS']/df['ins']).mean()
        l3_miss     =(1000*df['L3_miss']/df['ins']).mean()
        dtlb_miss   =(1000*df['DTLB_MISS']/df['ins']).mean()
        itlb_miss   =(1000*df['ITLB_MISS']/df['ins']).mean()
        ipc         =(df['ins']/df['cycle']).mean()
        print(("%10s\t%10s\t%10s\t%10s\t%10s\t%10s") % ("l1i_miss","l2_miss", "l3_miss","dtlb_miss","itlb_miss","ipc"))
        print("%10.2f\t%10.2f\t%10.2f\t%10.2f\t%10.2f\t%10.2f" % (l1i_miss,l2_miss,l3_miss,dtlb_miss,itlb_miss,ipc))

    latsDir  = sys.argv[1]
    user_num = sys.argv[2]
    #getLatPct(('%s/SET.lat' % latsDir),user_num)
    getArchMean(('%s/sys-arch.log' % latsDir))
       
