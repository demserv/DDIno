# ESP-IDF setup script for DDIno firmware
$IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.3.1"
$IDF_INSTALL = "$IDF_PATH\install.ps1"

# 1. Check Python
$python = Get-Command python -ErrorAction SilentlyContinue
if (!$python) {
    Write-Host "Python 3.10+ não encontrado."
    Write-Host "Baixe e instale de: https://www.python.org/downloads/"
    Write-Host "Na instalação, marque 'Add Python to PATH'"
    exit 1
}
Write-Host "Python: $($python.Source)"

# 2. Check IDF
if (!(Test-Path -LiteralPath $IDF_PATH)) {
    Write-Error "ESP-IDF não encontrado em $IDF_PATH"
    Write-Host "Clone o IDF:"
    Write-Host "  git clone --branch v5.3.1 https://github.com/espressif/esp-idf.git $IDF_PATH"
    exit 1
}

# 3. Run IDF install (downloads toolchain)
if (Test-Path -LiteralPath $IDF_INSTALL) {
    Write-Host "Instalando ESP-IDF toolchain (isto pode levar vários minutos)..."
    pushd $IDF_PATH
    try {
        powershell -ExecutionPolicy Bypass -File $IDF_INSTALL
        if ($?) { Write-Host "Toolchain instalado com sucesso" }
    } finally { popd }
}

Write-Host ""
Write-Host "Setup concluído. Para compilar:"
Write-Host "  cd firmware"
Write-Host "  .\build.ps1"
