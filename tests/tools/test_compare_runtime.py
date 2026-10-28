import importlib.util
from pathlib import Path
import tempfile
import unittest

MODULE = Path(__file__).resolve().parents[2] / "tools/compare_runtime.py"
spec = importlib.util.spec_from_file_location("compare_runtime", MODULE)
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)


def fixture(path, period=20000):
    path.write_text(f"""# samples=2
# common_min_us=3
# common_max_us=9
# common_total_us=12
# work_max_us=120
# release_abs_max_us=10
# deadline_misses=0
# dropped_targets=0
# clock_hz=1000000
# tick_hz=1000
# period_us={period}
cycle,common_us,work_us,release_error_us,steering_mdeg,pulse_us,target_x_cm,target_y_cm
0,3,100,-10,-11188,1388,515,-97
1,9,120,5,-11188,1388,515,-97
""", encoding="utf-8")


class ReportTests(unittest.TestCase):
    def test_numeric_values_and_standalone_outputs(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            fixture(root / "a.csv")
            fixture(root / "b.csv")
            data = report.make_report(root / "a.csv", root / "b.csv", root / "out")
            self.assertEqual(data["STM32"]["retained_common_mean_us"], 6)
            self.assertEqual(data["STM32"]["retained_release_abs_max_us"], 10)
            for name in ("report.html", "comparison.svg", "summary.csv", "summary.json"):
                self.assertTrue((root / "out" / name).is_file())

    def test_mismatched_periods_are_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            fixture(root / "a.csv")
            fixture(root / "b.csv", period=10000)
            with self.assertRaisesRegex(ValueError, "periods differ"):
                report.make_report(root / "a.csv", root / "b.csv", root / "out")

    def test_nonchronological_trace_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "trace.csv"
            fixture(path)
            path.write_text(path.read_text().replace("1,9,120", "4,9,120"))
            with self.assertRaisesRegex(ValueError, "chronological"):
                report.read_trace(path)

    def test_old_exports_do_not_invent_zero_irq_errors(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "trace.csv"
            fixture(path)
            summary, _ = report.read_trace(path)
            self.assertIsNone(summary["uart_rx_error_events"])

    def test_irq_loss_counts_are_reported_and_flagged(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            for name in ("a.csv", "b.csv"):
                path = root / name
                fixture(path)
                path.write_text("# uart_rx_ready=1\n# uart_rx_queue_drops=7\n" + path.read_text())
            data = report.make_report(root / "a.csv", root / "b.csv", root / "out")
            self.assertEqual(data["STM32"]["uart_rx_queue_drops"], 7)
            self.assertIn("fault/load run", (root / "out/report.html").read_text())


if __name__ == "__main__":
    unittest.main()
