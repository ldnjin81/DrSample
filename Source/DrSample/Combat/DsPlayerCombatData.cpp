#include "Combat/DsPlayerCombatData.h"
#include "TableData/DsGameAssets.h"

#include "Animation/AnimMontage.h"

FDsMeleeStage FDsMeleeStage::Make(const TCHAR *MontagePath, float Duration,
                                   float ActiveStart, float ActiveEnd, float DamageRate,
                                   float PoiseDamage, float StaminaCost,
                                   float LungeDistance) {
  FDsMeleeStage Stage;
  Stage.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(MontagePath));
  Stage.Duration = Duration;
  Stage.ActiveStart = ActiveStart;
  Stage.ActiveEnd = ActiveEnd;
  Stage.DamageRate = DamageRate;
  Stage.PoiseDamage = PoiseDamage;
  Stage.StaminaCost = StaminaCost;
  Stage.LungeDistance = LungeDistance;
  return Stage;
}

UDsPlayerCombatData::UDsPlayerCombatData() {
  // 판정 구간은 각 모션 길이의 약 30~50%이다.
  LightStages = {
      FDsMeleeStage::Make(TEXT(""), 1.67f, .50f, .84f, 1.0f, 20.f, 12.f, 80.f),
      FDsMeleeStage::Make(TEXT(""), 1.40f, .42f, .70f, 1.1f, 20.f, 12.f, 80.f),
      FDsMeleeStage::Make(TEXT(""), 1.87f, .56f, .94f, 1.3f, 30.f, 12.f, 80.f)};
  // 강공격 모션은 약 2.6m 전진하고 캡슐이 따라가므로 별도 전진은 주지 않는다.
  HeavyStages = {FDsMeleeStage::Make(TEXT(""), 3.03f, 1.36f, 1.82f, 2.2f, 60.f, 30.f, 0.f)};
  Dodge.RollMontages.SetNum(8);
}

const UDsPlayerCombatData* UDsPlayerCombatData::Get() {
  static TWeakObjectPtr<UDsPlayerCombatData> Cached;
  if (Cached.IsValid()) return Cached.Get();
  auto* Data = DsGameAssets::Load<UDsPlayerCombatData>(TEXT("PlayerCombat"));
  if (!Data) {
    UE_LOG(LogTemp, Warning, TEXT("플레이어 전투 데이터가 없어 기본값을 사용합니다."));
    Data = NewObject<UDsPlayerCombatData>();
  }
  // 클래스 생성 중에는 테이블을 조회하지 않는다. 기본값과 같은 값이라 저장에서 생략된 참조는 첫 사용 시 채운다.
  const TCHAR* LightKeys[] = {TEXT("AM_Attack_PrimaryA"), TEXT("AM_Attack_PrimaryB"), TEXT("AM_Attack_PrimaryC")};
  for (int32 Index = 0; Index < FMath::Min(Data->LightStages.Num(), 3); ++Index)
    if (Data->LightStages[Index].Montage.IsNull()) Data->LightStages[Index].Montage = TSoftObjectPtr<UAnimMontage>(DsGameAssets::Path(LightKeys[Index]));
  if (!Data->HeavyStages.IsEmpty() && Data->HeavyStages[0].Montage.IsNull())
    Data->HeavyStages[0].Montage = TSoftObjectPtr<UAnimMontage>(DsGameAssets::Path(TEXT("AM_Attack_RMB")));
  const TCHAR* Suffixes[] = {TEXT("F"), TEXT("FR"), TEXT("R"), TEXT("BR"), TEXT("B"), TEXT("BL"), TEXT("L"), TEXT("FL")};
  for (int32 Index = 0; Index < FMath::Min(Data->Dodge.RollMontages.Num(), 8); ++Index)
    if (Data->Dodge.RollMontages[Index].IsNull()) Data->Dodge.RollMontages[Index] = TSoftObjectPtr<UAnimMontage>(DsGameAssets::Path(FName(FString(TEXT("AM_Roll_")) + Suffixes[Index])));
  if (Data->Guard.Montage.IsNull()) Data->Guard.Montage = TSoftObjectPtr<UAnimMontage>(DsGameAssets::Path(TEXT("AM_Deflect_InPlace")));
  Cached = Data;
  return Data;
}
