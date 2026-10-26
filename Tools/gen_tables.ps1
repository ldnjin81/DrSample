param([string]$DrTable = 'C:\Project\ProjectDT\Tools\DrTable\drtable.exe')
# 샘플 테이블(GameAssets·EffectDefs·BossPhase·BossAttack·EffectExtensions)을 생성한다. DrFramework 플러그인은 테이블을 갖지 않는다.
$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$Output = Join-Path $env:TEMP 'DrSample-DrTable\sample'
$Schema = "$Root\Source\DrSample\Tables\Schemas"
$InputData = "$Root\Source\DrSample\Tables\Data"
$Cpp = "$Root\Source\DrSample\TableData\Generated"
New-Item -ItemType Directory -Force "$Output\client", "$Output\server" | Out-Null
& $DrTable build --input $InputData --schema $Schema --out-cpp $Cpp --out-client "$Output\client" --out-server "$Output\server" --prefix Ds --ue-plugin
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
# drtable은 모듈 API 매크로를 붙이지 않는다. 다른 모듈이 행 정적 함수(Find/GetAll)를 링크할 수 있게 행 구조체를 내보낸다.
Get-ChildItem $Cpp -Filter "Ds*Row.h" | ForEach-Object {
    $h = Get-Content $_.FullName -Raw
    $n = [regex]::Replace($h, "struct (FDs\w+Row) : public FTableRowBase", "struct DRSAMPLE_API `$1 : public FTableRowBase")
    if ($n -ne $h) { [IO.File]::WriteAllText($_.FullName, $n, (New-Object Text.UTF8Encoding $false)) }
}
& $DrTable check --client "$Output\client" --server "$Output\server"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
