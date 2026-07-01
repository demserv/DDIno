# Build script for DDIno firmware (ESP-IDF)
$IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.3.1"
$IDF_EXPORT = "$IDF_PATH\export.ps1"

if (!(Test-Path -LiteralPath $IDF_EXPORT)) {
    Write-Error "ESP-IDF não encontrado em $IDF_PATH"
    exit 1
}

# Check Python
$python = Get-Command python -ErrorAction SilentlyContinue
if (!$python) {
    Write-Error "Python não encontrado. Instale Python 3.10+ em python.org"
    exit 1
}

# Source IDF environment and build
pushd $PSScriptRoot
try {
    $env:IDF_PATH = $IDF_PATH
    & "$IDF_PATH\export.ps1"
    idf.py build
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build concluído com sucesso"
    } else {
        Write-Error "Build falhou (código $LASTEXITCODE)"
        exit 1
    }
} finally {
    popd
}
