@echo off

set BAT_DIR=%~dp0

rem %1 Debug\Release
rem %2 Win32\x64
rem %3 makefile����Ŀ¼
rem %4 �����������Ŀ¼
rem %5 ��Ŀ����Ŀ¼
rem %6 ϣ��Debug\Release����Ŀ¼
rem %7 Ŀ���ļ���
rem ..\..\Compile\MyRecompile.bat $(Configuration) $(Platform) $(ProjectDir) $(SolutionDir) $(ProjectDir) $(SolutionDir) $(TargetName)

call %BAT_DIR%\MyCleanup.bat %1 %2 %3 %4 %5
call %BAT_DIR%\MyCompile.bat %1 %2 %3 %6 %7
