@echo off
setlocal
chcp 65001 >nul
set "CITY_BUILD_SERIES=AndroidV2"
echo Сборка CityLive RP V2: профили, парковки, баланс и управление машиной.
call "%~dp0BuildAndroidV1.cmd" "%~1" Shipping
exit /b %errorlevel%
