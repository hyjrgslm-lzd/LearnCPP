"""Bounded course-tool child processes; never terminate unrelated processes.

A failure, timeout or unconfirmed cleanup remains FAIL. Text logs are decoded
as UTF-8 with replacement; run MSVC with VSLANG=1033 when collecting text evidence.
"""
from __future__ import annotations
import math
import os
import signal
import subprocess
import time
from typing import Any


def run_process(command: list[str], timeout: float) -> dict[str, Any]:
    if not command or not math.isfinite(timeout) or timeout <= 0:
        raise ValueError("a command and a finite positive timeout are required before launch")
    settings: dict[str, Any]
    if os.name == "nt":
        settings = {"creationflags": subprocess.CREATE_NO_WINDOW | subprocess.CREATE_NEW_PROCESS_GROUP}
    else:
        settings = {"start_new_session": True}
    started = time.monotonic()
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True, encoding="utf-8", errors="replace", **settings)
    except OSError as error:
        return {"command": command, "exit_code": None, "timeout": False,
                "process_seconds": time.monotonic() - started, "stdout": "", "stderr": "",
                "cleanup_error": "", "error": f"launch failed: {error}", "status": "FAIL"}
    timed_out = False
    cleanup_errors: list[str] = []
    failure = ""
    stdout = stderr = ""
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except BaseException as error:
        timed_out = isinstance(error, subprocess.TimeoutExpired)
        failure = f"{type(error).__name__}: {error}"
        try:
            if os.name == "nt":
                killed = subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                                        capture_output=True, text=True, errors="replace", timeout=5,
                                        creationflags=subprocess.CREATE_NO_WINDOW)
                if killed.returncode:
                    cleanup_errors.append("tree termination: " + (killed.stderr or killed.stdout))
            else:
                os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        except BaseException as cleanup_error:
            cleanup_errors.append(f"tree termination failed: {cleanup_error}")
        finally:
            try:
                if process.poll() is None:
                    process.kill()
            except ProcessLookupError:
                pass
            except BaseException as cleanup_error:
                cleanup_errors.append(f"root termination failed: {cleanup_error}")
        try:
            stdout, stderr = process.communicate(timeout=5)
        except BaseException as cleanup_error:
            cleanup_errors.append(f"pipe collection failed: {cleanup_error}")
            try:
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5)
            except BaseException as reap_error:
                cleanup_errors.append(f"reap failed: {reap_error}")
    status = "PASS" if not failure and not cleanup_errors and process.returncode == 0 else "FAIL"
    return {"command": command, "exit_code": process.returncode, "timeout": timed_out,
            "process_seconds": time.monotonic() - started, "stdout": stdout, "stderr": stderr,
            "cleanup_error": "; ".join(cleanup_errors), "error": failure, "status": status}
