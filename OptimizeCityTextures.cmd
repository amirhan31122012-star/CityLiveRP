@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Сначала закройте Unreal Engine.
    pause
    exit /b 1
)
set "UE_ROOT="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 не найден.
    pause
    exit /b 1
)
set "CITYLIVE_TEXTURE_TOKEN=%RANDOM%_%RANDOM%_%RANDOM%"
set "TEXTURE_LOG_DIR=%PROJECT_DIR%Saved\CityTextureBudget\%CITYLIVE_TEXTURE_TOKEN%"
mkdir "%TEXTURE_LOG_DIR%"
echo Оптимизируются текстуры активной карты города. Повторного импорта геометрии нет.
echo Дождитесь окончания. Журнал: %TEXTURE_LOG_DIR%\Optimize.log
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" /Engine/Maps/Entry "-ExecutePythonScript=%PROJECT_DIR%Scripts\OptimizeCityTextures.py" "-abslog=%TEXTURE_LOG_DIR%\Optimize.log" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%TEXTURE_LOG_DIR%\complete.ok" goto failure
echo Готово. Откройте BuildAndOpen.cmd и проверьте расход памяти в игре.
pause
exit /b 0
:failure
echo Оптимизация не завершена. Пришлите Optimize.log из открывающейся папки.
start "" "%TEXTURE_LOG_DIR%"
pause
exit /b 1
