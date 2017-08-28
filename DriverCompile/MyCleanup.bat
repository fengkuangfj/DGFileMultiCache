@echo off

rem %1 Debug\Release
rem %2 Win32\x64
rem %3 makefile所在目录
rem %4 解决方案所在目录
rem %5 项目所在目录
rem ..\..\Compile\MyCleanup.bat $(Configuration) $(Platform) $(ProjectDir) $(SolutionDir) $(ProjectDir)

if "Debug"=="%1" (
set CHK_OR_FRE=chk
) else (
set CHK_OR_FRE=fre
)

if "Win32"=="%2" (
set X86_OR_X64=x86
set WXP_OR_WIN7=wxp
set X86_OR_AMD64=x86
set I386_OR_AMD64=i386
set DEBUG_OR_X64=Debug
) else (
set X86_OR_X64=x64
set WXP_OR_WIN7=win7
set X86_OR_AMD64=amd64
set I386_OR_AMD64=amd64
set DEBUG_OR_X64=x64
)

cd /d "%4"

for /r . %%d in (.) do rd /s /q "%3\obj%CHK_OR_FRE%_%WXP_OR_WIN7%_%X86_OR_AMD64%" 2>nul
del /a /f /s "%3\*.log" 2>nul

for /r . %%d in (.) do rd /s /q "%4\%1\%X86_OR_X64%" 2>nul

for /r . %%d in (.) do rd /s /q "%5\obj\%2" 2>nul
for /r . %%d in (.) do rd /s /q "%5\%DEBUG_OR_X64%" 2>nul

if "2"=="errorlevel" (
set errorlevel=0
)
