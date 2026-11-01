"""Synthetic fixtures test the checker, not the hardware's timing."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "check_realtime_trace", Path(__file__).resolve().parents[2] / "tools/check_realtime_trace.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class TimingContractTests(unittest.TestCase):
    def evaluate(self, changes=None, omit=(), **options):
        values = dict(samples=1500, schedule_version=2, period_us=20000,
                      busy_budget_us=5000, release_budget_us=1000,
                      control_priority=2, rx_priority=1, clock_hz=1000000,
                      tick_hz=100, deadline_misses=0, busy_budget_misses=0,
                      busy_max_us=5000, release_abs_max_us=1000,
                      skipped_releases=0, received_frames=300,
                      accepted_targets=300, dropped_targets=0, uart_rx_ready=1,
                      uart_rx_queue_drops=0, uart_rx_error_events=0,
                      uart_rx_overrun_events=0)
        values.update(changes or {})
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.csv"
            path.write_text(''.join(f'# {k}={v}\n' for k, v in values.items() if k not in omit))
            return module.check(path, **options)

    def test_exact_budget_boundaries_pass(self):
        self.assertEqual(self.evaluate()["status"], "OBSERVED_PASS_NOT_WCET")

    def test_over_budget_and_missed_releases_fail(self):
        for key, value in (("busy_max_us", 5001), ("release_abs_max_us", 1001),
                           ("deadline_misses", 1), ("skipped_releases", 1),
                           ("control_priority", 1), ("samples", 1499)):
            with self.subTest(key=key):
                self.assertEqual(self.evaluate({key: value})["status"], "OBSERVED_FAIL")

    def test_old_exports_are_not_treated_as_pass(self):
        with self.assertRaises(ValueError):
            self.evaluate(omit=("busy_max_us",))

    def test_overload_allows_loss_but_not_deadline_failure(self):
        self.assertEqual(self.evaluate({"dropped_targets": 5})["status"], "OBSERVED_FAIL")
        self.assertEqual(self.evaluate({"dropped_targets": 5}, allow_input_loss=True)["status"],
                         "OBSERVED_PASS_NOT_WCET")
        self.assertEqual(self.evaluate({"dropped_targets": 5, "deadline_misses": 1},
                                      allow_input_loss=True)["status"], "OBSERVED_FAIL")


if __name__ == "__main__":
    unittest.main()
