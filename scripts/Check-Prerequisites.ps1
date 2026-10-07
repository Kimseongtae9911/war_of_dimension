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
foreach ($path in (& git -C $script:RepoRoot lfs ls-files --name-only)) {
    $file = Get-Item -LiteralPath (Join-Path $script:RepoRoot $path) -ErrorAction SilentlyContinue
    if (-not $file -or $file.Length -eq 0 -or ($file.Length -lt 1024 -and (Get-Content -LiteralPath $file.FullName -First 1) -eq 'version https://git-lfs.github.com/spec/v1')) {
        throw "실제 LFS 에셋이 없습니다. git lfs pull로 받아야 합니다: $path"
    }
}
if ($LASTEXITCODE -ne 0) { throw 'Git LFS 파일 목록을 확인하지 못했습니다.' }
Write-Host '빌드 사전 조건 확인: PASS'
