# concatena_auditoria.ps1 — PowerShell script for compliance audit concatenation
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File concatena_auditoria.ps1 [-OutputFile <path>] [-SrcRoot <path>]
#
# Description:
#   Concatenates all project-authored source files into a single text file
#   for compliance audit review. Excludes third-party code (managed_components/),
#   build artifacts, and binary files.
#
# Defaults:
#   - OutputFile: "$PSScriptRoot/../audit_concat.txt" (relative to script location)
#   - SrcRoot: "$PSScriptRoot/.." (parent of tools/)

param(
    [string]$OutputFile = "",
    [string]$SrcRoot = ""
)

if (-not $OutputFile) {
    $OutputFile = Join-Path -Path $PSScriptRoot -ChildPath "..\audit_concat.txt"
}
$OutputFile = (Resolve-Path -Path $OutputFile -ErrorAction SilentlyContinue).Path
if (-not $OutputFile) {
    $OutputFile = Join-Path -Path $PSScriptRoot -ChildPath "..\audit_concat.txt"
}

if (-not $SrcRoot) {
    $SrcRoot = Resolve-Path -Path (Join-Path -Path $PSScriptRoot -ChildPath "..")
} else {
    $SrcRoot = Resolve-Path -Path $SrcRoot
}

Write-Host "=== Concatenador de Auditoria ==="
Write-Host "SrcRoot:   $SrcRoot"
Write-Host "Output:    $OutputFile"

# --- File extensions to include ---
$includeExts = @(".c", ".h", ".md", ".txt", ".yml", ".yaml", ".json", ".cfg",
                 ".csv", ".sh", ".ps1", ".py", ".cmake", "CMakeLists.txt",
                 "Kconfig", ".defaults", "partitions.csv")

# --- Directories to exclude entirely ---
$excludeDirs = @(
    "managed_components",
    "build",
    ".git",
    "node_modules",
    "__pycache__",
    ".vscode",
    ".idea"
)

# --- Collect files ---
$files = Get-ChildItem -Path $SrcRoot -Recurse -File | Where-Object {
    $include = $false
    foreach ($ext in $includeExts) {
        if ($_.Name -eq $ext -or $_.Extension -eq $ext -or $_.Name -like "*$ext") {
            $include = $true
            break
        }
    }
    if (-not $include) { return $false }

    # Exclude directories
    foreach ($excl in $excludeDirs) {
        if ($_.FullName -match "\\$excl\\") {
            return $false
        }
    }

    # Exclude the output file itself
    if ($_.FullName -eq $OutputFile) { return $false }

    # Exclude the script itself (defensive)
    if ($_.FullName -eq $MyInvocation.MyCommand.Path) { return $false }

    return $true
} | Sort-Object FullName

$totalLines = 0
$totalFiles = 0

# Clear output file
"" | Out-File -FilePath $OutputFile -Encoding UTF8

foreach ($file in $files) {
    $relPath = [System.IO.Path]::GetRelativePath($SrcRoot, $file.FullName)
    $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
    if (-not $content) {
        Write-Warning "    SKIP (unreadable): $relPath"
        continue
    }

    $lineCount = ($content -split "`r`n|`n" | Measure-Object -Line).Lines

    $separator = @"

"@"="*60
FILE: $relPath
LINES: $lineCount
"="*60

"@
    $separator | Out-File -FilePath $OutputFile -Encoding UTF8 -Append
    $content | Out-File -FilePath $OutputFile -Encoding UTF8 -Append

    $totalFiles++
    $totalLines += $lineCount
    Write-Host "    ADDED: $relPath ($lineCount lines)"
}

Write-Host "=== Concluido ==="
Write-Host "Arquivos: $totalFiles"
Write-Host "Linhas:   $totalLines"
Write-Host "Output:   $OutputFile"
