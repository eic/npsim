"""
Per-track overhead distribution overlay by particle species.
Usage: python3 plot_overhead_pdg.py <histos_overhead.root> [output.png]
"""
import sys
import uproot
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

RFILE = sys.argv[1] if len(sys.argv) > 1 else '/tmp/latest_histos_overhead.root'
OUTFILE = sys.argv[2] if len(sys.argv) > 2 else '/tmp/overhead_pdg.png'

species = [
    ('m_overhead_dist_pdg11',   'e⁻',    '#1f77b4'),
    ('m_overhead_dist_pdg22',   'γ',      '#ff7f0e'),
    ('m_overhead_dist_pdg-22',  'opt-γ',  '#d62728'),
    ('m_overhead_dist_pdg-11',  'e⁺',     '#9467bd'),
    ('m_overhead_dist_pdg2112', 'n',      '#2ca02c'),
    ('m_overhead_dist_pdg2212', 'p',      '#8c564b'),
    ('m_overhead_dist_pdg211',  'π⁺',     '#e377c2'),
    ('m_overhead_dist_pdg-211', 'π⁻',     '#bcbd22'),
]

XMIN, XMAX, SMOOTH = 2.0, 100.0, 7

fig, axes = plt.subplots(1, 2, figsize=(13, 5))
fig.patch.set_facecolor('white')

with uproot.open(RFILE) as f:
    for hname, label, color in species:
        if hname not in f:
            continue
        h = f[hname]
        vals, edges = h.to_numpy()
        centers = (edges[:-1] + edges[1:]) / 2
        mask = (centers >= XMIN) & (centers <= XMAX)
        x = centers[mask]
        y = vals[mask].astype(float)
        total = y.sum()
        if total == 0:
            continue
        y_norm = y / total
        y_smooth = np.convolve(y_norm, np.ones(SMOOTH) / SMOOTH, mode='same')
        n_tracks = int(vals.sum())
        mean_us = (centers * vals).sum() / vals.sum() if vals.sum() > 0 else 0
        full_label = f'{label} ({n_tracks/1e6:.1f}M, mean={mean_us:.0f}µs)'
        for ax in axes:
            ax.plot(x, y_smooth, label=full_label, color=color, linewidth=1.8)

axes[0].set_xlim(XMIN, 40)
axes[0].set_ylim(bottom=0)
axes[0].set_xlabel('per-track overhead [µs]', fontsize=11)
axes[0].set_ylabel('fraction of tracks (normalized)', fontsize=10)
axes[0].set_title('Linear scale', fontsize=11)
axes[0].legend(fontsize=8, framealpha=0.95)
axes[0].grid(True, color='#dddddd', linewidth=0.5)

axes[1].set_yscale('log')
axes[1].set_xlim(XMIN, XMAX)
axes[1].set_ylim(1e-6, 0.2)
axes[1].set_xlabel('per-track overhead [µs]', fontsize=11)
axes[1].set_ylabel('fraction of tracks (normalized)', fontsize=10)
axes[1].set_title('Log scale', fontsize=11)
axes[1].legend(fontsize=8, framealpha=0.95)
axes[1].grid(True, color='#dddddd', linewidth=0.5, which='both')

for ax in axes:
    ax.set_facecolor('white')
    for sp in ax.spines.values():
        sp.set_edgecolor('#aaaaaa')

fig.suptitle(
    'Per-track overhead by species (normalized, smoothed, <2µs excluded)\n'
    f'{RFILE}',
    fontsize=10, y=1.01
)
plt.tight_layout()
plt.savefig(OUTFILE, dpi=150, bbox_inches='tight', facecolor='white')
print(f'saved {OUTFILE}')
