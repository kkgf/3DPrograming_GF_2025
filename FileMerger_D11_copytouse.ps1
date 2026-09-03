# This ensures the script runs in the folder where it is saved
Set-Location $PSScriptRoot

$outputFile = "merged_output.txt"
$searchPaths = @(
    ".", 
    ".\Graphics",
	".\Source"
#    ".\src",           # 'src' folder inside current directory
#    "..\shared_libs"   # A folder sitting next to your current project folder
)

# Force-create/clear the file at the start to ensure it exists
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

foreach ($dir in $searchPaths) {
    if (Test-Path $dir) {
        # Using -Recurse here just in case your files are in subfolders
        $files = Get-ChildItem -Path $dir -Include *.h, *.cpp -File -Recurse
        
        foreach ($file in $files) {
            Append-File $file.FullName
            $fileCount++
        }
    } else {
        Write-Warning "Folder not found, skipping: $dir"
    }
}

if ($fileCount -gt 0) {
    Write-Host "`nDone! Successfully merged $fileCount files into $outputFile" -ForegroundColor Green
} else {
    Write-Error "No .h or .cpp files were found in the specified paths. $outputFile remains empty."
}

Read-Host "`nPress Enter to exit"