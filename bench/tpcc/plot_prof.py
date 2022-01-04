#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.12

fig = plt.figure(figsize=(7,3))

ax = plt.subplot(111)
patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

for i in range(0, len(files), 2):
    x = []
    total_time = np.array([])
    after_tx_time = np.array([])
    wait_time = np.array([])
    total_time_err = np.array([])
    after_tx_time_err = np.array([])
    wait_time_err = np.array([])
    scan_time = np.array([])
    scan_time_err = np.array([])
    ### TODO: it creates a hole in the plot
    # if titles[int(i)] == "useFastPCWC" or titles[int(i)] == "useLogicalClocks":
    #     continue
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            j = 0
            for avg, stdev in zip(avgs, stdevs):
                j = j + 1
                if j == 1 or j > 11:
                    continue
                x.append(int(avg[0]))
                total_time = np.append(total_time, float(avg[2]))
                total_time_err = np.append(total_time_err, float(stdev[2]))
                after_tx_time = np.append(after_tx_time, float(avg[3]))
                after_tx_time_err = np.append(after_tx_time_err, float(stdev[3]))
                wait_time = np.append(wait_time, float(avg[4]))
                wait_time_err = np.append(wait_time_err, float(stdev[4]))
                if titles[int(i)] == "usePCWC-F" or titles[int(i)] == "usePCWC-NF":
                    scan_time = np.append(scan_time, float(avg[5]))
                    scan_time_err = np.append(scan_time_err, float(stdev[5]))
            wait_time_f = wait_time / total_time
            wait_time_err_f = wait_time_err / total_time
            if titles[int(i)] == "usePCWC-F" or titles[int(i)] == "usePCWC-NF":
                after_tx_time_f = (after_tx_time - wait_time - scan_time) / total_time
                after_tx_time_err_f = (after_tx_time_err) / total_time
                scan_time_f = scan_time / total_time
                scan_time_err_f = scan_time_err / total_time
            else:
                after_tx_time_f = (after_tx_time - wait_time) / total_time
                after_tx_time_err_f = (after_tx_time_err) / total_time
            ind = np.arange(len(x))
            ind = np.array(ind) + 0.08 * i
            if titles[int(i)] == "usePCWC-F" or titles[int(i)] == "usePCWC-NF":
                ax.bar(ind, scan_time_f, width, bottom=after_tx_time_f+wait_time_f, yerr=scan_time_err_f, label="Scanning (" + titles[int(i)] + ")")
            ax.bar(ind, after_tx_time_f, width, bottom=wait_time_f, yerr=after_tx_time_err_f, label="After TX (" + titles[int(i)] + ")")
            ax.bar(ind, wait_time_f, width, yerr=wait_time_err_f, label="Waiting (" + titles[int(i)] + ")",
                edgecolor='black', color='white', hatch=patterns[int(int(i)/2)])

            # plt.gca().set_prop_cycle(None)

plt.xticks(ind, x)
plt.xlabel('Threads')
plt.ylabel('normalized time')

box = ax.get_position()
ax.set_position([box.x0, box.y0 + box.height * 0.2,
                 box.width * 1.1, box.height * 0.8])
ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
          fancybox=True, shadow=True, ncol=4, prop={'size': 5})

plt.title(title)
plt.savefig("plot_prof_" + title + ".png", dpi=150)

