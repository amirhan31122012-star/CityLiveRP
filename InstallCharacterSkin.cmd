@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
set "CITYLIVE_SKIN_TOKEN=%RANDOM%_%RANDOM%_%RANDOM%"
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Сначала закройте Unreal Engine.
    pause
    exit /b 1
)
if not exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" (
    echo Не найден Unreal Engine 5.8 в стандартной папке.
    pause
    exit /b 1
)
echo Собирается код персонажа. Дождитесь окончания.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE
if errorlevel 1 goto failure
echo Устанавливается парень в бежевом свитере. Файлы уже находятся в репозитории.
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" "-ExecutePythonScript=%PROJECT_DIR%Scripts\ImportCharacterSkin.py" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%PROJECT_DIR%Saved\CharacterImport\Import_%CITYLIVE_SKIN_TOKEN%.ok" goto failure
echo Готово. Откройте BuildAndOpen.cmd и нажмите Играть.
pause
exit /b 0
:failure
echo Установка не подтверждена. Пришлите Saved\Logs\CityLiveRP.log.
pause
exit /b 1
