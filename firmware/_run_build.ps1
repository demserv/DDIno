$py = 'C:\Users\6066180\.espressif\python_env\idf5.3_py3.14_env\Scripts'
$xtensa = 'C:\Users\6066180\.espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin'
$cmake = 'C:\Users\6066180\.espressif\tools\cmake\3.30.2\bin'
$ninja = 'C:\Users\6066180\.espressif\tools\ninja\1.12.1'
$idfExe = 'C:\Users\6066180\.espressif\tools\idf-exe\1.0.3'
$env:IDF_PATH = 'C:\esp\v5.3.2\esp-idf'
$env:IDF_TOOLS_PATH = 'C:\Users\6066180\.espressif'
$env:IDF_PYTHON_ENV_PATH = 'C:\Users\6066180\.espressif\python_env\idf5.3_py3.14_env'
$env:Path = "$py;$xtensa;$cmake;$ninja;$idfExe;" + $env:Path
Set-Location 'C:\Users\6066180\OneDrive - Thomson Reuters Incorporated\Desktop\DDIno\firmware'
$pyExe = Join-Path $py 'python.exe'
$idfPy = Join-Path $env:IDF_PATH 'tools\idf.py'
if (Test-Path 'build-test3\CMakeCache.txt') {
  & $pyExe $idfPy -B build-test3 build
} else {
  & $pyExe $idfPy build
}
exit $LASTEXITCODE
