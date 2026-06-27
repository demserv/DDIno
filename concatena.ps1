param(
    [string]$RootPath = (Get-Location).Path,
    [string]$OutputFile = $null,
    [int]$Parts = 10
)

# Definir arquivo padrão
if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $RootPath "projeto_concatenado.txt"
}

$RootPath   = [IO.Path]::GetFullPath($RootPath)
$OutputFile = [IO.Path]::GetFullPath($OutputFile)

Write-Host "Diretorio raiz: $RootPath"
Write-Host "Arquivo base: $OutputFile"
Write-Host "Partes: $Parts"
Write-Host "Iniciando varredura..."

$totalFiles    = 0
$includedFiles = 0
$skippedErrors = 0

# ================================
# LISTAR ARQUIVOS
# ================================
$ExcludePaths = @(
    "*managed_components\lvgl__lvgl\docs\*",
    "*managed_components\lvgl__lvgl\examples\*",
    "*managed_components\lvgl__lvgl\demos\*",
    "*managed_components\lvgl__lvgl\tests\*"
)

$allFiles = Get-ChildItem -Path $RootPath -File -Recurse -Force |
            Where-Object {
                $path = $_.FullName
                -not ($ExcludePaths | Where-Object { $path -like $_ })
            } |
            Sort-Object FullName

$totalFiles = $allFiles.Count

if ($totalFiles -eq 0) {
    Write-Host "Nenhum arquivo encontrado."
    exit
}

# ================================
# CALCULAR DIVISÃO
# ================================
if ($totalFiles -lt $Parts) {
    $filesPerPart = 1
} else {
    $filesPerPart = [Math]::Ceiling($totalFiles / $Parts)
}

Write-Host "Total de arquivos: $totalFiles"
Write-Host "Arquivos por parte: $filesPerPart"

# ================================
# DIRETÓRIO DE SAÍDA
# ================================
$outputDir = Split-Path -Parent $OutputFile
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# ================================
# LOOP DE GERAÇÃO
# ================================
for ($part = 1; $part -le $Parts; $part++) {

    $startIndex = ($part - 1) * $filesPerPart
    $endIndex   = [Math]::Min($startIndex + $filesPerPart - 1, $totalFiles - 1)

    if ($startIndex -ge $totalFiles) {
        break
    }

    $partFile = [IO.Path]::Combine(
        $outputDir,
        ("{0}_parte_{1}.txt" -f [IO.Path]::GetFileNameWithoutExtension($OutputFile), $part)
    )

    Write-Host ""
    Write-Host "Gerando parte $part -> $partFile"

    # Limpa arquivo se existir
    if (Test-Path $partFile) {
        Remove-Item $partFile
    }

    for ($i = $startIndex; $i -le $endIndex; $i++) {

        $file = $allFiles[$i]

        try {
            Add-Content -Path $partFile -Value ("`n============================")
            Add-Content -Path $partFile -Value ("FILE: " + $file.FullName)
            Add-Content -Path $partFile -Value ("============================`n")

            Get-Content -Path $file.FullName -ErrorAction Stop |
                Add-Content -Path $partFile

            $includedFiles++

        } catch {
            Write-Warning "Erro ao ler: $($file.FullName)"
            $skippedErrors++
        }
    }
}

# ================================
# FINAL
# ================================
Write-Host ""
Write-Host "======================================"
Write-Host "Concluido."
Write-Host "Total arquivos: $totalFiles"
Write-Host "Incluidos: $includedFiles"
Write-Host "Erros: $skippedErrors"
Write-Host "Arquivos gerados em: $outputDir"
Write-Host "======================================"