import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load the data
df = pd.read_csv('build/latency_data.csv')

# Reconstruct the raw samples for plotting
raw_data = np.repeat(df['latency_ns'], df['count'])

plt.figure(figsize=(12, 6))

# Plot 1: Histogram
plt.subplot(1, 2, 1)
plt.hist(raw_data, bins=100, color='skyblue', edgecolor='black')
plt.title('Latency Distribution (P50-P99)')
plt.xlabel('Latency (ns)')
plt.ylabel('Frequency')
plt.grid(axis='y', alpha=0.3)

# Plot 2: Boxplot (to see the tail)
plt.subplot(1, 2, 2)
plt.boxplot(raw_data, vert=True, patch_artist=True)
plt.title('Latency Outliers')
plt.ylabel('Nanoseconds')

plt.tight_layout()
plt.savefig('latency_profile.png')
print("Graph saved as latency_profile.png")
plt.show()