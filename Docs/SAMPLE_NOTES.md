# 샘플 콘텐츠 가져오기와 알려진 한계

## Lyra 콘텐츠 가져오기

샘플의 메시·스켈레톤·애니메이션은 Epic Games의 **Lyra Starter Game**에서 가져온다. Epic 콘텐츠는 저장소에 넣지 않으며(라이선스상 재배포 불가), 각자 설치한 Lyra에서 복사한다.

```powershell
.\Tools\import_epic_content.ps1 -LyraPath '<Lyra Starter Game 프로젝트 폴더>'
```

- `Tools/epic_content_manifest.txt`에 고정된 117개 에셋(Manny/Quinn 메시·스켈레톤·물리 에셋·이동 블렌드·공격/피격/사망 시퀀스와 그 의존성)을 **원래 패키지 경로**(`Content/Characters/`, `Content/Effects/`, `Content/PhysicsMaterials/`)로 복사한다. 이 폴더들은 `.gitignore`에 있다.
- Lyra 원본은 읽기만 한다. 복사본에서만 Lyra 전용 참조(로드할 수 없는 게임 노티파이, 에디터 AnimationModifiers 사용자 데이터, PA_Mannequin의 Lyra 전용 물리 재질)를 정리한다. 정리는 `UDsSampleContent::CleanAnimation/CleanPhysics`(에디터 보조 함수)를 `Tools/prepare_epic_content.py`가 호출해 수행한다.
- 입력·출력 해시를 `Saved/import_epic_content-state.json`에 남겨, 다시 실행하면 바뀐 것이 없을 때 건너뛴다. 정리 스크립트나 DrSample DLL이 바뀌면 다시 정리한다.
- 의존성 목록을 새로 뽑으려면 `DRSAMPLE_DISCOVER=1`, `DRSAMPLE_LYRA_PATH=<Lyra 프로젝트>`로 `prepare_epic_content.py`를 실행한다.

## 저장소에 들어 있는 샘플 에셋

| 위치 | 내용 |
| --- | --- |
| `Content/DrSample/Characters` | 플레이어 `B_DrSamplePlayer`(부모 `ADsCharacter`), 적 `B_DrSampleEnemy`(부모 `ADsSampleEnemy`) |
| `Content/DrSample/Animations` | 애님 BP `ABP_DrSample`(`UDsSampleAnimInstance`), 공격·피격(앞·뒤·좌·우)·사망 몽타주 |
| `Content/DrSample/Combat` | 플레이어 전투 데이터(`UDsPlayerCombatData`), 적 데이터(`UDsBossData`) |
| `Content/DrSample/Input` | 입력 액션 10개, 매핑 `IMC_DrSample` |
| `Content/DrSample/Maps` | `L_DrSample`(바닥·조명·하늘·PlayerStart·내비 영역·적) |
| `Content/DrSample/B_DrSampleGameMode` | 게임 모드 BP(부모 `ADsGameMode`) |
| `Content/Data` | 구운 테이블(DA_GameAssets·DA_EffectDefs·DA_EffectExtensions·DA_BossPhase·DA_BossAttack) |

이 에셋은 모두 `Tools/build_sample_content.py`로 다시 만들 수 있다. 스크립트는 이미 있는 에셋을 재사용하므로, 부모 클래스를 바꿀 때는 해당 에셋을 지우고 실행한다.

## 검증

- `Automation RunTests DrFramework+DrSample`: 73개(기반 5, 샘플 68). `DrSample.PIE.BasicCombat`은 `L_DrSample`에서 이동 → 공격 명중·피격 → 적 추적·반격·플레이어 피격 → 적 사망을 실제 BP·몽타주로 검증한다.
- 화면 확인: `-RenderOffscreen -ResX=1280 -ResY=720`으로 같은 테스트를 실행하면 `Saved/Screenshots/DrSample/`에 장면 4장(대기·명중·피격·사망)을 테스트 전용 사선 카메라로 남긴다.
- `-ExecCmds`에는 `Automation RunTests …`만 쓴다. `Automation List; Automation RunTests …`처럼 `Automation` 접두사를 두 번 쓰면 두 번째 명령이 잘못 해석돼 테스트가 0개 실행된다.

## 알려진 한계

- 사운드·별도 FX·메뉴 화면 콘텐츠는 없다. 해당 기능은 대체 동작(무음, 기본 연출)을 쓴다.
- 적은 보스 시스템(`ADsBossCharacter`)을 재사용한다. 공격·사망은 보스 시퀀스 경로를, 일반 피격 반응만 샘플 파생 클래스(`ADsSampleEnemy`)가 공격 방향(앞·뒤·좌·우)에 맞춰 재생한다. 사망한 적은 피격 몽타주를 재생하지 않는다.
- 헤드리스 이동 테스트는 EnhancedInput Move 액션을 주입하며 실제 키 입력은 넣지 않는다.
- Rider에서는 `DrSample.uproject`로 연다. 생성된 `.sln/.slnx`로 열면 include 경로가 Windows 환경 변수 길이 제한(32,767자)을 넘어 MSBuild가 "환경 변수 이름 또는 값이 너무 깁니다" 오류로 멈춘다.
