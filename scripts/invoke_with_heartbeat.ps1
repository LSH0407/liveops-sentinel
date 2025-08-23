param(
  [Parameter(Mandatory=$true)][string]$Cmd,
  [int]$TimeoutSec=900,
  [string]$LogPath="build\_last_step.log"
)

Write-Host "[CMD] $Cmd"

# 간단한 타임아웃 실행
try {
  $job = Start-Job -ScriptBlock {
    param($command)
    Invoke-Expression $command
  } -ArgumentList $Cmd

  $timer = [System.Diagnostics.Stopwatch]::StartNew()
  $lastBeat = [System.Diagnostics.Stopwatch]::StartNew()

  while ($job.State -eq 'Running') {
    # 하트비트 출력
    if ($lastBeat.Elapsed.TotalSeconds -ge 10) {
      $elapsed = [int]$timer.Elapsed.TotalSeconds
      Write-Host "[heartbeat] $([DateTime]::Now.ToString('HH:mm:ss')) elapsed=${elapsed}s"
      $lastBeat.Restart()
    }
    
    # 타임아웃 체크
    if ($timer.Elapsed.TotalSeconds -ge $TimeoutSec) {
      Write-Warning "Timeout ${TimeoutSec}s reached. Stopping job..."
      Stop-Job $job -Force
      Remove-Job $job -Force
      exit 1
    }
    
    Start-Sleep -Milliseconds 1000
  }

  # 결과 수집
  $result = Receive-Job $job
  $exitCode = if ($job.State -eq 'Failed') { 1 } else { 0 }

  # 로그 기록
  if ($result) {
    $result | Out-String | Out-File -FilePath $LogPath -Encoding UTF8
    $result | Write-Host
  }

  Remove-Job $job -Force
  exit $exitCode

} catch {
  Write-Host "ERROR in invoke_with_heartbeat: $($_.Exception.Message)" -ForegroundColor Red
  exit 1
}
