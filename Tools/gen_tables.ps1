# 사용: .\Tools\gen_tables.ps1 -DrTable <drtable 실행 파일 경로>  (DrTableSystem 0.12.0 이상 Releases에서 받는다)
param([Parameter(Mandatory = $true)][string]$DrTable)
# 샘플 테이블(GameAssets·EffectDefs·BossPhase·BossAttack·EffectExtensions)을 생성한다. DrFramework 플러그인은 테이블을 갖지 않는다.
$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$Output = Join-Path $env:TEMP 'DrSample-DrTable\sample'
$Schema = "$Root\Source\DrSample\Tables\Schemas"
$InputData = "$Root\Source\DrSample\Tables\Data"
$Cpp = "$Root\Source\DrSample\TableData\Generated"
New-Item -ItemType Directory -Force "$Output\client", "$Output\server" | Out-Null
& $DrTable build --input $InputData --schema $Schema --out-cpp $Cpp --out-client "$Output\client" --out-server "$Output\server" --prefix Ds --ue-plugin --api DRSAMPLE_API
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $DrTable check --client "$Output\client" --server "$Output\server"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
