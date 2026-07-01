param(
    [string]$RootPath = $PSScriptRoot,
    [int]$MaxMegabytes = 20
)

$ErrorActionPreference = "Stop"

Write-Host "Iniciando..."
Write-Host ("RootPath: {0}" -f $RootPath)

# Configurações
$maxBytes = $MaxMegabytes * 1024 * 1024
$encoding = [System.Text.Encoding]::UTF8

# Verifica RootPath existe
if (-not (Test-Path -Path $RootPath)) {
    Write-Host "ERRO: RootPath não encontrado: $RootPath"
    exit 1
}

# Lista arquivos (todos os arquivos do diretório do script e subdiretórios)
$files = Get-ChildItem -Path $RootPath -File -Recurse -ErrorAction Stop | Sort-Object FullName
$totalFiles = $files.Count
$totalBytesAllFiles = 0
foreach ($f in $files) { $totalBytesAllFiles += (Get-Item $f.FullName).Length }

Write-Host "Total arquivos encontrados: $totalFiles"
Write-Host ("Tamanho total (bytes): {0}" -f $totalBytesAllFiles)
Write-Host ("Tamanho total (MB aprox): {0:N2}" -f ($totalBytesAllFiles / 1MB))

if ($totalFiles -eq 0) {
    Write-Host "Nenhum arquivo encontrado."
    exit 0
}

# Diretório de saída (mesmo diretório do script)
$OutputDir = $PSScriptRoot
Write-Host ("Arquivos de saída serão gravados em: {0}" -f $OutputDir)

# Remove arquivos antigos que seguem o padrão
Get-ChildItem -Path $OutputDir -Filter "projeto_concatenado_part_*.txt" -File -ErrorAction SilentlyContinue | ForEach-Object {
    try { Remove-Item $_.FullName -Force -ErrorAction Stop; Write-Host ("Removido antigo: {0}" -f $_.Name) }
    catch { Write-Host ("Falha ao remover {0}: {1}" -f $_.FullName, $_.Exception.Message) }
}

# Funções para iniciar e finalizar parte
function Start-Part {
    param($partNumber)
    $script:currentPartNumber = $partNumber
    $script:currentFileName = Join-Path $OutputDir ("projeto_concatenado_part_{0:D3}.txt" -f $partNumber)
    $header = "***************************************INICIO DO ARQUIVO $partNumber********************************`n"
    try {
        [System.IO.File]::WriteAllText($script:currentFileName, $header, $encoding)
        $script:currentSize = $encoding.GetByteCount($header)
        Write-Host ("Gerando {0}" -f $script:currentFileName)
    }
    catch {
        Write-Host ("ERRO ao criar {0}: {1}" -f $script:currentFileName, $_.Exception.Message)
        throw
    }
}

function Finish-Part {
    if (-not $script:currentFileName) { return }
    $footer = "`n***********************************FINAL DO ARQUIVO $script:currentPartNumber*************************`n"
    try {
        [System.IO.File]::AppendAllText($script:currentFileName, $footer, $encoding)
        $script:currentSize += $encoding.GetByteCount($footer)
        Write-Host ("Parte {0} concluida (tamanho aproximado: {1} bytes)" -f $script:currentPartNumber, $script:currentSize)
    }
    catch {
        Write-Host ("ERRO ao finalizar {0}: {1}" -f $script:currentFileName, $_.Exception.Message)
        throw
    }
}

# Inicializa primeira parte
$partNumber = 1
$currentFileName = $null
$currentSize = 0
Start-Part -partNumber $partNumber

# Contadores de progresso
$processedCount = 0
$processedBytes = 0

# Loop principal
foreach ($f in $files) {

    # Lê conteúdo do arquivo (tratamento de erro)
    try {
        $content = Get-Content $f.FullName -Raw -ErrorAction Stop
    }
    catch {
        $errLine = "`nERRO AO LER: $($f.FullName)`n"
        try {
            [System.IO.File]::AppendAllText($currentFileName, $errLine, $encoding)
            $currentSize += $encoding.GetByteCount($errLine)
        } catch {}
        Write-Host ("ERRO AO LER: {0}" -f $f.FullName)
        continue
    }

    # Monta entrada com indicador do arquivo (nome e path)
    $entryHeader = "`n===== FILE: $($f.FullName) =====`n"
    $entryFooter = "`n===== END FILE: $($f.FullName) =====`n"
    $entry = $entryHeader + $content + $entryFooter
    $entryBytes = $encoding.GetByteCount($entry)

    # Se a adição deste arquivo ultrapassaria o limite, finalize a parte e comece outra
    if (($currentSize -gt 0) -and (($currentSize + $entryBytes) -gt $maxBytes)) {
        Finish-Part
        $partNumber++
        Start-Part -partNumber $partNumber
    }

    # Anexa o arquivo inteiro à parte atual
    try {
        [System.IO.File]::AppendAllText($currentFileName, $entry, $encoding)
        $currentSize += $entryBytes
    }
    catch {
        Write-Host ("ERRO ao escrever em {0}: {1}" -f $currentFileName, $_.Exception.Message)
        throw
    }

    # Atualiza contadores
    $processedCount++
    $processedBytes += (Get-Item $f.FullName).Length

    # Mensagem de progresso
    Write-Host ("Processado {0}/{1} : {2} (tamanho do arquivo: {3} bytes)" -f $processedCount, $totalFiles, $f.FullName, (Get-Item $f.FullName).Length)
}

# Finaliza a última parte
Finish-Part

# Resumo final
Write-Host "----------------------------------------"
Write-Host ("Total arquivos processados: {0}" -f $processedCount)
Write-Host ("Tamanho total processado (bytes): {0}" -f $processedBytes)
Write-Host ("Tamanho total processado (MB aprox): {0:N2}" -f ($processedBytes / 1MB))
Write-Host "Finalizado."
