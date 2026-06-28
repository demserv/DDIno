# ============================================================
# concatena.ps1  (v4 - SEM FILTROS)
# Concatena TODOS os arquivos do diretorio (recursivo) em N
# partes (default = 10). Nao aplica nenhum filtro de extensao
# ou pasta. Apenas o proprio arquivo de saida e ignorado.
#
# Uso:
#   .\concatena.ps1
#   .\concatena.ps1 -Parts 10
#   .\concatena.ps1 -RootPath "C:\proj" -Parts 8
#
# Caracteristicas:
#   - StreamWriter (resolve "fluxo nao era legivel")
#   - FileShare.ReadWrite (resolve conflito com OneDrive)
#   - UTF-8 sem BOM
#   - Delimitadores solidos de inicio/fim por arquivo
#   - Cabecalho por parte com metadata
#   - Diagnostico no final
# ============================================================

param(
    [string]$RootPath   = (Get-Location).Path,
    [string]$OutputFile = $null,
    [int]   $Parts      = 10
)

# ------------------------------------------------------------
# Definir arquivo padrao
# ------------------------------------------------------------
if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $RootPath "projeto_concatenado.txt"
}

$RootPath   = [IO.Path]::GetFullPath($RootPath)
$OutputFile = [IO.Path]::GetFullPath($OutputFile)

Write-Host "============================================================"
Write-Host " Diretorio raiz : $RootPath"
Write-Host " Arquivo base   : $OutputFile"
Write-Host " Partes         : $Parts"
Write-Host " Filtros        : NENHUM (todos os arquivos serao incluidos)"
Write-Host "============================================================"
Write-Host "Iniciando varredura..."

# ------------------------------------------------------------
# LISTAR TODOS OS ARQUIVOS (sem filtros)
# Apenas excluindo o proprio arquivo de saida para evitar loop
# ------------------------------------------------------------
$allFiles = Get-ChildItem -Path $RootPath -File -Recurse -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName.ToLower() -notlike "*projeto_concatenado*" } |
    Sort-Object FullName

$totalFiles = $allFiles.Count

Write-Host ""
Write-Host (" Total de arquivos encontrados: {0}" -f $totalFiles) -ForegroundColor Green
Write-Host ""

if ($totalFiles -eq 0) {
    Write-Host "Nenhum arquivo encontrado em $RootPath"
    exit
}

# ------------------------------------------------------------
# CALCULAR DIVISAO
# ------------------------------------------------------------
if ($totalFiles -lt $Parts) {
    $Parts = $totalFiles
    $filesPerPart = 1
} else {
    $filesPerPart = [Math]::Ceiling($totalFiles / $Parts)
}

Write-Host (" Arquivos por parte (aprox.): {0}" -f $filesPerPart)
Write-Host ""

# ------------------------------------------------------------
# DIRETORIO DE SAIDA
# ------------------------------------------------------------
$outputDir  = Split-Path -Parent $OutputFile
$outputBase = [IO.Path]::GetFileNameWithoutExtension($OutputFile)
$outputExt  = [IO.Path]::GetExtension($OutputFile)
if ([string]::IsNullOrWhiteSpace($outputExt)) { $outputExt = ".txt" }

if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# UTF-8 sem BOM
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$totalIncluded = 0
$totalErrors   = 0
$totalBytes    = 0

# ------------------------------------------------------------
# LOOP DE GERACAO
# ------------------------------------------------------------
for ($part = 1; $part -le $Parts; $part++) {

    $startIndex = ($part - 1) * $filesPerPart
    $endIndex   = [Math]::Min($startIndex + $filesPerPart - 1, $totalFiles - 1)

    if ($startIndex -ge $totalFiles) { break }

    $partName = "{0}_parte_{1:D2}{2}" -f $outputBase, $part, $outputExt
    $partPath = Join-Path $outputDir $partName

    Write-Host "------------------------------------------------------------"
    Write-Host (" Gerando parte {0}/{1}: {2}" -f $part, $Parts, $partName)
    Write-Host (" Arquivos {0} a {1} (de {2})" -f ($startIndex+1), ($endIndex+1), $totalFiles)

    $partIncluded = 0
    $partChars    = 0

    $writer = New-Object System.IO.StreamWriter($partPath, $false, $utf8NoBom)

    try {
        # ===== Cabecalho da parte =====
        $writer.WriteLine("============================================================")
        $writer.WriteLine(" PARTE $part / $Parts")
        $writer.WriteLine(" Gerado em      : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
        $writer.WriteLine(" Diretorio raiz : $RootPath")
        $writer.WriteLine(" Faixa          : arquivos $($startIndex+1) a $($endIndex+1) de $totalFiles")
        $writer.WriteLine("============================================================")
        $writer.WriteLine("")

        for ($i = $startIndex; $i -le $endIndex; $i++) {
            $file = $allFiles[$i]
            $relative = $file.FullName.Substring($RootPath.Length).TrimStart('\','/')

            try {
                # Leitura robusta (FileShare.ReadWrite -> compativel com OneDrive)
                $fs = [System.IO.File]::Open(
                    $file.FullName,
                    [System.IO.FileMode]::Open,
                    [System.IO.FileAccess]::Read,
                    [System.IO.FileShare]::ReadWrite
                )
                $reader  = New-Object System.IO.StreamReader($fs)
                $content = $reader.ReadToEnd()
                $reader.Close()
                $fs.Close()

                # ===== Delimitador de INICIO de arquivo =====
                $writer.WriteLine("")
                $writer.WriteLine("============================================================")
                $writer.WriteLine(">>>>> BEGIN FILE")
                $writer.WriteLine(" FILE     : $relative")
                $writer.WriteLine(" SIZE     : $($file.Length) bytes")
                $writer.WriteLine(" MODIFIED : $($file.LastWriteTime)")
                $writer.WriteLine("============================================================")
                $writer.Write($content)
                if (-not $content.EndsWith("`n")) { $writer.WriteLine("") }
                # ===== Delimitador de FIM de arquivo =====
                $writer.WriteLine("============================================================")
                $writer.WriteLine("<<<<< END FILE: $relative")
                $writer.WriteLine("============================================================")

                $partIncluded++
                $totalIncluded++
                $partChars += $content.Length
                $totalBytes += $file.Length
            }
            catch {
                $totalErrors++
                Write-Host ("   [ERRO] {0} -> {1}" -f $relative, $_.Exception.Message) -ForegroundColor Yellow

                # registra o erro DENTRO do arquivo de saida para nao perder o registro
                $writer.WriteLine("")
                $writer.WriteLine("============================================================")
                $writer.WriteLine(">>>>> BEGIN FILE (ERRO DE LEITURA)")
                $writer.WriteLine(" FILE  : $relative")
                $writer.WriteLine(" ERROR : $($_.Exception.Message)")
                $writer.WriteLine("============================================================")
                $writer.WriteLine("<<<<< END FILE: $relative")
                $writer.WriteLine("============================================================")
            }
        }
    }
    finally {
        $writer.Flush()
        $writer.Close()
        $writer.Dispose()
    }

    $tokensApprox = [Math]::Round($partChars / 4)
    Write-Host (" -> {0} arquivos | ~{1} chars | ~{2} tokens" -f $partIncluded, $partChars, $tokensApprox)
}

# ------------------------------------------------------------
# FINAL
# ------------------------------------------------------------
$totalMB = [Math]::Round($totalBytes / 1MB, 2)

Write-Host ""
Write-Host "============================================================"
Write-Host " Concluido."
Write-Host (" Total arquivos encontrados : {0}" -f $totalFiles)
Write-Host (" Incluidos com sucesso      : {0}" -f $totalIncluded)
Write-Host (" Erros de leitura           : {0}" -f $totalErrors)
Write-Host (" Volume total processado    : {0} MB" -f $totalMB)
Write-Host (" Saida em                   : {0}" -f $outputDir)
Write-Host "============================================================"
