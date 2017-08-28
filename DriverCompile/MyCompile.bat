@echo off

rem %1 Debug\Release
rem %2 Win32\x64
rem %3 makefile����Ŀ¼
rem %4 ϣ��Debug\Release����Ŀ¼
rem %5 Ŀ���ļ���
rem ..\..\Compile\MyCompile.bat $(Configuration) $(Platform) $(ProjectDir) $(SolutionDir) $(TargetName)

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
) else (
set X86_OR_X64=x64
set WXP_OR_WIN7=win7
set X86_OR_AMD64=amd64
set I386_OR_AMD64=amd64
)

if not exist "%WINDDK7ROOT%" (
echo �޷�����
echo �����û�������WINDDK7ROOT��ֵΪwinddk7��װĿ¼��������
echo C:\WinDDK\7600.16385.1
goto End
)

cd /d "%WINDDK7ROOT%\bin"
call "setenv.bat" %WINDDK7ROOT% %CHK_OR_FRE% %X86_OR_X64% %WXP_OR_WIN7%
cd /d "%3"

if exist "..\..\IncreateVersion.bat" call "..\..\IncreateVersion.bat" "%~3\DGFile.rc"

build

if not "1"=="%errorlevel%" (
cd /d "%4"
if not exist "%4\%1\%X86_OR_X64%" (
mkdir "%4\%1\%X86_OR_X64%"
)
copy /y "%3\obj%CHK_OR_FRE%_%WXP_OR_WIN7%_%X86_OR_AMD64%\%I386_OR_AMD64%\%5.pdb" "%4\%1\%X86_OR_X64%\%5.pdb"
copy /y "%3\obj%CHK_OR_FRE%_%WXP_OR_WIN7%_%X86_OR_AMD64%\%I386_OR_AMD64%\%5.sys" "%4\%1\%X86_OR_X64%\%5.sys"
if exist "..\CoreWizard\DGFileSysSetup\res" (
copy /y "%4\%1\%X86_OR_X64%\%5.sys" "..\CoreWizard\DGFileSysSetup\res\%5%X86_OR_X64%.sys"
) else (
copy /y "%4\%1\%X86_OR_X64%\%5.sys" "..\..\CoreWizard\DGFileSysSetup\res\%5%X86_OR_X64%.sys"
)
if "Release"=="%1" (
if not exist "%HuaTuSoftSign%" (
echo �޷�ǩ��
echo �����û�������HuaTuSoftSign��ǩ�����ַhttps://192.168.20.225/svn/DEV/��������
echo E:\DEV\HuaTuSoftSign
) else (
rem if exist "..\CoreWizard\DGFileSysSetup\res" (
rem "%HuaTuSoftSign%\SignNeedArguments.bat" "..\CoreWizard\DGFileSysSetup\res\%5%X86_OR_X64%.sys"
rem ) else (
rem "%HuaTuSoftSign%\SignNeedArguments.bat" "..\..\CoreWizard\DGFileSysSetup\res\%5%X86_OR_X64%.sys"
rem )
)
)
rem if "Release"=="%1" (
if not exist "%HuaTuSoftSymbols%" (
echo �޷�����pdb�����ſ�Ŀ¼
echo �����û�������HuaTuSoftSymbols�����ſ��ַhttps://192.168.20.225/svn/HuaTuSoftSymbols/��������
echo E:\HuaTuSoftSymbols
) else (
copy /y "%3\obj%CHK_OR_FRE%_%WXP_OR_WIN7%_%X86_OR_AMD64%\%I386_OR_AMD64%\%5.pdb" "%HuaTuSoftSymbols%\%5\%X86_OR_X64%\%5.pdb"
)
rem )
)

:End