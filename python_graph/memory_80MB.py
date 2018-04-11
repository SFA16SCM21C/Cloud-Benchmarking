import numpy as np
import matplotlib.pyplot as plt

iopsCount = list()
thread = list()

with open("memory_80M.txt") as f:
    content = f.readlines()

content = [x.strip() for x in content] 
content = [x for x in content if x]

for val in content:
	tempList = val.split(',')
	iopsCount.append(tempList[1])
	thread.append(tempList[0])	

iocount = np.array(iopsCount)
time = np.array(thread)

plt.scatter(time, iocount)
plt.xlabel('Thread')
plt.ylabel('Throughput (80MB memory block)')
plt.show()

