@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "UE_ROOT=%~1"
if not defined UE_ROOT set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Закройте Unreal Engine и снова запустите этот файл.
    pause
    exit /b 1
)
if not exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" (
    echo Не найден Unreal Engine. Укажите его папку первым аргументом.
    pause
    exit /b 1
)
echo Экспортируется только модель седана. Подождите окончания.
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" -run=pythonscript "-script=%PROJECT_DIR%Scripts\ExportSedanForDoors.py" -unattended -nosplash
if errorlevel 1 (
    echo Экспорт не удался. Пришлите сообщение об ошибке из этого окна.
    pause
    exit /b 1
)
echo Готово. Пришлите новый файл SedanForDoors из открывшейся папки.
start "" explorer.exe "%PROJECT_DIR%Saved\VehicleExport"
pause
