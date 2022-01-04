#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            for avg, stdev in zip(avgs, stdevs):
                x.append(int(avg[0]))
                if "PSTM" in titles[i]:
                    y.append(float(avg[4]) / 1.0e3)
                    yerr.append(float(stdev[4]) / 1.0e3)
                else:
                    y.append(float(avg[9]) / 1.0e3)
                    yerr.append(float(stdev[9]) / 1.0e3)
            plt.errorbar(x, y, yerr=yerr, label=titles[i])
            plt.xlabel('Threads')
            plt.ylabel('Throughput (kTXs/s)')
            plt.legend()
plt.title(title)
plt.savefig("plot_throughput_" + title + ".png", dpi=150)

