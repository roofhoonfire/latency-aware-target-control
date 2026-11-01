"""Qualify an observed run against the v2 engineering timing contract.

Input is a halted-board export from export_runtime_probe.gdb. This does not
establish WCET, physical PWM correctness, or coverage of untested workloads.
"""
import argparse
import json
from pathlib import Path


def check(path, *, allow_input_loss=False, min_samples=1500):
    values = {}
    for line in Path(path).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("# ") and "=" in line:
            key, value = line[2:].split("=", 1)
            if key in values:
                raise ValueError(f"Duplicate metadata: {key}")
            values[key] = int(value)
    required = {
        "samples", "schedule_version", "period_us", "busy_budget_us",
        "release_budget_us", "control_priority", "rx_priority",
        "clock_hz", "tick_hz", "deadline_misses", "busy_budget_misses",
        "busy_max_us", "release_abs_max_us", "skipped_releases",
        "received_frames", "accepted_targets", "dropped_targets",
        "uart_rx_ready", "uart_rx_queue_drops", "uart_rx_error_events",
        "uart_rx_overrun_events",
    }
    missing = required - values.keys()
    if missing:
        raise ValueError(f"Missing measurement metadata: {', '.join(sorted(missing))}")
    if any(values[k] < 0 for k in required):
        raise ValueError("Negative measurement metadata")
    failures = []
    if (values["schedule_version"], values["period_us"],
        values["busy_budget_us"], values["release_budget_us"]) != (2, 20000, 5000, 1000):
        failures.append("Build does not match the v2 timing contract")
    if values["clock_hz"] <= 0 or values["tick_hz"] <= 0:
        failures.append("Invalid clock configuration")
    if values["control_priority"] <= values["rx_priority"]:
        failures.append("Control priority is not higher than receive priority")
    if values["samples"] < min_samples:
        failures.append("Insufficient measured cycles")
    if values["uart_rx_ready"] != 1:
        failures.append("Receiver not ready")
    for key in ("deadline_misses", "busy_budget_misses", "skipped_releases"):
        if values[key]:
            failures.append(f"{key}={values[key]}")
    if values["busy_max_us"] > 5000:
        failures.append("Observed busy interval exceeds 5 ms")
    if values["release_abs_max_us"] > 1000:
        failures.append("Observed release error exceeds 1 ms")
    losses = {k: values[k] for k in ("dropped_targets", "uart_rx_queue_drops",
              "uart_rx_error_events", "uart_rx_overrun_events")}
    if not allow_input_loss and any(losses.values()):
        failures.append("Input loss/error in a nominal qualification run")
    return {
        "status": "OBSERVED_FAIL" if failures else "OBSERVED_PASS_NOT_WCET",
        "failures": failures,
        "input_loss_allowed": allow_input_loss,
        "input_loss": losses,
        "measurements": values,
        "scope": "Observed software timing only; verify workload coverage separately.",
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace")
    parser.add_argument("--allow-input-loss", action="store_true",
                        help="Overload/fault runs only; timing limits still apply")
    parser.add_argument("--min-samples", type=int, default=1500)
    args = parser.parse_args()
    if args.min_samples < 1:
        parser.error("--min-samples must be positive")
    try:
        result = check(args.trace, allow_input_loss=args.allow_input_loss,
                       min_samples=args.min_samples)
    except (ValueError, OSError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))
    raise SystemExit(bool(result["failures"]))
