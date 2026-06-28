import glob, csv
import numpy as np

configs = {'AODV':'AODV', 'MADQN':'MA-DQN', 'Node':'GNN-MADRL-FL (ASRR)'}
n=50; sim_time=200; interval=1.0

print(f"{'Protocol':<22} {'PDR (%)':<10} {'Avg Delay (ms)':<16} {'Avg Hops':<10}")
for key,label in configs.items():
    pdr_vals, delay_vals, hop_vals = [], [], []
    for fname in sorted(glob.glob(f'results-{key}{n}-*.csv')):
        with open(fname) as f:
            reader = csv.DictReader(f)
            rows = list(reader)
            # deduplicate: keep only first occurrence of each (Src,Seq)
            seen = set()
            unique_rows = []
            for r in rows:
                sig = (r['Src'], r['Seq'])
                if sig not in seen:
                    seen.add(sig)
                    unique_rows.append(r)
            delivered = len(unique_rows)
            total_gen = (sim_time / interval) * n
            pdr_vals.append(delivered / total_gen * 100.0)
            delays = [float(r['Delay(s)']) for r in unique_rows]
            hops   = [int(r['HopCount']) for r in unique_rows]
            if delays: delay_vals.append(np.mean(delays))
            if hops:   hop_vals.append(np.mean(hops))
    pdr   = np.mean(pdr_vals)   if pdr_vals   else 0
    delay = np.mean(delay_vals) if delay_vals else 0
    hops  = np.mean(hop_vals)   if hop_vals   else 0
    print(f"{label:<22} {pdr:<10.1f} {delay*1000:<16.1f} {hops:<10.1f}")