param(
    [Parameter(Mandatory)][string] $Output,
    [ValidateRange(2, 600)][int] $Seconds = 30,
    [ValidateRange(1, 10)][int] $Interval = 2
)
# 실행 중인 로컬 서버의 스레드별 누적 CPU를 읽는다. 부하나 스레드 상태는 변경하지 않는다.
$repoRoot = Split-Path $PSScriptRoot -Parent
$recordPath = Join-Path $repoRoot '.runtime/processes.json'
if (-not (Test-Path -LiteralPath $recordPath)) { throw '로컬 서버 실행 기록이 없습니다.' }
$records = @(Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json)
$destination = [IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $destination) { throw '기존 출력은 덮어쓰지 않습니다.' }
[IO.Directory]::CreateDirectory((Split-Path $destination -Parent)) | Out-Null
$watch = [Diagnostics.Stopwatch]::StartNew()
while ($watch.Elapsed.TotalSeconds -lt $Seconds) {
    $samples = @()
    foreach ($record in $records) {
        $process = Get-Process -Id $record.ProcessId -ErrorAction SilentlyContinue
        if (-not $process) { continue }
        if ($process.StartTime.ToUniversalTime().Ticks -ne ([datetime]$record.StartTimeUtc).ToUniversalTime().Ticks) { throw 'PID 시작 시각 불일치' }
        $threadRows = @()
        foreach ($thread in $process.Threads) {
            try {
                $threadRows += @{ id = $thread.Id; cpu_ms = $thread.TotalProcessorTime.TotalMilliseconds; state = $thread.ThreadState.ToString() }
            } catch {
                # 표본 중 종료된 스레드는 0으로 보정하지 않고 누락한다.
            }
        }
        $samples += @{ role = $record.Module; pid = $process.Id; start = $record.StartTimeUtc; cpu_ms = $process.TotalProcessorTime.TotalMilliseconds; threads = $threadRows }
    }
    @{ timestamp_ms = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds(); elapsed_s = $watch.Elapsed.TotalSeconds; processes = $samples } |
        ConvertTo-Json -Depth 6 -Compress | Add-Content -LiteralPath $destination -Encoding utf8
    Start-Sleep -Seconds $Interval
}
Write-Output "스레드 CPU 표본: $destination"
