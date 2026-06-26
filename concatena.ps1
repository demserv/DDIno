param(
    [string]$RootPath = (Get-Location).Path,
    [string]$OutputFile = $null
)

if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $RootPath "projeto_concatenado.txt"
}

$RootPath = [System.IO.Path]::GetFullPath($RootPath)
$OutputFile = [System.IO.Path]::GetFullPath($OutputFile)

Write-Host "Diretorio raiz: $RootPath"
Write-Host "Arquivo de saida: $OutputFile"
Write-Host "Iniciando varredura..."

function Test-IsBinaryFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $sampleSize = 8192

    try {
        $fs = [System.IO.File]::OpenRead($Path)

        try {
            $buffer = New-Object byte[] $sampleSize
            $bytesRead = $fs.Read($buffer, 0, $sampleSize)

            if ($bytesRead -eq 0) {
                return $false
            }

            for ($i = 0; $i -lt $bytesRead; $i++) {
                if ($buffer[$i] -eq 0) {
                    return $true
                }
            }

            return $false
        }
        finally {
            $fs.Close()
        }
    }
    catch {
        return $true
    }
}

$totalFiles = 0
$includedFiles = 0
$skippedBinaryFiles = 0
$skippedErrors = 0

# Garante que a pasta de saída exista
$outputDir = Split-Path -Parent $OutputFile
if (-not [string]::IsNullOrWhiteSpace($outputDir) -and -not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

# Remove arquivo anterior, se existir
if (Test-Path $OutputFile) {
    Remove-Item $OutputFile -Force
}

# Cria StreamWriter UTF-8 sem BOM
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$writer = New-Object System.IO.StreamWriter($OutputFile, $false, $utf8NoBom)

try {
    Get-ChildItem -Path $RootPath -File -Recurse -Force | Sort-Object FullName | ForEach-Object {
        $file = $_
        $filePath = [System.IO.Path]::GetFullPath($file.FullName)

        # Evita incluir o próprio arquivo de saída
        if ($filePath -eq $OutputFile) {
            return
        }

        $script:totalFiles++

        try {
            if (Test-IsBinaryFile -Path $filePath) {
                $script:skippedBinaryFiles++
                return
            }

            $writer.WriteLine("***************Inicio do arquivo -$filePath*****************.")

            try {
                $reader = New-Object System.IO.StreamReader($filePath, $true)

                try {
                    while (($line = $reader.ReadLine()) -ne $null) {
                        $writer.WriteLine($line)
                    }
                }
                finally {
                    $reader.Close()
                }
            }
            catch {
                # Segunda tentativa com UTF-8 replacement fallback
                $encoding = New-Object System.Text.UTF8Encoding($false, $false)
                $reader = New-Object System.IO.StreamReader($filePath, $encoding, $true)

                try {
                    while (($line = $reader.ReadLine()) -ne $null) {
                        $writer.WriteLine($line)
                    }
                }
                finally {
                    $reader.Close()
                }
            }

            $writer.WriteLine("Fim do arquivo -$filePath******************")
            $writer.WriteLine("")

            $script:includedFiles++
        }
        catch {
            $script:skippedErrors++
            Write-Warning "Erro ao processar arquivo: $filePath | Motivo: $($_.Exception.Message)"
        }
    }
}
finally {
    $writer.Close()
}

Write-Host ""
Write-Host "Concluido."
Write-Host "Total de arquivos encontrados: $totalFiles"
Write-Host "Arquivos concatenados: $includedFiles"
Write-Host "Arquivos binarios ignorados: $skippedBinaryFiles"
Write-Host "Arquivos ignorados por erro: $skippedErrors"
Write-Host "TXT gerado em: $OutputFile"