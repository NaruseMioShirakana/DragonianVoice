@echo off
setlocal enabledelayedexpansion

set models=hubert/hubert4.0.onnx models/march7/march7_RVC_40000.json models/march7/march7_RVC.onnx models/march7/march7_RVC_48000.json 

for %%i in (%models%) do (
    set model=%%i
    call :downloadModel
)

echo All models downloaded successfully.
exit /b

:downloadModel
echo Downloading hubert model !model!...

if exist "!model!" (
    echo Model !model! already exists. Skipping download.
    exit /b
)

rem Extract the directory path from the model variable
set "dirPath=!model!"
for %%A in ("!dirPath!") do set "dirPath=%%~dpA"

rem Create the directory if it doesn't exist
if not exist "!dirPath!" (
    mkdir "!dirPath!"
)

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri https://huggingface.co/datasets/qinzhu/moevoice_config/resolve/main/!model! -OutFile !model!"

if !ERRORLEVEL! neq 0 (
    echo Failed to download !model!
    echo Please try again later or download the original !model! files and convert them yourself.
    exit /b
)

exit /b

