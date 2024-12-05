import matplotlib.pyplot as plt
import numpy as np

# Data
benchmarks = [
    "LONG-SPEC2K6-00", "LONG-SPEC2K6-01", "LONG-SPEC2K6-02", "LONG-SPEC2K6-03", "LONG-SPEC2K6-04",
    "LONG-SPEC2K6-05", "LONG-SPEC2K6-06", "LONG-SPEC2K6-07", "LONG-SPEC2K6-08", "LONG-SPEC2K6-09",
    "SHORT-FP-1", "SHORT-FP-2", "SHORT-FP-3", "SHORT-INT-1", "SHORT-INT-2", "SHORT-INT-3",
    "SHORT-MM-1", "SHORT-MM-2", "SHORT-MM-3", "SHORT-SERV-1", "SHORT-SERV-2", "SHORT-SERV-3"
]

mpki_original = [
    5.339, 8.523, 5.728, 5.868, 10.895, 5.973, 4.180, 19.297, 1.794, 5.520,
    3.842, 1.123, 0.442, 8.056, 8.670, 14.358, 9.297, 11.017, 4.667, 5.900, 5.967, 8.393
]

mpki_tage = [
    2.193, 8.183, 1.533, 2.560, 9.671, 5.701, 0.930, 9.163, 1.040, 4.667,
    1.635, 0.821, 0.435, 0.732, 5.312, 8.499, 8.592, 10.146, 0.290, 1.160, 1.121, 3.385
]

mpki_sc = [
    2.123, 7.594, 1.458, 2.491, 9.016, 5.300, 0.927, 9.085, 0.953, 4.576,
    1.620, 0.811, 0.435, 0.612, 5.303, 8.210, 7.810, 9.692, 0.157, 1.135, 1.106, 3.236
]

mpki_tage_loop = [
    2.187, 8.186, 1.534, 2.556, 9.674, 5.650, 0.809, 9.082, 0.981, 4.669,
    1.580, 0.817, 0.073, 0.731, 5.310, 8.469, 8.480, 10.153, 0.290, 1.157, 1.119, 3.384
]

mpki_cf_loop = [
    2.117, 7.598, 1.458, 2.486, 9.023, 5.251, 0.806, 9.005, 0.934, 4.577,
    1.565, 0.807, 0.073, 0.612, 5.301, 8.182, 7.706, 9.699, 0.157, 1.131, 1.104, 3.236
]



amean_values = [7.039, 3.990, 3.802, 3.950, 3.765]

# Adding the AMEAN bar
benchmarks_with_amean = benchmarks + ["AMEAN"]
mpki_original_with_amean = mpki_original + [amean_values[0]]
mpki_tage_with_amean = mpki_tage + [amean_values[1]]
mpki_sc_with_amean = mpki_sc + [amean_values[2]]
mpki_tage_loop_with_amean = mpki_tage_loop + [amean_values[3]]
mpki_cf_loop_with_amean = mpki_cf_loop + [amean_values[4]]

x_with_amean = np.arange(len(benchmarks_with_amean))

x = np.arange(len(benchmarks))
width = 0.15  # Width of each bar

fig, ax = plt.subplots(figsize=(15, 8))

# Plot bars
ax.bar(x_with_amean - 2*width, mpki_original_with_amean, width, label='Original')
ax.bar(x_with_amean - width, mpki_tage_with_amean, width, label='TAGE')
ax.bar(x_with_amean, mpki_sc_with_amean, width, label='SC')
ax.bar(x_with_amean + width, mpki_tage_loop_with_amean, width, label='TAGE + Loop')
ax.bar(x_with_amean + 2*width, mpki_cf_loop_with_amean, width, label='CF + Loop')

# Add numbers on bars with better spacing
for i, (orig, tage, sc, tage_loop, cf_loop) in enumerate(
    zip(mpki_original_with_amean, mpki_tage_with_amean, mpki_sc_with_amean, mpki_tage_loop_with_amean, mpki_cf_loop_with_amean)):
    if i < len(benchmarks):
        continue
    else:
        ax.text(i - 2*width, orig + 0.5, f'{orig:.2f}', ha='center', va='bottom', fontsize=8, rotation=45)
        ax.text(i - width, tage + 2.0, f'{tage:.2f}', ha='center', va='bottom', fontsize=8, rotation=45)
        ax.text(i, sc + 1.5, f'{sc:.2f}', ha='center', va='bottom', fontsize=8, rotation=45)
        ax.text(i + width, tage_loop + 1.0, f'{tage_loop:.2f}', ha='center', va='bottom', fontsize=8, rotation=45)
        ax.text(i + 2*width, cf_loop + 0.5, f'{cf_loop:.2f}', ha='center', va='bottom', fontsize=8, rotation=45)

# Formatting
ax.set_xlabel('Benchmarks')
ax.set_ylabel('MPKI')
ax.set_title('MPKI Comparison for Branch Predictor Configurations (Including AMEAN)')
ax.set_xticks(x_with_amean)
ax.set_xticklabels(benchmarks_with_amean, rotation=90)
ax.legend()
ax.grid(axis='y', linestyle='--', alpha=0.7)

# Show plot
plt.tight_layout()
plt.savefig('mpki_comparison.svg')
plt.show()