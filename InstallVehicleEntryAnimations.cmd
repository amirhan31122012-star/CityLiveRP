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
set "UE_ROOT=%~1"
if not defined UE_ROOT (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
)
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 не найден.
    pause
    exit /b 1
)
set "CITYLIVE_ENTRY_LOG=%PROJECT_DIR%Saved\VehicleEntry\%RANDOM%_%RANDOM%_%RANDOM%"
mkdir "%CITYLIVE_ENTRY_LOG%"
echo 1/2. Сборка кода посадки без изменения размера персонажа.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE > "%CITYLIVE_ENTRY_LOG%\Build.log" 2>&1
if errorlevel 1 goto failure
echo 2/2. Подготовка полноразмерных анимаций посадки и выхода.
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" /Engine/Maps/Entry "-ExecutePythonScript=%PROJECT_DIR%Scripts\PrepareVehicleEntryAnimations.py" "-abslog=%CITYLIVE_ENTRY_LOG%\Animation.log" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%CITYLIVE_ENTRY_LOG%\complete.ok" goto failure
echo Готово. Карта не переимпортируется. Теперь запустите BuildAndroidV2.cmd.
pause
exit /b 0
:failure
echo Подготовка не завершена. Пришлите Build.log или Animation.log из открывшейся папки.
start "" "%CITYLIVE_ENTRY_LOG%"
pause
exit /b 1
