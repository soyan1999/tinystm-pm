#!/usr/bin/env python
import sys
import random

def genLadder(nbVertices):
    resX = []
    resY = []
    kX = 1
    kX_update = 2
    kY = -1
    kY_update = 2
    for i in range(nbVertices):
        kX += kX_update
        if (kX >= nbVertices):
            kX_update = -2
            kX = nbVertices
        if (kX <= 0):
            kX = 1
        resX += [kX]
        kY += kY_update
        if (kY >= nbVertices):
            kY_update = -2
            kY = nbVertices
        resY += [kY]
    return [(x,y,1) for x, y in zip(resX, resY)]

def printLadder(filename, nbVertices):
    f = open(filename, "w")
    ladder = genLadder(nbVertices)
    lines = ["%4i    %4i  %4i    %4i\n"%(i+1, ladder[i][0], ladder[i][1], ladder[i][2]) for i in range(len(ladder))]
    f.writelines(["%i  2  0  0\n"%nbVertices])
    for i in range(nbVertices):
        f.writelines(["%i  %10.6f  %10.6f\n"%(i, random.random()*20.0 - 10.0, random.random()*20.0 - 10.0)])
    f.writelines(["%i  1\n"%nbVertices])
    f.writelines(lines)
    f.writelines("4\n 1  1.5  1.5\n 2  -1.5  1.5\n 3  1.5  -1.5\n 4  -1.5  -1.5\n")
    f.close()

if __name__ == "__main__":
   printLadder(sys.argv[1], int(sys.argv[2]))

