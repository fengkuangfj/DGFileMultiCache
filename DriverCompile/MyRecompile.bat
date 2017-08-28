@echo off

set BAT_DIR=%~dp0

rem %1 Debug\Release
rem %2 Win32\x64
rem %3 makefile所在目录
rem %4 解决方案所在目录
rem %5 项目所在目录
rem %6 希望Debug\Release所在目录
rem %7 目标文件名
rem ..\..\Compile\MyRecompile.bat $(Configuration) $(Platform) $(ProjectDir) $(SolutionDir) $(ProjectDir) $(SolutionDir) $(TargetName)

call %BAT_DIR%\MyCleanup.bat %1 %2 %3 %4 %5
call %BAT_DIR%\MyCompile.bat %1 %2 %3 %6 %7
