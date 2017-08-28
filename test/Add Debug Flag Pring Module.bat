@echo off

reg add HKLM\SYSTEM\CurrentControlSet\services\dgfile /v DebugFlag /t REG_DWORD /d 1 /f