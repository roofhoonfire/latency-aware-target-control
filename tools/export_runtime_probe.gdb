# Run only while HALTED, after an uninterrupted run. Change the output filename
# below for each board. Compatible with GDB command language; no Python needed.
set pagination off
set logging file runtime_trace.csv
set logging overwrite on
set logging redirect on
set logging on
printf "# samples=%u\n", runtime_probe.samples
printf "# common_min_us=%u\n", runtime_probe.common_min_us
printf "# common_max_us=%u\n", runtime_probe.common_max_us
printf "# common_total_us=%llu\n", runtime_probe.common_total_us
printf "# work_max_us=%u\n", runtime_probe.work_max_us
printf "# release_abs_max_us=%u\n", runtime_probe.release_abs_max_us
printf "# deadline_misses=%u\n", runtime_probe.deadline_misses
printf "# clock_hz=%u\n", probe_clock_hz
printf "# tick_hz=%u\n", probe_tick_hz
printf "# period_us=%u\n", probe_period_us
printf "# received_frames=%u\n", received_frames
printf "# dropped_targets=%u\n", dropped_targets
printf "# accepted_targets=%u\n", accepted_targets
printf "# invalid_targets=%u\n", invalid_targets
printf "# rejected_windows=%u\n", rejected_windows
printf "# uart_rx_ready=%u\n", uart_rx_stats.ready
printf "# uart_rx_irq_count=%u\n", uart_rx_stats.irq_count
printf "# uart_rx_bytes_read=%u\n", uart_rx_stats.bytes_read
printf "# uart_rx_queue_drops=%u\n", uart_rx_stats.queue_drops
printf "# uart_rx_error_events=%u\n", uart_rx_stats.error_events
printf "# uart_rx_overrun_events=%u\n", uart_rx_stats.overrun_events
printf "# uart_rx_fifo_resets=%u\n", uart_rx_stats.fifo_resets
printf "# uart_rx_gap_resets=%u\n", uart_rx_gap_resets
printf "cycle,common_us,work_us,release_error_us,steering_mdeg,pulse_us,target_x_cm,target_y_cm\n"
set $n = runtime_probe.samples
set $capacity = sizeof(runtime_probe.trace) / sizeof(runtime_probe.trace[0])
set $first = 0
if $n > $capacity
  set $n = $capacity
  set $first = runtime_probe.trace_next
end
set $i = 0
while $i < $n
  set $j = ($first + $i) % $capacity
  printf "%u,%u,%u,%d,%d,%u,%d,%d\n", runtime_probe.trace[$j].cycle, runtime_probe.trace[$j].common_us, runtime_probe.trace[$j].work_us, runtime_probe.trace[$j].release_error_us, runtime_probe.trace[$j].steering_mdeg, runtime_probe.trace[$j].pulse_us, runtime_probe.trace[$j].target_x_cm, runtime_probe.trace[$j].target_y_cm
  set $i = $i + 1
end
set logging off
set logging redirect off
