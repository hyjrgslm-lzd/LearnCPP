import sys
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from run_benchmarks import completion_code, parse_rows, run_process, summarize, unique_cases
from verify_materials import check_document, heading_slug


HEADER = "suite,variant,size,threads,milliseconds,completed,details\n"


class BenchmarkToolsTest(unittest.TestCase):
    def test_csv_and_quoted_details(self):
        row = parse_rows(HEADER + 'queue,mutex,10,2,0.5,10,"cold, bounded"\n')[0]
        self.assertEqual(row["completed"], 10)
        self.assertEqual(row["details"], "cold, bounded")
        row = parse_rows(HEADER + 'compute,par,10,0,0.5,10,"backend thread count unobserved"\n')[0]
        self.assertEqual(row["threads"], 0)

    def test_bad_measurements_are_rejected(self):
        for duration in ("nan", "inf", "-1"):
            with self.assertRaises(ValueError):
                parse_rows(HEADER + f"queue,mutex,10,2,{duration},10,\n")
        for value in ("oops\n", HEADER, HEADER + "queue,mutex,10,-1,1,10,\n",
                      HEADER + "queue,mutex,10,2,1,-1,\n",
                      HEADER + "queue,mutex,10,2,1,10,unexpected,extra\n",
                      HEADER + 'queue,mutex,10,2,1,10,"truncated'):
            with self.assertRaises(ValueError):
                parse_rows(value)

    def test_exit_status(self):
        for code, expected in ((0, "PASS"), (1, "FAIL"), (77, "SKIP")):
            result = run_process([sys.executable, "-c", f"raise SystemExit({code})"], 5)
            self.assertEqual(result["status"], expected)

    def test_external_timeout(self):
        result = run_process([sys.executable, "-c", "import time; time.sleep(60)"], 0.1)
        self.assertTrue(result["timeout"])
        self.assertEqual(result["status"], "FAIL")
        self.assertIsNotNone(result["exit_code"])

    def test_timeout_validated_before_process_creation(self):
        for timeout in (float("nan"), float("inf"), -1, 0):
            with mock.patch("run_benchmarks.subprocess.Popen") as launch:
                with self.assertRaises(ValueError):
                    run_process([sys.executable, "-c", "pass"], timeout)
                launch.assert_not_called()

    def test_post_launch_exception_reaps_root(self):
        process = mock.Mock(pid=123, returncode=1)
        process.poll.return_value = None
        process.communicate.side_effect = [ValueError("injected wait failure"), ("", "")]
        with mock.patch("run_benchmarks.subprocess.Popen", return_value=process), \
             mock.patch("run_benchmarks.subprocess.run", side_effect=subprocess.TimeoutExpired("taskkill", 5)), \
             mock.patch("run_benchmarks.os.killpg", create=True):
            result = run_process(["not-really-launched"], 1)
        process.kill.assert_called_once()
        self.assertEqual(result["status"], "FAIL")
        self.assertIn("injected wait failure", result["error"])

    @unittest.skipUnless(os.name == "nt", "Windows taskkill fallback")
    def test_tree_termination_timeout_still_kills_root(self):
        with mock.patch("run_benchmarks.subprocess.run", side_effect=subprocess.TimeoutExpired("taskkill", 5)):
            result = run_process([sys.executable, "-c", "import time; time.sleep(60)"], 0.1)
        self.assertEqual(result["status"], "FAIL")
        self.assertIsNotNone(result["exit_code"])
        self.assertIn("tree termination failed", result["cleanup_error"])

    def test_samples_are_distinct_process_repetitions(self):
        row = parse_rows(HEADER + "queue,mutex,10,2,1,10,\n")[0]
        with self.assertRaises(ValueError):
            unique_cases([row, row])
        with self.assertRaises(ValueError):
            summarize([{**row, "repetition": 0} for _ in range(5)], 5)
        valid = [{**row, "repetition": i, "milliseconds": i + 1} for i in range(5)]
        self.assertEqual(summarize(valid, 5)[0]["median_ms"], 3)

    def test_partial_results_require_explicit_permission(self):
        self.assertEqual(completion_code("PARTIAL_SKIP"), 77)
        self.assertEqual(completion_code("PARTIAL_SKIP", True), 0)
        self.assertEqual(completion_code("SKIP", True), 77)
        self.assertEqual(completion_code("FAIL", True), 1)

    def test_local_links_and_code_fences(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            target = root / "target.md"
            target.write_text('# 自测与答案\n\n<a id="explicit"></a>\n', encoding="utf-8")
            source = root / "source.md"
            source.write_text('[ok](target.md#自测与答案)\n[ok](target.md#explicit)\n'
                              '```text\n[example](missing.md)\n```\n', encoding="utf-8")
            self.assertEqual(check_document(source, root, {}), [])
            source.write_text('[bad](target.md#missing)\n[bad](missing.md)\n', encoding="utf-8")
            self.assertEqual(len(check_document(source, root, {})), 2)
            self.assertEqual(heading_slug('A `std::atomic<T>` test'), 'a-stdatomict-test')
            self.assertEqual(heading_slug('[Label](target.md) <em>text</em>'), 'label-text')


if __name__ == "__main__":
    unittest.main()
