@echo off

sc stop dgfile
sc delete dgfile
reg delete HKLM\SYSTEM\CurrentControlSet\services\dgfile /va /f
del /f /q "%systemroot%\System32\drivers\dgfile.sys"
del /f /q "%systemroot%\SysWOW64\System32\drivers\dgfile.sys"