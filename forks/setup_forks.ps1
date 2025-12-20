# OpenToken Library Forks Setup Script

Write-Host "Setting up OpenToken-patched Yubico libraries in Editable mode..." -ForegroundColor Cyan

$ForksDir = "c:\OpenToken\forks"

# 1. Install yubikey-manager fork
Write-Host "`n[1/2] Installing yubikey-manager fork..." -ForegroundColor Yellow
cd "$ForksDir\yubikey-manager"
pip install -e .

# 2. Install python-yubico fork
Write-Host "`n[2/2] Installing python-yubico fork..." -ForegroundColor Yellow
cd "$ForksDir\python-yubico"
pip install -e .

Write-Host "`nSetup Complete! Your system is now using the local forks." -ForegroundColor Green
Write-Host "Try running: ykman list"
