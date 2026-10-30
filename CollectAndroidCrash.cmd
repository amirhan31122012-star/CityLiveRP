@echo off
setlocal
chcp 65001 >nul
echo Подключите телефон по USB, откройте игру и дождитесь вылета.
echo Сейчас будут прочитаны журналы. Файлы игры и данные телефона не меняются.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\CollectAndroidCrash.ps1" -ProjectDir "%~dp0."
pause
