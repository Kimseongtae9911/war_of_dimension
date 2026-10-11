param(
    [ValidateRange(1000, 2000000)][int] $Jobs = 200000,
    [ValidateRange(1, 20)][int] $Repeats = 5,
    [string] $OutputDirectory = '',
    [string] $Python = ''
)
. "$PSScriptRoot/Development.ps1"
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $script:RepoRoot ('artifacts/logs/jobqueue-baseline/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $OutputDirectory) { throw "측정 결과 덮어쓰기를 거부합니다: $OutputDirectory" }
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$pythonCommand = Get-WodPython $Python
$msbuild = Get-WodMSBuild
& $msbuild (Join-Path $script:RepoRoot 'Server/ServerCore/tests/ServerCore.Tests.vcxproj') '/p:Configuration=Release' '/p:Platform=x64' '/nologo' '/verbosity:minimal' 2>&1 | Tee-Object -FilePath (Join-Path $OutputDirectory 'build.log')
if ($LASTEXITCODE -ne 0) { throw 'JobQueue 측정 실행 파일 빌드 실패' }
$system = Get-CimInstance Win32_ComputerSystem
$processors = @(Get-CimInstance Win32_Processor)
$os = Get-CimInstance Win32_OperatingSystem
$environment = @{
    Cpu = @($processors | ForEach-Object { $_.Name.Trim() }); LogicalProcessors = $system.NumberOfLogicalProcessors
    PhysicalMemoryBytes = $system.TotalPhysicalMemory; OS = $os.Caption; OSVersion = $os.Version
    PowerPlanGuid = [regex]::Match(((& powercfg.exe /getactivescheme) -join "`n"), '[0-9a-fA-F]{8}(-[0-9a-fA-F]{4}){3}-[0-9a-fA-F]{12}').Value
    MSBuild = $msbuild; Configuration = 'Release x64'
    BenchmarkOptimization = '/O2 (ServerCore.Tests)'; Scope = 'Core JobQueue microbenchmark; not game server TPS'
}
$environment | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'environment.json') -Encoding utf8
& $pythonCommand -X utf8 (Join-Path $PSScriptRoot 'Measure-JobQueue.py') --exe (Join-Path $script:RepoRoot 'artifacts/bin/Release/ServerCore.Tests/ServerCore.Tests.exe') --output $OutputDirectory --jobs $Jobs --repeats $Repeats
if ($LASTEXITCODE -ne 0) { throw "JobQueue 측정 실패: $OutputDirectory" }
Write-Host "JobQueue 기준 측정 완료: $OutputDirectory/summary.json"
