#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

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

bar_width = 0.11

patterns = ('///', '//', '/', 'O', 'OO', 'ooo', 'XX', 'xxx', '+++')
width = 1
height = 3
fig, axs = plt.subplots(nrows=height, ncols=width, figsize=(7,3.6), sharex=True, gridspec_kw={'hspace': 0.1})

    # "usePCWM2": "SPHT, link next",
    # "usePCWM3": "SPHT, link back",
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

showFirstAbort = False

ind_label = []
ind_ticks = []
index = 0

for b in range(nbBenches):
    files = benches[b]["files"]
    titles = benches[b]["titles"]
    name = benches[b]["name"]
    for i in range(0, len(files), 2):
        x = []
        labels = []
        all_cases = np.array([])
        htm_commits = np.array([])
        sgl_commits = np.array([])
        aborts = np.array([])
        #conflicts = []
        #capacity = []
        htm_commits_err = []
        sgl_commits_err = []
        aborts_err = []
        with open(files[i], 'r') as avgFile:
            with open(files[i+1], 'r') as stdevFile:
                avgs = csv.reader(avgFile, delimiter='\t')
                stdevs = csv.reader(stdevFile, delimiter='\t')
                next(avgs)
                next(stdevs)
                j = 0
                for avg, stdev in zip(avgs, stdevs):
                    j = j + 1
                    # if j == 7 or j == 9 or j == 10 or j == 12:
                    #     continue
                    if int(avg[0]) == 2 or int(avg[0]) == 4 or int(avg[0]) == 8 or int(avg[0]) == 12 or int(avg[0]) == 20 or int(avg[0]) == 24 or int(avg[0]) == 28 or int(avg[0]) == 33 or int(avg[0]) == 40 or int(avg[0]) == 48:
                        continue
                    x.append(int(avg[0]))
                    htm_commits = np.append(htm_commits, float(avg[2]))
                    htm_commits_err = np.append(htm_commits_err, float(stdev[2]))
                    if "PSTM" in titles[i]:
                        sgl_commits = np.append(sgl_commits, float(avg[2]) - float(avg[2]))
                        sgl_commits_err = np.append(sgl_commits_err, float(stdev[2]) - float(stdev[2]))
                        aborts = np.append(aborts, float(avg[3]))
                        aborts_err = np.append(aborts_err, float(stdev[3]))
                    else:
                        sgl_commits = np.append(sgl_commits, float(avg[3]))
                        sgl_commits_err = np.append(sgl_commits_err, float(stdev[3]))
                        aborts = np.append(aborts, float(avg[4]))
                        aborts_err = np.append(aborts_err, float(stdev[4]))


                all_cases = htm_commits + sgl_commits + aborts
                htm_commits = (htm_commits) / all_cases
                htm_commits_err = (htm_commits_err) / all_cases
                sgl_commits = sgl_commits / all_cases
                sgl_commits_err = sgl_commits_err / all_cases
                aborts = aborts / all_cases
                aborts_err = aborts_err / all_cases
                ind = np.arange(len(x))
                ind = np.array(ind) + 0.055 * i

                title = ""
                for t in title_map:
                    if t in titles[i]:
                        title = title_map[t]
                        break

                ind_label += [[title for i in ind]]
                ind_ticks += [ind]
                
                # color_str = "#%02x%02x%02x"%(50+i*13,50+i*13,50+i*13)
                color_str = "#%02x%02x%02x"%(0x70,0x70,0x70)
                if (not showFirstAbort):
                    showFirstAbort = True
                    ### This axs.flat[b].bar is what actually prints the bar
                    axs.flat[b].bar(ind, htm_commits, bar_width, bottom=aborts+sgl_commits, yerr=htm_commits_err,
                        label="HTM",  hatch="//", color=color_str, edgecolor='black')
                    axs.flat[b].bar(ind, sgl_commits, bar_width, bottom=aborts, yerr=sgl_commits_err, color="#cccccc", label="SGL")
                    axs.flat[b].bar(ind, aborts, bar_width, yerr=aborts_err, label="Aborts",
                        edgecolor='black', color='white',  hatch="XX")
                else:
                    print(color_str)
                    axs.flat[b].bar(ind, htm_commits, bar_width, bottom=aborts+sgl_commits, yerr=htm_commits_err,
                         hatch="//", color=color_str, edgecolor='black')
                    axs.flat[b].bar(ind, sgl_commits, bar_width, bottom=aborts, yerr=sgl_commits_err, color="#cccccc")
                    axs.flat[b].bar(ind, aborts, bar_width, yerr=aborts_err,
                        edgecolor='black', color='white',  hatch="XX")


                # axs.flat[b].set_xticklabels(title, rotation=45, horizontalalignment='left', verticalalignment='top', visible=True)

                
                axs.flat[b].tick_params(axis = "y", labelsize = 8.5, rotation = 30, pad = -2)
                ax2 = axs.flat[b].twinx()
                ax2.set_yticks([], [])
                ax2.set_ylabel(name, size=8.3)

                # plt.gca().set_prop_cycle(None)
    index += 1

    box = axs.flat[b].get_position()
    extraPadY = int(b / width) * -0.17
    extraPadX = int(b % width) * 0.2
    axs.flat[b].set_position([
        box.x0 - box.width * (0.06 - extraPadX),
        box.y0 + box.height * (0.22 - extraPadY),
        box.width * 1.16,
        box.height * 0.90
    ])
    plt.sca(axs.flat[b])
    plt.yticks([0.0, 0.2, 0.4, 0.6, 0.8, 1.0], ["", 0.2, 0.4, 0.6, 0.8, 1.0], size=8.5)
    axs.flat[b].xaxis.grid(True, linestyle="--", linewidth=0.2)
    axs.flat[b].yaxis.grid(True, linestyle="--", linewidth=0.5)

# plt.xticks(ind, x)
print("ind:", ind)
print("x:", x)
print("ind_label:", ind_label)
print("ind_ticks:", ind_ticks)
plt.text(0.15, -1.1, '1 thread')
plt.text(1.15, -1.1, '16 threads')
plt.text(2.15, -1.1, '32 threads')
plt.text(3.15, -1.1, '64 threads')

for i in range(index):
    plt.xticks(np.array(ind_ticks).flatten(), np.array(ind_label).flatten(), rotation = 75, size=0)

for b in range(3):
    axs.flat[b].tick_params(axis="x", size=1, labelsize=11)

axs.flat[2].tick_params(axis="x", labelrotation = 75, pad = -0.1, size=1, labelsize=11)

b = 2
# axs.flat[b].set_xlabel('Threads')

b = 1
axs.flat[b].set_ylabel('Prob. of diff. tx outcomes', size = 13)

b = 0
axs.flat[b].legend(
    loc='center',
    bbox_to_anchor=(0.25, 0.42, 0.5, 1.6),
    fancybox=True,
    shadow=True,
    handletextpad=0.15,
    columnspacing=0.8,
    ncol=5, 
    fontsize=11.5
)
# ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
#           fancybox=True, shadow=True, ncol=5, prop={'size': 5})



plt.margins(0.01, 0.01)

plt.savefig("plot_aborts_multi.pdf", dpi=150)

