#!/usr/bin/python3
from sys import argv


def main():
  fileName = argv[1]
  samples = int(argv[2])
  threads = int(argv[3])
  benchs = int(argv[4])
  key = argv[5]
  with open(fileName, 'r') as f:
    lns = f.readlines()
    ln_count = 0
    for bench in range(benchs):
      for thread in range(threads):
        sample = 0
        sum = 0
        while sample < samples:
          if ln_count >= len(lns):
            print('\ntxt end!')
            exit()
          if lns[ln_count].find('Assertion') != -1 or lns[ln_count].find('segmentation') != -1:
            print('\nerror exec!')
            exit()
          if lns[ln_count].find(key) != -1:
            var = ''
            if key == 'TIME':
              var = lns[ln_count+1].strip().split()[1]
            elif key == 'ABORTS':
              var = lns[ln_count+1].strip().split()[-1]
            else:
              var = lns[ln_count].strip().split()[-1]
            
            try:
              sum += float(var)
              sample += 1
            except:
              pass
          ln_count += 1
        print(sum/samples, end=' ')
      print('')

if __name__ == "__main__":
  main()