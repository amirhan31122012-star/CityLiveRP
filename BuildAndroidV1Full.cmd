@echo off
setlocal
set "CITYLIVE_ANDROID_FULL_COOK=1"
call "%~dp0BuildAndroidV1.cmd" "%~1"
exit /b %ERRORLEVEL%
