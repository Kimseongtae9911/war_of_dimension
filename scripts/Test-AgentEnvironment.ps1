param([switch] $StaticOnly)
. "$PSScriptRoot/Development.ps1"
foreach ($path in @('AGENTS.md', 'CLAUDE.md', 'GEMINI.md', '.codex/config.toml', '.mcp.json', '.gemini/settings.json', '.serena/project.yml', 'docs/INDEX.md', 'docs/guides/INDEX.md', '.agents/skills/archify/SKILL.md')) {
    if (-not (Test-Path -LiteralPath (Join-Path $script:RepoRoot $path))) { throw "필수 파일이 없습니다: $path" }
}
foreach ($path in @('.mcp.json', '.gemini/settings.json')) {
    $null = Get-Content -LiteralPath (Join-Path $script:RepoRoot $path) -Raw | ConvertFrom-Json
}
& "$PSScriptRoot/New-CompilationDatabase.ps1"
$database = @(Get-Content -LiteralPath (Join-Path $script:RepoRoot 'compile_commands.json') -Raw | ConvertFrom-Json)
if ($database.Count -eq 0) { throw 'Compilation database가 비어 있습니다.' }
foreach ($entry in $database) {
    if (-not (Test-Path -LiteralPath $entry.file)) { throw "소스 경로가 없습니다: $($entry.file)" }
}
Write-Host "에이전트 설정 정적 검증: PASS ($($database.Count)개 소스)"
if (-not $StaticOnly) {
    Get-Command serena, node -ErrorAction Stop | Out-Null
    foreach ($source in @('Server/Game_Server/CServer.cpp', 'Server/Lobby_Server/CServer.cpp', 'Client/WarOfDimension/WarOfDimension.cpp')) {
        & serena project index-file $source $script:RepoRoot
        if ($LASTEXITCODE -ne 0) { throw "Serena 심볼 조회 준비 실패: $source" }
    }
    & node (Join-Path $script:RepoRoot '.agents/skills/archify/bin/archify.mjs') doctor
    if ($LASTEXITCODE -ne 0) { throw 'Archify 환경 점검 실패' }
}
