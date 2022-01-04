#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.11

fig = plt.figure(figsize=(7,3))

ax = plt.subplot(111)
patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

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
                x.append(int(avg[0]))
                htm_commits = np.append(htm_commits, float(avg[2]))
                htm_commits_err = np.append(htm_commits_err, float(stdev[2]))
                sgl_commits = np.append(sgl_commits, float(avg[3]))
                sgl_commits_err = np.append(sgl_commits_err, float(stdev[3]))
                aborts = np.append(aborts, float(avg[4]))
                aborts_err = np.append(aborts_err, float(stdev[4]))

            all_cases = htm_commits + sgl_commits + aborts
            htm_commits = (htm_commits + sgl_commits) / all_cases
            htm_commits_err = (htm_commits_err + sgl_commits_err) / all_cases
            #sgl_commits = sgl_commits / all_cases
            #sgl_commits_err = sgl_commits_err / all_cases
            aborts = aborts / all_cases
            aborts_err = aborts_err / all_cases
            ind = np.arange(len(x))
            ind = np.array(ind) + 0.065 * i

            ax.bar(ind, htm_commits, width, bottom=aborts, yerr=htm_commits_err, label="Commit (" + titles[int(i)] + ")")
            #ax.bar(ind, sgl_commits, width, bottom=aborts, yerr=htm_commits_err, label="SGL (" + titles[int(i)] + ")")
            ax.bar(ind, aborts, width, yerr=aborts_err, label="Abort (" + titles[int(i)] + ")",
                edgecolor='black', color='white', hatch=patterns[int(int(i)/2)])

            # plt.gca().set_prop_cycle(None)

plt.xticks(ind, x)
plt.xlabel('Threads')
plt.ylabel('normalized number of \ncommits and aborts')

plt.margins(0.01, 0.01)
box = ax.get_position()
ax.set_position([box.x0, box.y0 + box.height * 0.2,
                 box.width * 1.1, box.height * 0.8])
ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
          fancybox=True, shadow=True, ncol=5, prop={'size': 5})

plt.title(title)
plt.savefig("plot_aborts_" + title + ".png", dpi=150)

