import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("exercise", Path(__file__).resolve().parents[2] / "tools/uart_rx_exercise.py")
exercise = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exercise)


class ExerciseTests(unittest.TestCase):
    def test_wire_layout(self):
        # Header and payload checked independently of the producer's packing.
        data = exercise.frame(0x1234, x=515, y=-97)
        self.assertEqual(data[:13], bytes.fromhex("aa55010108341203029fff0000"))
        self.assertEqual(len(data), 15)

    def test_fragmentation_preserves_frame(self):
        chunks = exercise.scenario("fragmented")
        self.assertEqual(b"".join(b for _, b in chunks), exercise.frame(1))
        self.assertTrue(all(len(b) == 1 and t > 0 for t, b in chunks))

    def test_recovery_ends_with_two_complete_good_frames(self):
        chunks = exercise.scenario("recovery")
        self.assertNotEqual(chunks[1][1], exercise.frame(1))
        self.assertEqual(len(chunks[2][1]), 7)
        self.assertEqual(chunks[-2][1], exercise.frame(3))
        self.assertEqual(chunks[-1][1], exercise.frame(4))

    def test_burst_length(self):
        self.assertEqual(len(exercise.scenario("burst", 200)[0][1]), 3000)


if __name__ == "__main__":
    unittest.main()
