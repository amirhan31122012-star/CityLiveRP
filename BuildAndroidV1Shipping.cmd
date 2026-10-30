@echo off
call "%~dp0BuildAndroidV1.cmd" "%~1" Shipping
exit /b %errorlevel%
