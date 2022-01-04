#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

curArgc = 1
benches = []
benches = {}
print("curArgc:", curArgc, "sys.argv[curArgc]:", sys.argv[curArgc])
benches["nbSols"] = int(sys.argv[curArgc])
curArgc += 1
nbFiles = benches["nbSols"] * 2
benches["files"] = sys.argv[curArgc:(curArgc+nbFiles)]
benches["titles"] = [t[-2] for t in [f.split('_') for f in benches["files"]]]
# print("  >>> files:", benches[i]["files"])
benches["name"] = [t[0] for t in [f.split('_use') for f in benches["files"]]][0]
print("  >>> name:", benches["name"])
curArgc += nbFiles


title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.11

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
bar_width = 0.11
patterns = ('///', '//', '/', 'O', 'OO', 'ooo', 'XX', 'xxx', '+++')
fig, axs = plt.subplots(nrows=1, ncols=1, figsize=(7,2.5), sharex=False, gridspec_kw={'hspace': 0.1})

titles = benches["titles"]
name = benches["name"]
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

            title = ""
            for t in title_map:
                if t in titles[i]:
                    title = title_map[t]
                    break

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
                axs.bar(ind, htm_commits, bar_width, bottom=aborts+sgl_commits, yerr=htm_commits_err,
                    label="HTM",  hatch="//", color=color_str, edgecolor='black')
                axs.bar(ind, sgl_commits, bar_width, bottom=aborts, yerr=sgl_commits_err, color="#cccccc", label="SGL")
                axs.bar(ind, aborts, bar_width, yerr=aborts_err, label="Aborts",
                    edgecolor='black', color='white',  hatch="XX")
            else:
                print(color_str)
                axs.bar(ind, htm_commits, bar_width, bottom=aborts+sgl_commits, yerr=htm_commits_err,
                        hatch="//", color=color_str, edgecolor='black')
                axs.bar(ind, sgl_commits, bar_width, bottom=aborts, yerr=sgl_commits_err, color="#cccccc")
                axs.bar(ind, aborts, bar_width, yerr=aborts_err,
                    edgecolor='black', color='white',  hatch="XX")

            # axs.bar(ind, htm_commits, width, bottom=aborts, yerr=htm_commits_err, label="Commit (" + titles[int(i)] + ")")
            # #ax.bar(ind, sgl_commits, width, bottom=aborts, yerr=htm_commits_err, label="SGL (" + titles[int(i)] + ")")
            # axs.bar(ind, aborts, width, yerr=aborts_err, label="Abort (" + titles[int(i)] + ")",
            #     edgecolor='black', color='white', hatch=patterns[int(int(i)/2)])

            # # plt.gca().set_prop_cycle(None)
            # ax2 = axs.twinx()
            # ax2.set_yticks([], [])
            # ax2.set_ylabel(name, size=8.3)

box = axs.get_position()
axs.set_position([
    box.x0 - box.width * (0.0),
    box.y0 + box.height * (0.34),
    box.width * 1.11,
    box.height * 0.65
])
plt.margins(0.01, 0.01)

plt.xticks(np.array(ind_ticks).flatten(), np.array(ind_label).flatten(), rotation = 75, size=1)
axs.tick_params(axis="x", labelrotation = 75, pad = -0.1, size=1, labelsize=13)
axs.tick_params(axis = "y", labelsize = 13, rotation = 30, pad = -2)

plt.yticks([0.0, 0.2, 0.4, 0.6, 0.8, 1.0], ["", 0.2, 0.4, 0.6, 0.8, 1.0])
axs.xaxis.grid(True, linestyle="--", linewidth=0.2)
axs.yaxis.grid(True, linestyle="--", linewidth=0.5)

# plt.xticks(ind, x)
print("ind:", ind)
print("x:", x)
print("ind_label:", ind_label)
print("ind_ticks:", ind_ticks)
plt.text(0.15, -0.83, '1 thread')
plt.text(1.15, -0.83, '16 threads')
plt.text(2.15, -0.83, '32 threads')
plt.text(3.15, -0.83, '64 threads')

axs.legend(
    loc='center',
    bbox_to_anchor=(0.25, 0.42, 0.5, 1.42),
    fancybox=True,
    shadow=True,
    handletextpad=0.15,
    columnspacing=0.8,
    ncol=5, 
    fontsize=11.5
)

# plt.xlabel('Threads')
axs.set_ylabel('Prob. of diff. \ntx outcomes', size = 13)

plt.savefig("plot_aborts_" + title + ".png")

