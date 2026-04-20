param([string]$Path)
$text = Get-Content -Raw $Path
# UTF-16 LE with BOM
$enc = New-Object System.Text.UnicodeEncoding -ArgumentList $false, $true
[System.IO.File]::WriteAllText($Path, $text, $enc)
Write-Host "Re-encoded $Path as UTF-16 LE with BOM"
