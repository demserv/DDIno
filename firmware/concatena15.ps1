param(
    [string]$RootPath = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host "Iniciando..."

$parts = 15

# Lista arquivos
$files = Get-ChildItem -Path $RootPath -File -Recurse | Sort-Object FullName
$total = $files.Count

Write-Host "Total arquivos: $total"

if ($total -eq 0) {
    Write-Host "Nenhum arquivo encontrado."
    exit
}

# Calcula quantidade por parte (SEM Math complexo)
$filesPerPart = $total / $parts

if ($filesPerPart -eq 0) {
    $filesPerPart = 1
}

# Remove arquivos antigos
for ($i = 1; $i -le $parts; $i++) {
    $name = "projeto_concatenado_part_{0:D2}_of_15.txt" -f $i
    if (Test-Path $name) {
        Remove-Item $name -Force
    }
}

# Loop principal
for ($part = 1; $part -le $parts; $part++) {

    $fileName = "projeto_concatenado_part_{0:D2}_of_15.txt" -f $part
    Write-Host "Gerando $fileName"

    $start = ($part - 1) * $filesPerPart
    $end = $start + $filesPerPart - 1

    if ($end -ge $total) {
        $end = $total - 1
    }

    Add-Content $fileName "***************************************INICIO DO ARQUIVO $part/15********************************"

    for ($i = $start; $i -le $end; $i++) {

        if ($i -ge $total) { break }

        $f = $files[$i]

        try {
            $content = Get-Content $f.FullName -Raw -ErrorAction Stop

            Add-Content $fileName ""
            Add-Content $fileName "===== FILE: $($f.FullName) ====="
            Add-Content $fileName $content
        }
        catch {
            Add-Content $fileName "ERRO AO LER: $($f.FullName)"
        }
    }

    Add-Content $fileName "***********************************FINAL DO ARQUIVO $part/15*************************"

    Write-Host "Parte $part concluida"
}

Write-Host "Finalizado."