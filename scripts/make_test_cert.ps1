#Requires -RunAsAdministrator
$ErrorActionPreference = 'Stop'

$subject = "CN=SwitchProDriver Dev Cert"
$store   = "Cert:\LocalMachine\My"

$existing = Get-ChildItem $store | Where-Object { $_.Subject -eq $subject }
if ($existing) {
    Write-Host "Test cert already present: $($existing.Thumbprint)"
    $cert = $existing | Select-Object -First 1
} else {
    Write-Host "Creating self-signed code-signing cert: $subject"
    $cert = New-SelfSignedCertificate `
        -Subject $subject `
        -Type CodeSigningCert `
        -KeyUsage DigitalSignature `
        -KeyExportPolicy Exportable `
        -HashAlgorithm SHA256 `
        -KeyLength 2048 `
        -NotAfter (Get-Date).AddYears(5) `
        -CertStoreLocation $store
    Write-Host "Created: $($cert.Thumbprint)"
}

# Copy to Root and TrustedPublisher so the OS trusts its signatures.
foreach ($dest in @("Cert:\LocalMachine\Root", "Cert:\LocalMachine\TrustedPublisher")) {
    $existing = Get-ChildItem $dest | Where-Object { $_.Thumbprint -eq $cert.Thumbprint }
    if (-not $existing) {
        $store = Get-Item $dest
        $destStore = New-Object System.Security.Cryptography.X509Certificates.X509Store($store.Name, "LocalMachine")
        $destStore.Open("ReadWrite")
        $destStore.Add($cert)
        $destStore.Close()
        Write-Host "Installed $($cert.Thumbprint) into $dest"
    }
}

Write-Host "`nThumbprint: $($cert.Thumbprint)"
$outPath = "c:\git\SwitchProDriver\build\driver\cert_thumbprint.txt"
$cert.Thumbprint | Out-File -FilePath $outPath -Encoding ASCII
Write-Host "Saved thumbprint to $outPath"
