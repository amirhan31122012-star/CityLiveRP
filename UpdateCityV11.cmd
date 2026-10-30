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
set "CITYLIVE_UPDATE_TOKEN=%RANDOM%_%RANDOM%_%RANDOM%"
set "UPDATE_LOG_DIR=%PROJECT_DIR%Saved\CityUpdateV11\%CITYLIVE_UPDATE_TOKEN%"
mkdir "%UPDATE_LOG_DIR%"
echo Сборка обновлённого кода перед обновлением карты.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE > "%UPDATE_LOG_DIR%\Build.log" 2>&1
if errorlevel 1 goto failure
echo Обновляется активная карта: удаляются декоративные машины и заменяется небо.
echo Сохраняется копия карты CityLive_BeforeV11. Повторного импорта Blender нет.
echo Дождитесь окончания. Журнал: %UPDATE_LOG_DIR%\Update.log
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" /Engine/Maps/Entry "-ExecutePythonScript=%PROJECT_DIR%Scripts\UpdateCityV11.py" "-abslog=%UPDATE_LOG_DIR%\Update.log" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%UPDATE_LOG_DIR%\complete.ok" goto failure
echo Готово. Проверьте игру через BuildAndOpen.cmd, затем соберите APK через BuildAndroidV1.cmd.
pause
exit /b 0
:failure
echo Обновление остановлено. Пришлите Build.log или Update.log из открывающейся папки.
start "" "%UPDATE_LOG_DIR%"
pause
exit /b 1
