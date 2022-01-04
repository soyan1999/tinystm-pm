#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]

fig, axs = plt.subplots(nrows=1, ncols=1, figsize=(7,3.2), sharex=False, gridspec_kw={'hspace': 0.1})

patterns = [
    [1, 0],
    [1, 2],
    [1, 2],
    [1, 0],
    [4, 2],
    [1, 0],
    [4, 2],
    [1, 0],
    [1, 0],
    [1, 0],
    [2, 2],
    [3, 2],
    [5, 2],
    [6, 2],
    [2, 2],
    [4, 2],
    [5, 2],
    [1, 0],
    [1, 0],
]

# patterns = [
#     [0, 0],
#     [1, 2],
#     [1, 2],
#     [0, 0],
#     [4, 2],
#     [0, 0],
#     [4, 2],
#     [0, 0],
#     [0, 0],
#     [0, 0],
#     [2, 2],
#     [3, 2],
#     [5, 2],
#     [6, 2],
#     [2, 2],
#     [4, 2],
#     [5, 2],
#     [0, 0],
#     [0, 0],
# ]

title_map = {
    "useCcHTM": "cc-HTM",
    "usePSTM": "PSTM",
    "useCraftyImmDur": "Crafty (Dur)",
    "useCrafty": "Crafty",
    "useLogicalClocks": "DudeTM",
    "usePhysicalClocks": "NV-HTM",
    "usePCWM2": "SPHT-FL",
    "usePCWM3": "SPHT-BL",
    "usePCWM": "SPHT-NL"
}

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    linewidth = 0.9
    marker="."
    alpha=0.7
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            for avg, stdev in zip(avgs, stdevs):
                x.append(int(avg[0]) +(i-len(files)/2)*0.05)
                y.append(((float(avg[2]) + float(avg[3])) / float(avg[1])) / 1.0e6)
                yerr.append(((float(stdev[2]) + float(stdev[3])) / float(avg[1])) / 1.0e6)

            titleAxis=""
            for t in title_map:
                if t in titles[i]:
                    titleAxis += title_map[t]
                    break

            if "PCWM" in titles[i]:
                linewidth=1.3
            if "PCWM2" in titles[i] or "PCWM3" in titles[i]:
                linewidth=1.5
            if "PCWM2" in titles[i]:
                marker="v"
            if "PCWM3" in titles[i]:
                marker="^"
            if "Log" in titles[i]:
                marker="+"
            if "Crafty" in titles[i]:
                marker="x"
            if "Phy" in titles[i]:
                marker="1"
            if "CcHTM" in titles[i]:
                marker="2"
            if "PSTM" in titles[i]:
                marker="p"

            # axs.plot([x2+(i-6)*0.08 for x2 in x ], y, alpha = alpha,
            #     label=title, linewidth=linewidth, markersize=8, marker=marker, dashes=patterns[int(i/2)])
            print("pattern="+str(patterns[int(i/2)]))
            axs.errorbar([x2+(i-6)*0.06 for x2 in x], y, label=titleAxis, yerr=yerr, linewidth=linewidth, 
                markersize=8, marker=marker, dashes=patterns[int(i/2)])
            # line.set_dashes(patterns[int(i/2)])
            
# axs.set_title("TPC-C", pad=1.5)
axs.tick_params(axis = "y", labelsize = 10, rotation = 60, pad = -2)

box = axs.get_position()
axs.set_position([
    box.x0 - box.width * (0.05),
    box.y0 + box.height * (0.05),
    box.width * 0.95,
    box.height * 1.1
])
plt.margins(0.02, 0.02)

plt.xlabel('Threads', size=14)
plt.ylabel('Throughput (MTXs/s)', size=14)
plt.legend(bbox_to_anchor=(1.0, 1.0), loc='upper left', ncol=1)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

xticks = [1, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 64]
plt.yticks(size=12)
plt.xticks(xticks, xticks, size=12)

# plt.title("TPC-C 32 warehouses 98% pay. 1% n.o. 1 del. TXs")
plt.savefig("plot_throughput_" + title + ".pdf")
# plt.show()

