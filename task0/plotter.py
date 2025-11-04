import sys
import matplotlib.pyplot as plt

hit_times = []
miss_times = []

for l in sys.stdin:
    triplet = l.split(",")
    label = triplet[0]
    t = int(triplet[1])
    
    if label == "hit":
        hit_times.append(t)
    elif label == "miss":
        miss_times.append(t)

if not hit_times and not miss_times:
    print("Task 0 not implemented it seems...")
    exit(1) 

# Combine all times to determine bin range
all_times = hit_times + miss_times
if all_times:
    nr_bins = (max(all_times) - min(all_times) + 50) // 2
    
    # Create histogram with both datasets
    plt.hist([hit_times, miss_times], bins=nr_bins, 
             color=['blue', 'orange'], 
             label=['Cache Hit', 'Cache Miss'],
             alpha=0.7)
    
    plt.legend()
    plt.xlabel('Time (cycles)')
    plt.ylabel('Frequency')
    plt.title('Cache Hit vs Miss Timing Distribution')
    
plt.savefig("hist.png")
print("Histogram saved as hist.png.")

#-----------------------------------------------------------------
# /*  TASK 0_Lab2:


