#!/usr/bin/env pwsh
# LiveOps Sentinel Deployment Script

Write-Host "========================================" -ForegroundColor Green
Write-Host "LiveOps Sentinel Deployment Script" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

try {
    Write-Host "[1/5] Checking Git status..." -ForegroundColor Cyan
    git status
    if ($LASTEXITCODE -ne 0) { throw "Git status failed" }

    Write-Host ""
    Write-Host "[2/5] Adding all changes..." -ForegroundColor Cyan
    git add .
    if ($LASTEXITCODE -ne 0) { throw "Git add failed" }

    Write-Host ""
    Write-Host "[3/5] Committing changes..." -ForegroundColor Cyan
    git commit -m "Update release pipeline and dependencies"
    if ($LASTEXITCODE -ne 0) { throw "Git commit failed" }

    Write-Host ""
    Write-Host "[4/5] Pushing to GitHub..." -ForegroundColor Cyan
    git push origin main
    if ($LASTEXITCODE -ne 0) { throw "Git push failed" }

    Write-Host ""
    Write-Host "[5/5] Creating and pushing tag..." -ForegroundColor Cyan
    git tag v0.2.1
    if ($LASTEXITCODE -ne 0) { throw "Git tag failed" }

    git push origin v0.2.1
    if ($LASTEXITCODE -ne 0) { throw "Git push tag failed" }

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "DEPLOYMENT COMPLETED SUCCESSFULLY!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "GitHub Actions will now automatically:" -ForegroundColor Yellow
    Write-Host "- Build the project for Windows and Linux" -ForegroundColor White
    Write-Host "- Create release packages" -ForegroundColor White
    Write-Host "- Generate GitHub Release with downloads" -ForegroundColor White
    Write-Host ""
    Write-Host "Check progress at: https://github.com/LSH0407/liveops-sentinel/actions" -ForegroundColor Cyan
    Write-Host ""
}
catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Deployment failed!" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

Read-Host "Press Enter to continue"
