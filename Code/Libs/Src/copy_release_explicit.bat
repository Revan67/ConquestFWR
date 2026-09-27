@echo off
rem Modern project files link release DLLs and PDBs directly into
rem Code\Libs\ExplicitDLL\Release.  The original CQ2 post-build step copied
rem through a developer-specific Z: drive and now only causes otherwise
rem successful local builds to fail.
exit /b 0
