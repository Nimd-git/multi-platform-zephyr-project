@echo off
REM Wrapper for esptool: bypasses pip-entrypoint .exe launcher which silently
REM fails when invoked from nested cmd.exe /C (ninja POST_BUILD chains).
REM Instead, run esptool as a Python module directly.
REM
REM Place this directory EARLIEST in PATH so cmd.exe PATHEXT search finds
REM esptool.cmd here before esptool.exe in .venv\Scripts.

"%~dp0..\.venv\Scripts\python.exe" -m esptool %*
