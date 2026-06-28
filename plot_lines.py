import glob, csv
import numpy as np
import matplotlib.pyplot as plt

configs = {'AODV':'AODV', 'MADQN':'MA-DQN', 'Node':'GNN-MADRL-FL'}
n = 50
colors = {'AODV':'#d62728', 'MADQN':'#ff7f0e', 'Node':'#2ca02c'}

# store mean and std for each protocol
stats = {label: {'pdr':[], 'delay':[], 'hops':[]} for label in configs.values()}

for key, label in configs.items():
    for fname in sorted(glob.glob(f'results-{key}{n}-*.csv')):
        with open(fname) as f:
            reader = csv.DictReader(f)
            rows = list(reader)
            # deduplicate
            seen = set()
            unique = []
            for r in rows:
                sig = (r['Src'], r['Seq'])
                if sig not in seen:
                    seen.add(sig)
                    unique.append(r)
            delivered = len(unique)
            # true generated from max seq per source
            max_seq = {}
            for r in unique:
                src = r['Src']
                seq = int(r['Seq'])
                max_seq[src] = max(max_seq.get(src,0), seq)
            total_gen = sum([s+1 for s in max_seq.values()])
            pdr = delivered / total_gen * 100.0 if total_gen > 0 else 0
            delays = [float(r['Delay(s)']) for r in unique]
            hops   = [int(r['HopCount']) for r in unique]
            stats[label]['pdr'].append(pdr)
            stats[label]['delay'].append(np.mean(delays)*1000 if delays else 0)  # ms
            stats[label]['hops'].append(np.mean(hops) if hops else 0)

# Prepare data for plotting
labels = [configs[k] for k in ['AODV','MADQN','Node']]
pdr_means = [np.mean(stats[l]['pdr']) for l in labels]
pdr_stds  = [np.std(stats[l]['pdr']) for l in labels]
delay_means = [np.mean(stats[l]['delay']) for l in labels]
delay_stds  = [np.std(stats[l]['delay']) for l in labels]
hops_means  = [np.mean(stats[l]['hops']) for l in labels]
hops_stds   = [np.std(stats[l]['hops']) for l in labels]

x = np.arange(len(labels))

fig, axes = plt.subplots(1, 3, figsize=(12, 4))

# PDR
axes[0].errorbar(x, pdr_means, yerr=pdr_stds, marker='o', linestyle='-', capsize=5,
                 color='#2ca02c')
axes[0].set_xticks(x)
axes[0].set_xticklabels(labels, rotation=15)
axes[0].set_ylabel('PDR (%)')
axes[0].set_title('Packet Delivery Ratio')

# Delay
axes[1].errorbar(x, delay_means, yerr=delay_stds, marker='s', linestyle='-', capsize=5,
                 color='#d62728')
axes[1].set_xticks(x)
axes[1].set_xticklabels(labels, rotation=15)
axes[1].set_ylabel('Avg Delay (ms)')
axes[1].set_title('End‑to‑End Delay')

# Hops
axes[2].errorbar(x, hops_means, yerr=hops_stds, marker='^', linestyle='-', capsize=5,
                 color='#1f77b4')
axes[2].set_xticks(x)
axes[2].set_xticklabels(labels, rotation=15)
axes[2].set_ylabel('Avg Hops')
axes[2].set_title('Average Hop Count')

plt.tight_layout()
plt.savefig('comparison_lines.png')
print("Chart saved to comparison_lines.png")
