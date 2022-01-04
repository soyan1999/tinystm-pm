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

bar_width = 0.12

patterns = ('///', '//', '/', 'O', 'OO', 'ooo', 'XX', 'xxx', '+++')
width = 1
height = 2
fig, axs = plt.subplots(nrows=height, ncols=width, figsize=(7,2.8), sharex=True, gridspec_kw={'hspace': 0.1})

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
    ax = axs.flat[b]
    # ax = axs
    for i in range(0, len(files), 2):
        x = []
        labels = []
        tx_time      = np.array([])
        tx_time_err  = np.array([])
        SGL_time     = np.array([])
        SGL_time_err = np.array([])
        com_time     = np.array([])
        com_time_err = np.array([])
        ntx_time     = np.array([])
        ntx_time_err = np.array([])
        tx_fail_time     = np.array([])
        tx_fail_time_err = np.array([])
        com_fail_time     = np.array([])
        com_fail_time_err = np.array([])
        #conflicts = []
        #capacity = []
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
                    x.append(int(avg[0]))
                    sum_time = float(avg[2])+float(avg[3])+float(avg[7])
                    # sum_time = 1
                    tx_time = np.append(tx_time, float(avg[2])/sum_time)
                    tx_time_err = np.append(tx_time_err, float(stdev[2])/sum_time)
                    com_time = np.append(com_time, float(avg[3])/sum_time)
                    com_time_err = np.append(com_time_err, float(stdev[3])/sum_time)
                    ntx_time = np.append(ntx_time, float(avg[4])/sum_time)
                    ntx_time_err = np.append(ntx_time_err, float(stdev[4])/sum_time)
                    SGL_time = np.append(SGL_time, min(float(avg[5]), float(avg[2]))/sum_time)
                    SGL_time_err = np.append(SGL_time_err, float(stdev[5])/sum_time)
                    tx_fail_time = np.append(tx_fail_time, float(avg[6])/sum_time)
                    tx_fail_time_err = np.append(tx_fail_time_err, float(stdev[6])/sum_time)
                    com_fail_time = np.append(com_fail_time, float(avg[7])/sum_time)
                    com_fail_time_err = np.append(com_fail_time_err, float(stdev[7])/sum_time)
                    # ntx_time = np.append(ntx_time, float(avg[7]))/sum_time)

                tx_time -= tx_fail_time
                tx_time -= SGL_time

                ind = np.arange(len(x))
                ind = np.array(ind) + 0.063 * i

                title = ""
                for t in title_map:
                    if t in titles[i]:
                        title = title_map[t]
                        break

                ind_label += [[title for i in ind]]
                ind_ticks += [ind]

                # print("SGL_time_err:", SGL_time_err)
                # print("ntx_time_err:", ntx_time)
                # print("com_time_err:", com_time)
                # print("")
                
                # color_str = "#%02x%02x%02x"%(50+i*13,50+i*13,50+i*13)
                # color_str = "#%02x%02x%02x"%(0x70,0x70,0x70)
                if (not showFirstAbort):
                    showFirstAbort = True
                    ax.bar(ind, SGL_time, bar_width, bottom=tx_time+com_fail_time+tx_fail_time+com_time, #yerr=tx_time_err,
                        label="SGL", color="#777777",  hatch="//", edgecolor='black')
                    ax.bar(ind, tx_time, bar_width, bottom=com_fail_time+tx_fail_time+com_time, #yerr=tx_time_err,
                        label="Proc. Txs that Succeed", color="#FFFFFF", edgecolor='black')
                    ax.bar(ind, com_time+com_fail_time, bar_width, bottom=tx_fail_time, #yerr=com_time_err,
                        label="Commit Phase", hatch="xx", color="#bbbbbb", edgecolor='black')
                    ax.bar(ind, tx_fail_time, bar_width, #yerr=SGL_time_err,
                        label="Proc. Txs that Fail", hatch="xx+", color="#000000", edgecolor='black')
                    # ax.bar(ind, com_fail_time, bar_width, #yerr=ntx_time_err,
                    #     label="Commit Fail", color='white', edgecolor='black')
                else:
                    ax.bar(ind, SGL_time, bar_width, bottom=tx_time+com_fail_time+tx_fail_time+com_time, #yerr=tx_time_err,
                        color="#777777",  hatch="//", edgecolor='black')
                    ax.bar(ind, tx_time, bar_width, bottom=com_fail_time+tx_fail_time+com_time, #yerr=tx_time_err,
                        color="#FFFFFF", edgecolor='black')
                    ax.bar(ind, com_time+com_fail_time, bar_width, bottom=tx_fail_time, #yerr=com_time_err,
                        hatch="xx", color="#bbbbbb", edgecolor='black')
                    ax.bar(ind, tx_fail_time, bar_width, #yerr=SGL_time_err,
                        hatch="xx+", color="#000000", edgecolor='black')
                    # ax.bar(ind, com_fail_time, bar_width, #yerr=ntx_time_err,
                    #     color='white', edgecolor='black')


                # axs.flat[b].set_xticklabels(title, rotation=45, horizontalalignment='left', verticalalignment='top', visible=True)

                
                ax.tick_params(axis = "y", labelsize = 7, rotation = 45, pad = -2)
                ax2 = ax.twinx()
                ax2.set_yticks([], [])
                ax2.set_ylabel(name, size=8)

                # plt.gca().set_prop_cycle(None)
    index += 1

    box = ax.get_position()
    extraPadY = int(b / width) * -0.27
    extraPadX = int(b % width) * 0.2
    ax.set_position([
        box.x0 - box.width * (0.075 - extraPadX),
        box.y0 + box.height * (0.229 - extraPadY),
        box.width * 1.17,
        box.height * 0.8
    ])
    plt.sca(ax)
    # plt.yscale("log")
    # plt.yticks([0.0, 0.2, 0.4, 0.6, 0.8, 1.0], ["", 0.2, 0.4, 0.6, 0.8, 1.0])
    ax.xaxis.grid(True, linestyle="--", linewidth=0.2)
    ax.yaxis.grid(True, linestyle="--", linewidth=0.5)

# plt.xticks(ind, x)

plt.text(0.085,-0.98, '1 thread')
plt.text(1.085,-0.98, '4 thread')
plt.text(2.085,-0.98, '16 threads')
plt.text(3.085,-0.98, '32 threads')
plt.text(4.085,-0.98, '64 threads')

b = 1
ax = axs.flat[b]
# for i in range(index):
plt.xticks(np.array(ind_ticks).flatten(), np.array(ind_label).flatten(), size = 9)
ax.tick_params(axis="x", labelrotation = 75, pad = -0.1, size=1, labelsize=11)

axs.flat[0].tick_params(axis="x", size=0)

# b = 1
b = 1
ax = axs.flat[b]
# ax = axs
ax.set_ylabel('Time (%)', size = 14)
ax.yaxis.set_label_coords(-0.04,0.9)

plt.sca(axs.flat[0])
plt.yticks([0.2, 0.4, 0.6, 0.8, 1.0], [20, 40, 60, 80, 100], size=8)

plt.sca(axs.flat[1])
plt.yticks([0.2, 0.4, 0.6, 0.8, 1.0], [20, 40, 60, 80, 100], size=8)

b = 0
ax = axs.flat[b]
# ax = axs
ax.legend(
    loc='center',
    bbox_to_anchor=(0.25, 0.41, 0.5, 1.6),
    fancybox=True,
    shadow=True,
    handletextpad=0.15,
    columnspacing=1.2,
    ncol=5, 
    fontsize=10
)
# ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
#           fancybox=True, shadow=True, ncol=5, prop={'size': 5})

plt.margins(0.01, 0.01)

plt.savefig("plot_prof_multi.pdf", dpi=150)

