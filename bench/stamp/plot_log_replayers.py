#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-3] for t in [f.split('_') for f in files]]

map_title = {
    "PCWM2" : "PCWM, link next",
    "PCWM3" : "PCWM, link back", 
    "BUFFER-WBINVD" : "PCWM, sort next",
    "BACKWARD" : "PCWM, sort back"
}

map_colors = {
    "VACATION_usePCWM2"  : "red",
    "VACATION_usePCWM"  : "orange",
    "GENOME_usePCWM2"    : "blue",
    "GENOME_usePCWM"    : "green",
    "33554432"  : "orange",
    "536870912" : "blue",
    "L134217720": "green",
    "L67108860" : "orange",
    "L6710886"  : "purple",
    "PCWM3"     : "orange",
    "PCWM2"     : "blue",
    "BACKWARD"  : "pink"
}

fig = plt.figure(figsize=(6,3))
ax = plt.subplot(1, 1, 1)

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    # with open(files[i], 'r') as avgFile:
    #     with open(files[i+1], 'r') as stdevFile:
    with open("log_" + files[i], 'r') as avgLogFile:
        with open("log_" + files[i+1], 'r') as stdevLogFile:
            # avgs = csv.reader(avgFile, delimiter='\t')
            # stdevs = csv.reader(stdevFile, delimiter='\t')
            avgsLog = csv.reader(avgLogFile, delimiter='\t')
            stdevsLog = csv.reader(stdevLogFile, delimiter='\t')
            print(files[i] + ", " + files[i+1] + " (" + titles[i] + ")")
            # next(avgs)
            # next(stdevs)
            next(avgsLog)
            next(stdevsLog)

            titleAxis = ""

            if "PCWM-" in titles[i]:
                titleAxis += "Sort"
            else:
                titleAxis += "Link"

            if "VACATION" in files[i]:
                titleAxis += " (Vacation)"
            else:
                titleAxis += " (Genome)"
            # if "BACKWARD" in titles[i] or "usePCWM3" in titles[i]:
            #     titleAxis += "filter"
            # elif "usePCWM-BUFFER-WBINVD" in titles[i] or "usePCWM2" in titles[i]:
            #     titleAxis += "no-filter"

            # for avg, stdev, avgLog, stdevLog in zip(avgs, stdevs, avgsLog, stdevsLog):
            for avgLog, stdevLog in zip(avgsLog, stdevsLog):
                x.append(float(avgLog[1]) + 0.008 * (i - 3.0) * float(avgLog[1]))
                # y.append((float(avgLog[6]) / 2300000000.0))
                # yerr.append((float(stdevLog[6]) / 2300000000.0))
                # y.append(float(avgLog[4]) / (float(avgLog[6]) / 2300000000.0) / 1e6)
                # yerr.append(1.0 / (float(stdevLog[6]) / 2300000000.0)  / 1e6)
                y.append(float(avgLog[13]) / 1e6)
                yerr.append(float(stdevLog[13]) / 1e6)
            color = "red"
            for c in map_colors:
                if c in files[i]:
                    color = map_colors[c]
                    break
            if "PCWM2" in titles[i] or "PCWM3" in titles[i]:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, linestyle="--", color=color)
            else:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, color=color)

plt.margins(0.01, 0.01)
plt.xlabel('Number of Replayers', size=13)
plt.ylabel('Replay Throughput (1e6 wrts/s)', size=13)
plt.legend(prop={'size': 10})
plt.xscale("symlog", basex=2)

ax.tick_params(axis="x", pad = 2.5, size=1, labelsize=11)
ax.tick_params(axis="y", pad = 2.5, size=1, labelsize=11)


# plt.title("Parallel Replayer")
xticks = [1, 2, 4, 8, 16, 32, 64]
plt.xticks(xticks, xticks)
box = ax.get_position()
ax.set_position([
    box.x0 - box.width * 0.04,
    box.y0 + box.height * 0.055,
    box.width * 1.15,
    box.height * 1.07
])
ax.yaxis.set_label_coords(-0.068,0.44)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

plt.savefig("plot_log_" + title + ".pdf", dpi=150)

