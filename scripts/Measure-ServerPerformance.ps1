param(
    [int[]] $Clients = @(4, 16, 64),
    [ValidateRange(1, 20)][int] $Repeats = 5,
    [string] $Python = '',
    [string] $OutputDirectory = '',
    [switch] $SkipBuild,
    [switch] $DisableMetrics,
    [ValidateSet('before', 'after', 'diagnostic')][string] $Stage = 'before'
)
. "$PSScriptRoot/Development.ps1"
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $script:RepoRoot ('artifacts/logs/server-baseline/' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
if (Test-Path -LiteralPath $OutputDirectory) { throw '기존 기준 측정 디렉터리를 덮어쓰지 않습니다.' }
if (-not $SkipBuild) { & "$PSScriptRoot/Build.ps1" -Module Servers -Configuration Release }
$pythonCommand = Get-WodPython $Python
$extraArguments = @()
if ($DisableMetrics) { $extraArguments += '--disable-metrics' }
& $pythonCommand -X utf8 "$script:RepoRoot/tools/server_lab/baseline.py" --clients @Clients --repeats $Repeats --output $OutputDirectory --stage $Stage @extraArguments
if ($LASTEXITCODE -ne 0) { throw "기준 측정에 기능/수집/종료 실패가 있습니다. 원시/요약: $OutputDirectory" }
