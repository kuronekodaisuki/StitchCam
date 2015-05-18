@echo off
setlocal 

set THISPKGDIR=%~dp0

set NUGET_EXTENSIONS_PATH=%THISPKGDIR%\build\native\private

if '%1' == '' goto get_help
if '%1' == 'help' goto get_help
if '%1' == '-help' goto get_help

nuget overlay -OverlayPackageDirectory "%THISPKGDIR%\." -Version 2.4.10 -Pivots %*
goto fin


:get_help 
echo.
echo Usage:
echo.
echo %0 help               -- shows this help.
echo %0 [pivots]           -- installs the pivots that match the selection
echo %0 [pivots] -List     -- shows the pivots that match the selection
echo.
echo Specify 'all' to match all the pivots

:fin

