param(
    [string]$RootPath = (Get-Location).Path,
    [string]$OutputFile = $null
)

if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $RootPath "projeto_concatenado.txt"
}

$RootPath = [IO.Path]::GetFullPath($RootPath)
$OutputFile = [IO.Path]::GetFullPath($OutputFile)

Write-Host "Diretorio raiz: $RootPath"
Write-Host "Arquivo base: $OutputFile"
Write-Host "Iniciando varredura..."

$totalFiles = 0
$includedFiles = 0
$skippedErrors = 0

# Lista arquivos
$allFiles = Get-ChildItem -Path $RootPath -File -Recurse -Force | Sort-Object FullName
$totalFiles = $allFiles.Count

# Divide em 10 partes (CORRETO)
$parts = 10
if ($totalFiles -lt $parts) {
    $filesPerPart = 1
} else {
    $filesPerPart = [Math]::Ceiling($totalFiles / $parts)
}

# Diretório de saída
$outputDir = Split-Path -Parent $OutputFile
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# Loop
for ($part = 1; $part -le $parts; $part++) {

    $partFile = Join-Path $outputDir ("projeto_concatenado_part_{0:D2}_of_10.txt" -f $part)

    # Sobrescreve sempre
    if (Test-Path $partFile) {
        Remove-Item $partFile -Force
    }

    Write-Host "Gerando arquivo $partFile"

    $writer = New-Object IO.StreamWriter($partFile, $false, [Text.UTF8Encoding]::new($false))

    try {

        $startIndex = ($part - 1) * $filesPerPart
        $endIndex = $startIndex + $filesPerPart - 1

        if ($startIndex -ge $totalFiles) {
            $writer.Close()
            Write-Host "Parte $part vazia, ignorada."
            continue
        }

        if ($endIndex -ge $totalFiles) {
            $endIndex = $totalFiles - 1
        }

        $writer.WriteLine("***************************************INICIO DO ARQUIVO $part/10********************************")

        for ($i = $startIndex; $i -le $endIndex; $i++) {

            $file = $allFiles[$i]

            try {
                $content = Get-Content -Path $file.FullName -Raw -ErrorAction Stop

                $writer.WriteLine("")
                $writer.WriteLine("===== FILE: $($file.FullName) =====")
                $writer.WriteLine($content)

                $includedFiles++
            }
            catch {
                $skippedErrors++
            }
        }

        $writer.WriteLine("***********************************FINAL DO ARQUIVO $part/10*************************")
    }
    finally {
        $writer.Close()
    }

    Write-Host "Arquivo $part/10 gerado com sucesso"
}

Write-Host ""
Write-Host "Concluido."
Write-Host "Total arquivos: $totalFiles"
Write-Host "Incluidos: $includedFiles"
Write-Host "Erros: $skippedErrors"
Write-Host "Arquivos gerados em: $outputDir"