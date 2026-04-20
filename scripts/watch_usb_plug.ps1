param([int]$Seconds = 120)
$start = Get-Date
$deadline = $start.AddSeconds($Seconds)
Write-Host "Watching USB/PnP channels until $deadline. Plug in the controller now."
Write-Host ""

$channels = @(
  'Microsoft-Windows-Kernel-PnP/Configuration',
  'Microsoft-Windows-Kernel-PnP/Device Management',
  'Microsoft-Windows-UserPnp/DeviceInstall',
  'Microsoft-Windows-USB-USBXHCI-Operational',
  'Microsoft-Windows-USB-UCMUCSICX/Operational'
)

$seen = @{}
while ((Get-Date) -lt $deadline) {
    foreach ($ch in $channels) {
        try {
            $events = Get-WinEvent -FilterHashtable @{LogName=$ch; StartTime=$start} -ErrorAction SilentlyContinue
        } catch { $events = $null }
        foreach ($ev in $events) {
            $key = "$ch|$($ev.RecordId)"
            if (-not $seen.ContainsKey($key)) {
                $seen[$key] = $true
                $msg = $ev.Message
                if ($msg.Length -gt 180) { $msg = $msg.Substring(0, 180) + '...' }
                $msg = $msg -replace "`r?`n", ' '
                Write-Host ("[{0:HH:mm:ss}] [{1}] Id={2} Level={3} {4}" -f $ev.TimeCreated, ($ch -replace 'Microsoft-Windows-',''), $ev.Id, $ev.LevelDisplayName, $msg)
            }
        }
    }
    Start-Sleep -Milliseconds 500
}

Write-Host ""
Write-Host "Done. Captured $($seen.Count) unique events."
