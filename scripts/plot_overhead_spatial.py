"""
Per-track overhead spatial heatmaps (xy and zr).
Usage: python3 plot_overhead_spatial.py <histos_overhead.root> [output.png]
"""
import sys
import uproot
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm

RFILE = sys.argv[1] if len(sys.argv) > 1 else '/tmp/latest_histos_overhead.root'
OUTFILE = sys.argv[2] if len(sys.argv) > 2 else '/tmp/overhead_spatial.png'

with uproot.open(RFILE) as f:
    hxy = f['m_overhead_xy']
    xy_vals, xy_xe, xy_ye = hxy.to_numpy()
    hzr = f['m_overhead_zr']
    zr_vals, zr_xe, zr_ye = hzr.to_numpy()

# mm -> cm
xy_xe /= 10; xy_ye /= 10
zr_xe /= 10; zr_ye /= 10

# zoom zr: z in [-500, 500] cm, r in [0, 200] cm
zmask = (zr_xe >= -500) & (zr_xe <= 500)
rmask = (zr_ye >= 0)    & (zr_ye <= 200)
zi = np.where(zmask)[0]
ri = np.where(rmask)[0]
zr_zoom  = zr_vals[zi[0]:zi[-1], ri[0]:ri[-1]]
zr_xe_z  = zr_xe[zi[0]:zi[-1]+1]
zr_ye_z  = zr_ye[ri[0]:ri[-1]+1]

fig, axes = plt.subplots(1, 2, figsize=(15, 6))
fig.patch.set_facecolor('white')

def heatmap(ax, xe, ye, vals, xlabel, ylabel, title):
    vp = np.where(vals > 0, vals, np.nan)
    vmin = np.nanpercentile(vp, 20)
    vmax = np.nanpercentile(vp, 99.5)
    im = ax.pcolormesh(xe, ye, vp.T, norm=LogNorm(vmin=vmin, vmax=vmax), cmap='plasma')
    cb = plt.colorbar(im, ax=ax)
    cb.set_label('overhead [ns·tracks]', fontsize=10)
    ax.set_xlabel(xlabel, fontsize=11)
    ax.set_ylabel(ylabel, fontsize=11)
    ax.set_title(title, fontsize=12)
    ax.set_facecolor('#f0f0f0')
    for sp in ax.spines.values():
        sp.set_edgecolor('#999999')

heatmap(axes[0], xy_xe, xy_ye, xy_vals, 'x [cm]', 'y [cm]', 'Overhead map — xy')
axes[0].set_aspect('equal')
heatmap(axes[1], zr_xe_z, zr_ye_z, zr_zoom, 'z [cm]', 'r [cm]', 'Overhead map — zr (|z|<500cm, r<200cm)')

fig.suptitle(
    'Per-track overhead spatial map (track start position weighted by overhead)\n'
    f'{RFILE}',
    fontsize=10, y=1.01
)
plt.tight_layout()
plt.savefig(OUTFILE, dpi=150, bbox_inches='tight', facecolor='white')
print(f'saved {OUTFILE}')
