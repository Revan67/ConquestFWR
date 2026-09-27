@echo off
rem Modern project files link debug DLLs and PDBs directly into
rem Code\Libs\ExplicitDLL\Debug.  Retain this hook as a successful no-op for
rem projects that still reference the historical CQ2 post-build event.
exit /b 0
