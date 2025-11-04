import sys
import matplotlib.pyplot as plt

times = []
for l in sys.stdin:
    triplet = l.split(",")
    t = int(triplet[1])
    times.append(t)

if not times:
    print("Task 0 not implemented it seems...")
    exit(1) 

nr_bins = (max(times) - min(times) + 50) // 2
plt.hist(times, bins=nr_bins)
plt.savefig("hist.png")
print("Histogram saved as hist.png.")

#-----------------------------------------------------------------
# /*  TASK 0_Lab2:


