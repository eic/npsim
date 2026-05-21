#!/usr/bin/env python3
"""
Analyze performance profile histograms to identify hot regions.
Extracts summed timing values for regions in R and Z from the ZR plot.
Outputs: annotated ZR plot, region×PDG heatmap, text summary.

Usage: plot_timing_profile.py <histos.root> [output_prefix]
"""

import sys
import ROOT
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def analyze_hot_regions(input_file, output_prefix=None):
    """
    Analyze the m_zr histogram to find hot regions and create annotated plots

    Args:
        input_file: Path to the histos.root file
        output_prefix: Prefix for output files (default: input filename without .root)
    """
    # Set ROOT to batch mode
    ROOT.gROOT.SetBatch(True)
    ROOT.gStyle.SetOptStat(0)
    ROOT.gStyle.SetPalette(ROOT.kRainBow)

    # Open the file
    f = ROOT.TFile.Open(input_file)
    if not f or f.IsZombie():
        print(f"Error: Cannot open file {input_file}")
        return 1

    # Get histograms
    m_zr = f.Get("m_zr")

    if not m_zr:
        print("Error: Could not find histogram m_zr in file")
        f.Close()
        return 1

    if output_prefix is None:
        output_prefix = input_file.replace(".root", "")

    print(f"Analyzing hot regions from: {input_file}")
    print(f"=" * 80)
    print()

    # Get histogram properties
    nbins_z = m_zr.GetNbinsX()
    nbins_r = m_zr.GetNbinsY()
    z_min = m_zr.GetXaxis().GetXmin()
    z_max = m_zr.GetXaxis().GetXmax()
    r_min = m_zr.GetYaxis().GetXmin()
    r_max = m_zr.GetYaxis().GetXmax()

    print(f"Histogram dimensions:")
    print(f"  Z bins: {nbins_z}, range: [{z_min/1000:.2f}, {z_max/1000:.2f}] m")
    print(f"  R bins: {nbins_r}, range: [{r_min/1000:.2f}, {r_max/1000:.2f}] m")
    print()

    # Extract all bin values
    total_time = 0
    bin_values = []

    for iz in range(1, nbins_z + 1):
        for ir in range(1, nbins_r + 1):
            value = m_zr.GetBinContent(iz, ir)
            if value > 0:
                z_center = m_zr.GetXaxis().GetBinCenter(iz)
                r_center = m_zr.GetYaxis().GetBinCenter(ir)
                bin_values.append({
                    'z': z_center,
                    'r': r_center,
                    'time': value
                })
                total_time += value

    print(f"Total time: {total_time:.2e} ns = {total_time/1e9:.3f} s")
    print(f"Non-zero bins: {len(bin_values)} / {nbins_z * nbins_r}")
    print()

    if len(bin_values) == 0:
        print("No data in histogram!")
        f.Close()
        return 1

    # Define detector regions based on actual geometry
    # Format: (name, z_min, z_max, r_min, r_max)
    detector_regions = [
        ("Forward EM+Hadron Cal", 3346.25, 4971.75, 0, 2626),
        ("dRICH", 1944.75, 3250,  0, 1800),
        ("DIRC Bar (straight)", -2734, 1900, 750, 782.50),
        ("DIRC Bar (triangle)", -3150, -2734, 728.75, 1007.50),
        ("EEEMCal", -1945, -1740, 0, 540),
        ("Barrel Imaging and SciFi Cal",-2700, 1900,  826.77, 1217.61),
        ("Barrel HCal", -3196.25, 3196.25, 1838.5, 2700.3),
        ("Tracker / Beampipe", -1740, 1945, 0, 728.75),
        ("Far Forward", 5500, 50000, 0, 1500),
    ]

    # Group by region
    region_stats = {}

    for name, z_lo, z_hi, r_lo, r_hi in detector_regions:
        region_stats[name] = {
            'z_range': (z_lo, z_hi),
            'r_range': (r_lo, r_hi),
            'time': 0,
            'n_bins': 0
        }

    # Accumulate times by region
    for bin_data in bin_values:
        z = bin_data['z']
        r = bin_data['r']
        t = bin_data['time']

        for name, z_lo, z_hi, r_lo, r_hi in detector_regions:
            if z_lo <= z < z_hi and r_lo <= r < r_hi:
                region_stats[name]['time'] += t
                region_stats[name]['n_bins'] += 1
                break

    # Combine DIRC Bar straight and triangle sections for reporting
    dirc_combined_time = 0
    dirc_combined_bins = 0
    dirc_regions = []

    if "DIRC Bar (straight)" in region_stats:
        straight = region_stats["DIRC Bar (straight)"]
        dirc_combined_time += straight['time']
        dirc_combined_bins += straight['n_bins']
        dirc_regions.append(("DIRC Bar (straight)", straight))

    if "DIRC Bar (triangle)" in region_stats:
        triangle = region_stats["DIRC Bar (triangle)"]
        dirc_combined_time += triangle['time']
        dirc_combined_bins += triangle['n_bins']
        dirc_regions.append(("DIRC Bar (triangle)", triangle))

    # Create combined entry for sorting/display
    combined_stats = {}
    for name, stats in region_stats.items():
        if name not in ["DIRC Bar (straight)", "DIRC Bar (triangle)"]:
            combined_stats[name] = stats

    # Add combined DIRC entry
    if dirc_regions:
        combined_stats["DIRC"] = {
            'time': dirc_combined_time,
            'n_bins': dirc_combined_bins,
            'sub_regions': dirc_regions  # Keep track of sub-regions for drawing boxes
        }

    # Sort regions by time
    sorted_regions = sorted(combined_stats.items(),
                           key=lambda x: x[1]['time'],
                           reverse=True)

    # Print top regions
    print("Top regions by total time:")
    print("-" * 80)
    print(f"{'Region':<35} {'Time (ms)':<12} {'% Total':<10} {'Bins':<8}")
    print("-" * 80)

    for region_name, stats in sorted_regions:
        if stats['time'] > 0:
            time_ms = stats['time'] / 1e6
            percent = 100 * stats['time'] / total_time
            print(f"{region_name:<35} {time_ms:>10.2f}   {percent:>7.2f}%   {stats['n_bins']:>6}")

    print("-" * 80)
    print()

    # Create annotated ZR plot using uproot + matplotlib
    import uproot
    import matplotlib.colors as mcolors
    from matplotlib.patches import Rectangle

    with uproot.open(input_file) as uf:
        h = uf["m_zr"]
        counts, z_edges, r_edges = h.to_numpy()

    fig_zr, ax_zr = plt.subplots(figsize=(14, 7))
    counts_plot = np.where(counts > 0, counts, np.nan)
    im = ax_zr.pcolormesh(z_edges, r_edges, counts_plot.T,
                           norm=mcolors.LogNorm(vmin=np.nanmin(counts_plot[counts_plot > 0]),
                                                vmax=np.nanmax(counts_plot)),
                           cmap='rainbow', shading='flat')
    cbar = fig_zr.colorbar(im, ax=ax_zr, pad=0.02)
    cbar.set_label('Time [ns]', fontsize=18)
    cbar.ax.tick_params(labelsize=16)

    ax_zr.set_xlabel('Z [mm]', fontsize=18)
    ax_zr.set_ylabel('R [mm]', fontsize=18)
    ax_zr.tick_params(axis='x', labelsize=16, rotation=45)
    ax_zr.tick_params(axis='y', labelsize=16)
    # Auto-zoom to Z range containing 99.9% of activity
    z_centers_arr = (z_edges[:-1] + z_edges[1:]) / 2
    z_content = counts.sum(axis=1)
    cumsum_z = np.cumsum(z_content)
    total_z = cumsum_z[-1]
    z_lo_idx = np.searchsorted(cumsum_z, 0.0005 * total_z)
    z_hi_idx = np.searchsorted(cumsum_z, 0.9995 * total_z)
    padding = (z_centers_arr[z_hi_idx] - z_centers_arr[z_lo_idx]) * 0.03
    ax_zr.set_xlim(z_centers_arr[z_lo_idx] - padding, z_centers_arr[z_hi_idx] + padding)
    ax_zr.set_ylim(r_edges[0], r_edges[-1])

    # Draw thin outline boxes for all regions
    for name, z_lo, z_hi, r_lo, r_hi in detector_regions:
        rect = Rectangle((z_lo, r_lo), z_hi - z_lo, r_hi - r_lo,
                          linewidth=1, edgecolor='black', facecolor='none')
        ax_zr.add_patch(rect)

    # Draw thick ranked boxes with white number labels
    top_n = len([r for r in sorted_regions if r[1]['time'] > 0])
    for idx, (region_name, stats) in enumerate(sorted_regions[:top_n]):
        if stats['time'] <= 0:
            continue
        if region_name == "DIRC" and 'sub_regions' in stats:
            all_z, all_r = [], []
            for sub_name, sub_stats in stats['sub_regions']:
                z_lo, z_hi = sub_stats['z_range']
                r_lo, r_hi = sub_stats['r_range']
                rect = Rectangle((z_lo, r_lo), z_hi - z_lo, r_hi - r_lo,
                                  linewidth=3, edgecolor='black', facecolor='none')
                ax_zr.add_patch(rect)
                all_z += [z_lo, z_hi]
                all_r += [r_lo, r_hi]
            z_center = min(all_z)
            r_center = (min(all_r) + max(all_r)) / 2.0
            ha = 'left'
        else:
            z_lo, z_hi = stats['z_range']
            r_lo, r_hi = stats['r_range']
            rect = Rectangle((z_lo, r_lo), z_hi - z_lo, r_hi - r_lo,
                              linewidth=3, edgecolor='black', facecolor='none')
            ax_zr.add_patch(rect)
            z_center = (z_lo + z_hi) / 2.0
            r_center = (r_lo + r_hi) / 2.0
            ha = 'center'

        ax_zr.text(z_center, r_center, str(idx + 1),
                   ha=ha, va='center', fontsize=22, fontweight='bold',
                   color='white',
                   bbox=dict(boxstyle='round,pad=0.15', facecolor='black',
                             edgecolor='none', alpha=0.6))

    plt.tight_layout()
    zr_png = f"{output_prefix}_zr_annotated.png"
    zr_pdf = f"{output_prefix}_zr_annotated.pdf"
    fig_zr.savefig(zr_png, dpi=150, bbox_inches='tight')
    fig_zr.savefig(zr_pdf, bbox_inches='tight')
    plt.close(fig_zr)
    print(f"Saved: {zr_png}")
    print(f"Saved: {zr_pdf}")

    # Write text summary
    # Compute per-PDG region sums
    pdg_labels = {11: "e-", -11: "e+", 22: "gamma", -22: "optical photon"}
    pdg_region_stats = {}
    for pdg, label in pdg_labels.items():
        hname = f"m_zr_pdg{pdg}"
        h = f.Get(hname)
        if not h:
            continue
        stats_pdg = {name: {'time': 0, 'n_bins': 0} for name, *_ in detector_regions}
        pdg_histo_total = 0
        for iz in range(1, h.GetNbinsX() + 1):
            for ir in range(1, h.GetNbinsY() + 1):
                value = h.GetBinContent(iz, ir)
                if value <= 0:
                    continue
                pdg_histo_total += value
                z = h.GetXaxis().GetBinCenter(iz)
                r = h.GetYaxis().GetBinCenter(ir)
                matched = False
                for name, z_lo, z_hi, r_lo, r_hi in detector_regions:
                    if z_lo <= z < z_hi and r_lo <= r < r_hi:
                        stats_pdg[name]['time'] += value
                        stats_pdg[name]['n_bins'] += 1
                        matched = True
                        break
        # Combine DIRC sub-regions
        dirc_time = stats_pdg.get("DIRC Bar (straight)", {}).get('time', 0) + \
                    stats_pdg.get("DIRC Bar (triangle)", {}).get('time', 0)
        dirc_bins = stats_pdg.get("DIRC Bar (straight)", {}).get('n_bins', 0) + \
                    stats_pdg.get("DIRC Bar (triangle)", {}).get('n_bins', 0)
        combined_pdg = {k: v for k, v in stats_pdg.items()
                        if k not in ("DIRC Bar (straight)", "DIRC Bar (triangle)")}
        combined_pdg["DIRC"] = {'time': dirc_time, 'n_bins': dirc_bins}
        pdg_assigned = sum(s['time'] for s in combined_pdg.values())
        combined_pdg["other regions"] = {'time': max(0, pdg_histo_total - pdg_assigned), 'n_bins': 0}
        pdg_total = pdg_histo_total
        pdg_region_stats[pdg] = {'label': label, 'regions': combined_pdg, 'total': pdg_total}

    # Build unified table: rows=regions, cols=total + per-PDG
    # Collect all region names in sorted order
    all_region_names = [name for name, stats in sorted_regions if stats['time'] > 0]
    # Compute "other regions" time = total - sum of all defined regions
    assigned_time = sum(stats['time'] for _, stats in sorted_regions if stats['time'] > 0)
    other_regions_time = max(0, total_time - assigned_time)
    all_region_names_with_other = all_region_names + ["other regions"]
    present_pdgs = [pdg for pdg in pdg_labels if pdg in pdg_region_stats]

    txt_file = f"{output_prefix}_profile.txt"
    with open(txt_file, 'w') as tf:
        tf.write(f"Performance Profile Summary\n")
        tf.write(f"Input: {input_file}\n")
        tf.write(f"Total time: {total_time:.2e} ns ({total_time/1e6:.1f} ms)\n")
        tf.write(f"\n")

        # Header: region | total % | per-PDG %
        pdg_col_labels = [pdg_labels[p] for p in present_pdgs]
        hdr_region = f"{'Region':<35}"
        hdr_total  = f"{'Total ms':>12} {'%tot':>6}"
        hdr_pdgs   = "".join(f"  {lbl+' ms':>12} {'%tot':>6}" for lbl in pdg_col_labels)
        tf.write(hdr_region + hdr_total + hdr_pdgs + "\n")
        tf.write("-" * (35 + 20 + 20 * len(present_pdgs)) + "\n")

        for region_name in all_region_names_with_other:
            if region_name == "other regions":
                region_time = other_regions_time
            else:
                region_time = combined_stats[region_name]['time'] if region_name in combined_stats else 0
            total_ms  = region_time / 1e6
            total_pct = 100 * region_time / total_time

            row = f"{region_name:<35}{total_ms:>12.1f} {total_pct:>5.1f}%"

            for pdg in present_pdgs:
                pdata = pdg_region_stats[pdg]
                t   = pdata['regions'].get(region_name, {}).get('time', 0)
                ms  = t / 1e6
                pct = 100 * t / total_time
                row += f"  {ms:>12.1f} {pct:>5.1f}%"

            tf.write(row + "\n")

        tf.write(f"\n")
        tf.write(f"Region geometry:\n")
        for name, z_lo, z_hi, r_lo, r_hi in detector_regions:
            tf.write(f"  {name}: Z=[{z_lo:.0f},{z_hi:.0f}] mm, R=[{r_lo:.0f},{r_hi:.0f}] mm\n")

    print(f"Saved: {txt_file}")
    print()

    # Heatmap: rows=particles+other, cols=regions+other regions, values=% of total sim time
    if present_pdgs:
        pdg_colors = {11: '#4e79a7', -11: '#f28e2b', 22: '#59a14f', -22: '#e15759', 'other': '#bab0ac'}
        pdg_names  = {11: 'e-', -11: 'e+', 22: 'gamma', -22: 'opt. photon', 'other': 'other'}

        pdg_row_keys = ['other'] + present_pdgs
        heatmap_data = np.zeros((len(pdg_row_keys), len(all_region_names_with_other)))
        for ri, region_name in enumerate(all_region_names_with_other):
            if region_name == "other regions":
                region_total = other_regions_time
            else:
                region_total = combined_stats[region_name]['time'] if region_name in combined_stats else 0
            accounted = 0
            for pi, pdg in enumerate(present_pdgs):
                t = pdg_region_stats[pdg]['regions'].get(region_name, {}).get('time', 0)
                heatmap_data[pi + 1, ri] = 100 * t / total_time
                accounted += t
            heatmap_data[0, ri] = 100 * max(0, region_total - accounted) / total_time

        row_labels = [pdg_names[p] for p in pdg_row_keys]
        col_labels = all_region_names_with_other

        fig, ax = plt.subplots(figsize=(max(10, 1.5 * len(col_labels)), max(4, 1.2 * len(row_labels))))
        im = ax.imshow(heatmap_data, aspect='auto', cmap='YlOrRd')

        ax.set_xticks(np.arange(len(col_labels)))
        ax.set_yticks(np.arange(len(row_labels)))
        ax.set_xticklabels(col_labels, rotation=25, ha='right', fontsize=14)
        ax.set_yticklabels(row_labels, fontsize=14)

        for pi in range(len(row_labels)):
            for ri in range(len(col_labels)):
                v = heatmap_data[pi, ri]
                if v > 0.05:
                    textcolor = 'white' if v > heatmap_data.max() * 0.6 else 'black'
                    ax.text(ri, pi, f'{v:.1f}%', ha='center', va='center',
                            fontsize=12, color=textcolor, fontweight='bold')

        cbar = fig.colorbar(im, ax=ax, fraction=0.03, pad=0.02)
        cbar.set_label('% of total simulation time', fontsize=13)
        cbar.ax.tick_params(labelsize=12)
        ax.set_title('Simulation time (% of total) by particle type and detector region',
                     fontsize=14, fontweight='bold')
        plt.tight_layout()
        heatmap_file = f"{output_prefix}_heatmap.png"
        fig.savefig(heatmap_file, dpi=150, bbox_inches='tight')
        plt.close(fig)
        print(f"Saved: {heatmap_file}")
        print()

    f.Close()
    return 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: plot_timing_profile.py <histos.root> [output_prefix]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_prefix = sys.argv[2] if len(sys.argv) > 2 else None

    sys.exit(analyze_hot_regions(input_file, output_prefix))
