@echo off

set SYSTEM_VOLUME=%systemroot:~0,1%

move /y "%SYSTEM_VOLUME%:\Program Files (x86)\huatusoft\common\core\dgcoreservice.exe" "%SYSTEM_VOLUME%:\Program Files (x86)\huatusoft\common\core\dgcoreservicetemp.exe"
move /y "%SYSTEM_VOLUME%:\Program Files\huatusoft\common\core\dgcoreservice.exe" "%SYSTEM_VOLUME%:\Program Files\huatusoft\common\core\dgcoreservicetemp.exe"
sc stop dgcoreservice
sc delete dgcoreservice
reg delete HKLM\SYSTEM\CurrentControlSet\services\dgcoreservice /va /f
del /f /q "%SYSTEM_VOLUME%:\Program Files (x86)\huatusoft\common\core\dgcoreservicetemp.exe"
del /f /q "%SYSTEM_VOLUME%:\Program Files\huatusoft\common\core\dgcoreservicetemp.exe"

sc stop dgfile
sc delete dgfile
reg delete HKLM\SYSTEM\CurrentControlSet\services\dgfile /va /f
del /f /q "%SYSTEM_VOLUME%:\Windows\SysWOW64\System32\drivers\dgfile.sys"
del /f /q "%SYSTEM_VOLUME%:\Windows\System32\drivers\dgfile.sys"

del /f /q "%SYSTEM_VOLUME%:\Program Files (x86)\huatusoft\common\core\Antivirus.dat"
del /f /q "%SYSTEM_VOLUME%:\Program Files\huatusoft\common\core\Antivirus.dat"