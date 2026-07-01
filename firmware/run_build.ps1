$IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.3.1"
$PYTHON = "C:\Users\Demetrius Mendes\AppData\Local\Programs\Python\Python312\python.exe"

# Build PATH without WindowsApps to avoid MS Store stubs
$cleanPath = @()
foreach ($p in $env:Path -split ';') {
    if ($p -notmatch 'WindowsApps') { $cleanPath += $p }
}
$env:Path = $cleanPath -join ';'
$env:PATH = "C:\Users\Demetrius Mendes\AppData\Local\Programs\Python\Python312\;C:\Users\Demetrius Mendes\AppData\Local\Programs\Python\Python312\Scripts\;$env:PATH"

# Source IDF
. "$IDF_PATH\export.ps1"

# Build
Push-Location $PSScriptRoot
try {
    idf.py build
    if ($LASTEXITCODE -eq 0) { Write-Host "BUILD OK" -ForegroundColor Green }
    else { Write-Error "BUILD FAILED (code $LASTEXITCODE)"; exit 1 }
} finally { Pop-Location }
