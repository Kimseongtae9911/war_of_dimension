param(
    [ValidateSet('Start', 'Stop', 'Status', 'InstallStartup', 'RemoveStartup')]
    [string] $Action = 'Start',
    [string] $ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [ValidateRange(1024, 65535)][int] $Port = 9120,
    [string] $ContextPath = (Join-Path (Split-Path -Parent $PSScriptRoot) '.serena/codex-http.yml'),
    [string] $RuntimeRoot = ''
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path.TrimEnd('\', '/')
$identity = "$($ProjectRoot.ToLowerInvariant())|$Port"
$digest = [Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($identity))).Substring(0, 12)
$runtimeRoot = if ($RuntimeRoot) { [IO.Path]::GetFullPath($RuntimeRoot) } else { Join-Path $ProjectRoot '.runtime' }
$statePath = Join-Path $runtimeRoot "serena-http-$Port.json"
$startupPath = Join-Path ([Environment]::GetFolderPath('Startup')) "Serena-$digest.lnk"
$endpoint = "http://127.0.0.1:$Port/mcp"

function Get-OwnedSerenaProcess($Record) {
    if ($Record.ProjectRoot -ne $ProjectRoot -or $Record.Port -ne $Port) {
        throw 'Serena 실행 기록의 프로젝트 또는 포트가 일치하지 않습니다.'
    }

    $process = Get-Process -Id $Record.ProcessId -ErrorAction SilentlyContinue
    if (-not $process) { return $null }
    if ($process.Path -ne $Record.Executable -or $process.StartTime.ToUniversalTime().Ticks -ne ([datetime]$Record.StartTimeUtc).ToUniversalTime().Ticks) {
        throw 'PID가 재사용되었습니다. 다른 프로세스를 재사용하거나 종료하지 않습니다.'
    }

    return $process
}

function Test-SerenaListener {
    $client = [Net.Sockets.TcpClient]::new()
    try {
        $client.ConnectAsync('127.0.0.1', $Port).Wait(300) | Out-Null

        return $client.Connected
    } catch {
        return $false
    } finally {
        $client.Dispose()
    }
}

function Stop-OwnedSerenaTree($Process) {
    # Serena의 Python worker와 clangd까지 이 스크립트가 시작한 트리만 종료한다.
    & taskkill.exe /PID $Process.Id /T /F | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Serena 프로세스 트리 종료 실패: $($Process.Id)" }
}

$mutex = [Threading.Mutex]::new($false, "Local\SerenaHttp-Port-$Port")
$locked = $false
try {
    try { $locked = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $locked = $true }
    if (-not $locked) { throw "포트 $Port 의 Serena 관리 명령이 이미 실행 중입니다. 완료 후 다시 실행하세요." }

    if ($Action -eq 'RemoveStartup') {
        if (Test-Path -LiteralPath $startupPath) { Remove-Item -LiteralPath $startupPath }
        Write-Host "로그인 자동 시작 해제: $startupPath"

        return
    }

    $record = if (Test-Path -LiteralPath $statePath) { Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json } else { $null }
    $process = if ($record) { Get-OwnedSerenaProcess $record } else { $null }
    if ($Action -eq 'Status') {
        [pscustomobject]@{ Project = $ProjectRoot; Endpoint = $endpoint; ProcessId = if ($process) { $process.Id } else { $null }; Listening = [bool]($process -and (Test-SerenaListener)); Startup = (Test-Path -LiteralPath $startupPath) }

        return
    }

    if ($Action -eq 'Stop') {
        if ($process) { Stop-OwnedSerenaTree $process }
        if (Test-Path -LiteralPath $statePath) { Remove-Item -LiteralPath $statePath }
        Write-Host "Serena 중지 완료: $ProjectRoot (포트 $Port). 로그인 자동 시작은 별도 RemoveStartup으로 해제합니다."

        return
    }

    $reused = [bool]$process
    if ($process) {
        if (-not (Test-SerenaListener)) { throw '기존 Serena 프로세스가 응답하지 않습니다. Stop 후 Start로 다시 시작하세요.' }
    } else {
        if (Test-SerenaListener) { throw "포트 $Port 를 다른 프로세스가 사용 중입니다. 프로젝트별로 다른 포트를 지정하세요." }
        if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot '.serena/project.yml'))) { throw '프로젝트의 .serena/project.yml이 없습니다.' }
        $ContextPath = (Resolve-Path -LiteralPath $ContextPath).Path
        Get-Command serena, uv -CommandType Application -ErrorAction Stop | Out-Null
        $uvToolsRoot = (& uv tool dir).Trim()
        if ($LASTEXITCODE -ne 0) { throw 'uv tool 디렉터리 조회 실패' }
        $executable = Join-Path $uvToolsRoot 'serena-agent/Scripts/python.exe'
        if (-not (Test-Path -LiteralPath $executable)) { throw "Serena Python 환경이 없습니다: $executable" }
        [IO.Directory]::CreateDirectory($runtimeRoot) | Out-Null
        $arguments = @("`"$(Join-Path $PSScriptRoot 'serena_http.py')`"", '--port', "$Port", '--project', "`"$ProjectRoot`"", '--context', "`"$ContextPath`"")
        # 여러 로그인 바로가기가 같은 언어 서버 패키지를 동시에 설치하지 않도록 한다.
        $initializationMutex = [Threading.Mutex]::new($false, 'Local\SerenaHttp-Initialization')
        $initializationLocked = $false
        try {
            try { $initializationLocked = $initializationMutex.WaitOne(60000) } catch [Threading.AbandonedMutexException] { $initializationLocked = $true }
            if (-not $initializationLocked) { throw '다른 프로젝트의 Serena 초기화 대기 시간이 초과되었습니다. 다시 Start를 실행하세요.' }
            $process = Start-Process -FilePath $executable -ArgumentList $arguments -WorkingDirectory $ProjectRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $runtimeRoot "serena-http-$Port.out.log") -RedirectStandardError (Join-Path $runtimeRoot "serena-http-$Port.err.log")
            try {
                $record = [pscustomobject]@{ ProjectRoot = $ProjectRoot; Port = $Port; Endpoint = $endpoint; ProcessId = $process.Id; Executable = $executable; StartTimeUtc = $process.StartTime.ToUniversalTime().ToString('O') }
                $record | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
                $deadline = [DateTime]::UtcNow.AddSeconds(90)
                while (-not (Test-SerenaListener)) {
                    $process.Refresh()
                    if ($process.HasExited) { throw "Serena가 종료되었습니다. 로그 확인: $runtimeRoot\serena-http-$Port.err.log" }
                    if ([DateTime]::UtcNow -ge $deadline) { throw 'Serena 시작 제한 시간 90초를 초과했습니다.' }
                    Start-Sleep -Milliseconds 300
                }
            } catch {
                $process.Refresh()
                if (-not $process.HasExited) { Stop-OwnedSerenaTree $process }
                if (Test-Path -LiteralPath $statePath) { Remove-Item -LiteralPath $statePath }
                throw
            }
        } finally {
            if ($initializationLocked) { $initializationMutex.ReleaseMutex() }
            $initializationMutex.Dispose()
        }
    }

    if ($Action -eq 'InstallStartup') {
        $shell = New-Object -ComObject WScript.Shell
        $shortcut = $shell.CreateShortcut($startupPath)
        $shortcut.TargetPath = (Get-Command pwsh -CommandType Application -ErrorAction Stop).Source
        $shortcut.Arguments = "-NoProfile -NonInteractive -WindowStyle Hidden -File `"$PSCommandPath`" -ProjectRoot `"$ProjectRoot`" -Port $Port -ContextPath `"$ContextPath`" -RuntimeRoot `"$runtimeRoot`""
        $shortcut.WorkingDirectory = $ProjectRoot
        $shortcut.WindowStyle = 7
        $shortcut.Description = "프로젝트별 Serena HTTP 서버: $ProjectRoot"
        $shortcut.Save()
        Write-Host "로그인 자동 시작 등록: $startupPath"
    }

    [pscustomobject]@{ Project = $ProjectRoot; Endpoint = $endpoint; ProcessId = $process.Id; Reused = $reused }
} finally {
    if ($locked) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
