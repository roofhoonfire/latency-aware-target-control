"""Convert two debugger CSV exports into numeric and visual reports (stdlib only).

Usage: python tools/compare_runtime.py stm32.csv zynq.csv --output results/run1
No synthetic measurements are substituted for missing data.
"""
import argparse
import csv
import html
import io
import json
from pathlib import Path
from statistics import mean


def read_trace(path):
    metadata, lines = {}, []
    for line in Path(path).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("# ") and "=" in line:
            key, value = line[2:].split("=", 1)
            metadata[key] = int(value)
        elif line.strip():
            lines.append(line)
    rows = [{k: int(v) for k, v in row.items()}
            for row in csv.DictReader(io.StringIO("\n".join(lines)))]
    if not rows or metadata.get("samples", 0) <= 0:
        raise ValueError(f"{path}: no measured samples")
    if metadata.get("clock_hz", 0) <= 0 or metadata.get("period_us", 0) <= 0:
        raise ValueError(f"{path}: missing/nonpositive clock or period")
    required = {"cycle", "common_us", "work_us", "release_error_us",
                "steering_mdeg", "pulse_us", "target_x_cm", "target_y_cm"}
    if not required.issubset(rows[0]):
        raise ValueError(f"{path}: missing trace columns")
    if any(b["cycle"] != a["cycle"] + 1 for a, b in zip(rows, rows[1:])):
        raise ValueError(f"{path}: trace is not chronological; halt before export")
    summary = {
        "retained_samples": len(rows),
        "retained_common_mean_us": mean(r["common_us"] for r in rows),
        "retained_common_max_us": max(r["common_us"] for r in rows),
        "retained_release_abs_max_us": max(abs(r["release_error_us"]) for r in rows),
        "run_samples": metadata["samples"],
        "run_common_min_us": metadata["common_min_us"],
        "run_common_mean_us": metadata["common_total_us"] / metadata["samples"],
        "run_common_max_us": metadata["common_max_us"],
        "run_release_abs_max_us": metadata["release_abs_max_us"],
        "run_deadline_misses": metadata["deadline_misses"],
        "run_queue_drops": metadata["dropped_targets"],
        "clock_hz": metadata["clock_hz"],
        "tick_hz": metadata["tick_hz"],
        "period_us": metadata["period_us"],
    }
    # Old polling exports have no IRQ counters: preserve missing as unknown,
    # rather than presenting an unmeasured value as zero.
    for key in ("uart_rx_ready", "uart_rx_irq_count", "uart_rx_bytes_read",
                "uart_rx_queue_drops", "uart_rx_error_events",
                "uart_rx_overrun_events", "uart_rx_fifo_resets", "uart_rx_gap_resets"):
        summary[key] = metadata.get(key)
    return summary, rows


def make_report(stm32_path, zynq_path, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    summaries, traces = {}, {}
    for name, path in (("STM32", stm32_path), ("Zynq", zynq_path)):
        summaries[name], traces[name] = read_trace(path)
    if summaries["STM32"]["period_us"] != summaries["Zynq"]["period_us"]:
        raise ValueError("Control periods differ; rerun using matching periods")
    (output / "summary.json").write_text(json.dumps(summaries, indent=2), encoding="utf-8")
    with (output / "summary.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["metric", "STM32", "Zynq"])
        for key in summaries["STM32"]:
            writer.writerow([key, summaries["STM32"][key], summaries["Zynq"][key]])
    panels = [
        ("Common pipeline mean (retained window, us)", "retained_common_mean_us"),
        ("Common pipeline max (retained window, us)", "retained_common_max_us"),
        ("Absolute release error max (retained window, us)", "retained_release_abs_max_us"),
        ("Observed deadline misses (whole run)", "run_deadline_misses"),
    ]
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="720" viewBox="0 0 1000 720">',
           '<rect width="1000" height="720" fill="#101827"/>',
           '<g font-family="Arial,sans-serif" fill="#eef2ff">',
           '<text x="35" y="42" font-size="24">STM32 / Zynq runtime observations</text>',
           '<text x="35" y="68" font-size="13">Measured debugger exports; no WCET or physical PWM claim. Each panel has its own scale.</text>']
    for index, (title, key) in enumerate(panels):
        top = 100 + index * 145
        svg.append(f'<text x="35" y="{top}" font-size="17">{title}</text>')
        scale = max(summaries[n][key] for n in summaries) or 1
        for j, (name, color) in enumerate((("STM32", "#60a5fa"), ("Zynq", "#34d399"))):
            y, value = top + 16 + j * 40, summaries[name][key]
            width = 670 * value / scale
            svg.extend([f'<text x="35" y="{y+21}">{name}</text>',
                        f'<rect x="110" y="{y}" width="{width:.2f}" height="28" rx="4" fill="{color}"/>',
                        f'<text x="{125+width:.2f}" y="{y+21}">{value:.3f}</text>'])
    svg.append('</g></svg>')
    chart = "\n".join(svg)
    (output / "comparison.svg").write_text(chart, encoding="utf-8")
    def cell(value):
        return "not recorded" if value is None else f"{value:.3f}"
    table = "".join(f"<tr><td>{html.escape(k)}</td><td>{cell(summaries['STM32'][k])}</td>"
                    f"<td>{cell(summaries['Zynq'][k])}</td></tr>" for k in summaries["STM32"])
    rx_warnings = []
    for name, summary in summaries.items():
        if summary["uart_rx_ready"] != 1:
            rx_warnings.append(f"{name}: IRQ receiver readiness is absent or not confirmed.")
        if any((summary[k] or 0) > 0 for k in
               ("uart_rx_queue_drops", "uart_rx_error_events", "run_queue_drops")):
            rx_warnings.append(f"{name}: input loss/error recorded; label this as a fault/load run.")
    warning_html = "".join(f"<p>{html.escape(w)}</p>" for w in rx_warnings)
    document = f'''<!doctype html><meta charset="utf-8"><title>Runtime comparison</title>
<style>body{{font:16px system-ui;max-width:1100px;margin:32px auto;background:#101827;color:#eef2ff}}
svg{{width:100%;height:auto}}td,th{{padding:9px 16px;border-bottom:1px solid #334155}}table{{border-collapse:collapse}}</style>
{chart}{warning_html}<p>Common timing covers controller step + mapper only, including preemption.
Work timing differs: STM32 includes its PWM backend; Zynq has no physical backend.
Release error is relative to the first scheduled release, not synchronized between boards.
Retained window contains at most 128 cycles; run statistics include startup and timeout cycles.
Use identical input replay, build optimization, run duration and workload before interpreting differences.
Output equivalence is not automatically established by this report.</p>
<table><tr><th>Metric</th><th>STM32</th><th>Zynq</th></tr>{table}</table>'''
    (output / "report.html").write_text(document, encoding="utf-8")
    return summaries


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stm32")
    parser.add_argument("zynq")
    parser.add_argument("--output", default="results/runtime")
    args = parser.parse_args()
    make_report(args.stm32, args.zynq, args.output)
    print(Path(args.output).resolve() / "report.html")
