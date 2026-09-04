# This ensures the script runs in the folder where it is saved
Set-Location $PSScriptRoot

$outputFile = "merged_output.txt"

# Directories whose subdirectories should also be searched
$searchPaths = @(
    ".\Graphics",
    ".\Source"
)

# Force-create/clear the output file
New-Item -Path $outputFile -ItemType File -Force | Out-Null
Write-Host "Created/Cleared: $outputFile" -ForegroundColor Gray

function Append-File($path) {
    $fileName = Split-Path $path -Leaf
    Write-Host "Appending: $fileName" -ForegroundColor Yellow

    $header = @"

/* ===================================================
   FILE: $path
   =================================================== */

"@

    Add-Content -Path $outputFile -Value $header
    Get-Content -Path $path | Add-Content -Path $outputFile
}

$fileCount = 0

# -------------------------------------------------------
# 1. Files directly in the current directory
#    NO -Recurse, so subdirectories are ignored
# -------------------------------------------------------
$rootFiles = Get-ChildItem -Path "." -Include *.h, *.cpp -File

foreach ($file in $rootFiles) {
    Append-File $file.FullName
    $fileCount++
}

# -------------------------------------------------------
# 2. Files in specified directories and their subfolders
# -------------------------------------------------------
foreach ($dir in $searchPaths) {
    if (Test-Path $dir) {
        $files = Get-ChildItem -Path $dir -Include *.h, *.cpp -File -Recurse

        foreach ($file in $files) {
            Append-File $file.FullName
            $fileCount++
        }
    }
    else {
        Write-Warning "Folder not found, skipping: $dir"
    }
}

if ($fileCount -gt 0) {
    Write-Host "`nDone! Successfully merged $fileCount files into $outputFile" -ForegroundColor Green
}
else {
    Write-Error "No .h or .cpp files were found."
}

Read-Host "`nPress Enter to exit"