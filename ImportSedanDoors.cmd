@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "CITYLIVE_DOORS_FBX=%~f1"
set "CITYLIVE_IMPORT_TOKEN=%RANDOM%_%RANDOM%_%RANDOM%"
set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if "%~1"=="" (
    call :choose_fbx
    if errorlevel 1 exit /b 0
)
if not exist "%CITYLIVE_DOORS_FBX%" (
    echo Файл FBX не найден.
    pause
    exit /b 1
)
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Сначала закройте Unreal Engine и повторите импорт.
    pause
    exit /b 1
)
if not exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" (
    echo Unreal Engine 5.8 не найден в стандартной папке установки.
    pause
    exit /b 1
)
echo Сначала собирается актуальный код проекта. Дождитесь окончания.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE
if errorlevel 1 goto failure
echo Импортируется модель с четырьмя дверями. Дождитесь окончания.
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" "-ExecutePythonScript=%PROJECT_DIR%Scripts\ImportSedanDoors.py" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%PROJECT_DIR%Saved\VehicleImport\Import_%CITYLIVE_IMPORT_TOKEN%.ok" goto failure
echo Готово. Запустите BuildAndOpen.cmd и проверьте двери.
pause
exit /b 0
:failure
echo Импорт не подтверждён. Пришлите Saved\Logs\CityLiveRP.log.
pause
exit /b 1

:choose_fbx
echo Выберите скачанный SK_CitySedan_4Doors_v2.fbx в открывшемся окне.
set "PICKER_RESULT=%TEMP%\CityLiveSedan_%CITYLIVE_IMPORT_TOKEN%.txt"
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%PROJECT_DIR%Scripts\SelectSedanDoors.ps1" -OutputPath "%PICKER_RESULT%"
if errorlevel 1 exit /b 1
if not exist "%PICKER_RESULT%" exit /b 1
set /p "CITYLIVE_DOORS_FBX="<"%PICKER_RESULT%"
del "%PICKER_RESULT%"
exit /b 0
