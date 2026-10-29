@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "BLENDER_EXE="
for /f "delims=" %%B in ('where blender.exe 2^>nul') do set "BLENDER_EXE=%%B"
if not defined BLENDER_EXE (
    for /d %%D in ("%ProgramFiles%\Blender Foundation\Blender *") do if exist "%%~D\blender.exe" set "BLENDER_EXE=%%~D\blender.exe"
)
if not defined BLENDER_EXE (
    echo Установите Blender с https://www.blender.org/download/ и снова откройте этот файл.
    pause
    exit /b 1
)
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Сначала закройте Unreal Engine.
    pause
    exit /b 1
)
set "SOURCE_FBX=%~f1"
if not defined SOURCE_FBX (
    for /f "delims=" %%F in ('dir /b /a-d /o-d "%PROJECT_DIR%Saved\VehicleExport\SedanForDoors_*.fbx" 2^>nul') do if not defined SOURCE_FBX set "SOURCE_FBX=%PROJECT_DIR%Saved\VehicleExport\%%F"
)
if not exist "%SOURCE_FBX%" (
    echo Исходный FBX не найден. Запустите ExportSedanDoors.cmd, затем снова этот файл.
    echo Если FBX сохранён в другом месте, перетащите его на PrepareSedanDoorsLocal.cmd.
    pause
    exit /b 1
)
set "OUTPUT_DIR=%PROJECT_DIR%Saved\VehicleDoors\%RANDOM%_%RANDOM%_%RANDOM%"
set "OUTPUT_FBX=%OUTPUT_DIR%\SK_CitySedan_4Doors.fbx"
echo Из исходного FBX создаются четыре подвижные двери. Дождитесь окончания.
"%BLENDER_EXE%" --background --python-exit-code 1 --python "%PROJECT_DIR%Scripts\PrepareSedanDoorsBlender.py" -- "%SOURCE_FBX%" "%OUTPUT_FBX%"
if errorlevel 1 goto failure
if not exist "%OUTPUT_FBX%" goto failure
call "%PROJECT_DIR%ImportSedanDoors.cmd" "%OUTPUT_FBX%"
exit /b %ERRORLEVEL%
:failure
echo Подготовка модели не завершена. Пришлите текст ошибки из этого окна.
pause
exit /b 1
