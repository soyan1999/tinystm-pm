#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

nbBenches = int(sys.argv[1])
curArgc = 2
benches = []

for i in range(nbBenches):
    benches += [{}]
    print("curArgc:", curArgc, "sys.argv[curArgc]:", sys.argv[curArgc])
    benches[i]["nbSols"] = int(sys.argv[curArgc])
    curArgc += 1
    nbFiles = benches[i]["nbSols"] * 2
    benches[i]["files"] = sys.argv[curArgc:(curArgc+nbFiles)]
    benches[i]["titles"] = [t[-2] for t in [f.split('_') for f in benches[i]["files"]]]
    # print("  >>> files:", benches[i]["files"])
    benches[i]["name"] = [t[0] for t in [f.split('_use') for f in benches[i]["files"]]][0]
    print("  >>> name:", benches[i]["name"])
    curArgc += nbFiles

# print(benches)

# fig = plt.figure(figsize=(6,7))
width = 3
height = 3
fig, axs = plt.subplots(nrows=height, ncols=width, figsize=(8,7.5), sharex=False, gridspec_kw={'hspace': 0.1})

patterns = [
    (0, 0, 0, 0),
    (1, 2, 1, 2),
    (1, 2, 1, 2),
    (0, 0, 0, 0),
    (4, 2, 1, 2),
    (0, 0, 0, 0),
    (4, 2, 1, 2),
    (0, 0, 0, 0),
    (0, 0, 0, 0),
    (0, 0, 0, 0),
    (2, 2, 1, 2),
    (3, 2, 1, 2),
    (5, 2, 1, 2),
    (6, 2, 3, 2),
    (2, 2, 3, 2),
    (4, 2, 1, 2),
    (5, 2, 1, 2),
    (0, 0, 0, 0),
    (0, 0, 0, 0),
]

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

for b in range(nbBenches):
    files = benches[b]["files"]
    titles = benches[b]["titles"]
    name = benches[b]["name"]
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
                print(files[i])
                for avg, stdev in zip(avgs, stdevs):
                    x.append(int(avg[0]))
                    if "PSTM" in titles[i]:
                        y.append(float(avg[4]) / 1.0e3)
                        yerr.append(float(stdev[4]) / 1.0e3)
                    else:
                        y.append(float(avg[9]) / 1.0e3)
                        yerr.append(float(stdev[9]) / 1.0e3)
                title = ""
                for t in title_map:
                    if t in titles[i]:
                        title = title_map[t]
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
                # if "Logi" in titles[i] or "PSTM" in titles[i]:
                #     linewidth=1.2
                #yerr=yerr,
                axs.flat[b].plot([x2+(i-6)*0.08 for x2 in x ], y, alpha = alpha,
                    label=title, linewidth=linewidth, markersize=8, marker=marker, dashes=patterns[int(i/2)])
                axs.flat[b].set_title(name, pad=1.5)
                axs.flat[b].tick_params(axis = "y", labelsize = 10, rotation = 60, pad = -2)

    box = axs.flat[b].get_position()
    extraPadY = int(b / width) * 0.21
    extraPadX = int(b % width) * 0.21
    axs.flat[b].set_position([
        box.x0 - box.width * (0.215 - extraPadX),
        box.y0 + box.height * (0.22 - extraPadY),
        box.width * 1.22,
        box.height * 1.179
    ])
    plt.sca(axs.flat[b])
    plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
    plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)
    plt.gca().set_ylim(bottom=0)
    xticks = [1, 16, 32, 48, 64]
    xticks_l = ["", "", "", "", ""]
    plt.margins(0.02, 0.02)
    if b < 5:
        plt.xticks(xticks, xticks_l)
    else:
        plt.xticks(xticks, xticks)

b = 3
axs.flat[b].set_ylabel('Throughput (x1000 TXs/s)', size=14)
# axs.flat[b].legend(loc='right', bbox_to_anchor=(0.8, 0.1, 1.5, 0.9), fontsize=14)
b = 7
axs.flat[b].set_xlabel('Threads', size=14)

b = 7
axs.flat[b].legend(loc='right', bbox_to_anchor=(0.6, 0.03, 1.5, 0.8), fontsize=14)

# for b in range(width * height - nbBenches):
#     axs.flat[nbBenches - b + 1].axis('off')
#     axs.flat[nbBenches - b + 1].plot('off')
axs.flat[8].axis('off')
axs.flat[8].plot('off')

# xticks = [1, 16, 32, 48, 64]
# plt.xticks(xticks, xticks)

# axs.flat[6].set_xticks(xticks, xticks)


plt.savefig("plot_throughput_multi.pdf", dpi=150)

