param()
. "$PSScriptRoot/Development.ps1"
$null = Get-WodMSBuild
foreach ($path in @(
    'Server/Game_Server/Include/boost/pfr.hpp',
    'Server/Game_Server/Include/rapidjson/document.h',
    'Server/Game_Server/Resource/HeightMesh.obj',
    'Server/Game_Server/Resource/NavMeshData3.obj',
    'Server/Game_Server/DataFile/CSV/skill_info.csv',
    'Server/Lobby_Server/Resource/HeightMesh2.obj',
    'Client/WarOfDimension/fmod_vc.lib',
    'Client/WarOfDimension/fmodL_vc.lib',
    'Client/WarOfDimension/fmod.dll',
    'Client/WarOfDimension/fmodL.dll'
)) {
    $file = Get-Item -LiteralPath (Join-Path $script:RepoRoot $path) -ErrorAction SilentlyContinue
    if (-not $file -or $file.Length -eq 0) { throw "필수 파일이 없거나 비어 있습니다: $path" }
}
foreach ($line in Get-Content -LiteralPath (Join-Path $script:RepoRoot '.gitattributes')) {
    if ($line -notmatch '^(.+) filter=lfs ') { continue }
    $path = $Matches[1]
    $file = Get-Item -LiteralPath (Join-Path $script:RepoRoot $path) -ErrorAction SilentlyContinue
    if (-not $file -or $file.Length -lt 1024) { throw "실제 LFS 에셋이 없습니다. git lfs pull로 받아야 합니다: $path" }
}
Write-Host '빌드 사전 조건 확인: PASS'
