set model=hubert/hubert4.0.onnx

:download_model
echo Downloading hubert model %model%...


if exist "%model%" (
  echo Model %model% already exists. Skipping download.
  goto :eof
)

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri https://huggingface.co/datasets/qinzhu/moevoice_config/resolve/main/%model% -OutFile %model%"

if %ERRORLEVEL% neq 0 (
  echo Failed to download hubert4 model %model%
  echo Please try again later or download the original hubert4 model files and convert them yourself.
  goto :eof
)
