#!/usr/bin/python3
from sys import argv


def main():
  fileName = argv[1]
  with open(fileName, 'r') as f:
    lns = f.readlines()
    cnt = 0
    sm = 0
    for ln in lns:
      if ln !=  '':
        cnt += 1
        sm += int(ln)
    
    print(sm/cnt)

if __name__ == "__main__":
  main()