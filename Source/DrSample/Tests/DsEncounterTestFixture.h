#pragma once
// 엔카운터·오디오 테스트가 함께 쓰는 주입 월드(플레이어·보스·체크포인트·문).
#if WITH_DEV_AUTOMATION_TESTS
#include "Encounter/DsEncounterSubsystem.h"
#include "Audio/DsAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"
#include "Abilities/Combat/DsGameplayAbility_AttackLight.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "Boss/DsBossBrainComponent.h"
#include "Boss/DsBossCharacter.h"
#include "Characters/DsCharacter.h"
#include "Core/DsGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTestFixture.h"

namespace DsEncounterTests {
struct FFixture {
  DsTestFixture::FWorldFixture Base{true};
  ADsCharacter *Player;
  ADsBossCharacter *Boss;
  UDsAbilitySystemComponent *ASC;
  UDsEncounterSubsystem *Encounter;
  AActor *Checkpoint;
  AActor *Door;
  AActor *Start;
  FFixture() {
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
    Base.World->InitializeActorsForPlay(FURL());
    Base.World->SetBegunPlay(true);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player = Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(), FVector(-1000, 0, 100), FRotator::ZeroRotator, Params);
    Boss = Base.World->SpawnActor<ADsBossCharacter>(ADsBossCharacter::StaticClass(), FVector(2000, 0, 100), FRotator(0, 180, 0), Params);
    Checkpoint = Base.World->SpawnActor<ATargetPoint>(FVector(-500, 0, 100), FRotator::ZeroRotator);
    Door = Base.World->SpawnActor<AStaticMeshActor>(FVector(0, 0, 800), FRotator::ZeroRotator);
    Start = Base.World->SpawnActor<ATargetPoint>(FVector(-1000, 0, 100), FRotator::ZeroRotator);
    ASC = Cast<UDsAbilitySystemComponent>(Player->GetAbilitySystemComponent());
    ASC->GiveAbility(FGameplayAbilitySpec(UDsGameplayAbility_AttackLight::StaticClass(), 1));
    FDsBossPhaseRow P1;
    P1.Id = 1;
    P1.Phase = 1;
    P1.PatternSet = TEXT("P1");
    P1.MoveSpeedRate = 100;
    FDsBossPhaseRow P2 = P1;
    P2.Id = 2;
    P2.Phase = 2;
    P2.EnterHpRate = 50;
    FDsBossAttackRow Attack;
    Attack.Id = 1;
    Attack.PatternSet = TEXT("P1");
    Attack.Weight = 1;
    Attack.MaxRange = 3000;
    Attack.Cooldown = FTimespan::FromSeconds(60);
    Attack.Duration = FTimespan::FromSeconds(1);
    Boss->GetBrain()->InjectTables({P1, P2}, {Attack});
    Boss->GetBrain()->SetTarget(Player);
    Encounter = Base.World->GetSubsystem<UDsEncounterSubsystem>();
    Encounter->BindActors(Player, Boss, Checkpoint, Door, Start);
    Encounter->SetDoorClosedZ(250.f);
  }
  ~FFixture() {
    Base.World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(Base.World);
  }
  void Activate() {
    Player->SetActorLocation(Checkpoint->GetActorLocation());
    Encounter->Advance(.01f);
  }
  void Enter() {
    Player->SetActorLocation(FVector(301, 0, 100));
    Encounter->Advance(.01f);
    Encounter->Advance(Encounter->DoorMoveDuration);
  }
  void KillPlayer() { Boss->GetDrASC()->ApplyDamage(ASC, 10000, 0, Boss); }
  void KillBoss() { ASC->ApplyDamage(Boss->GetDrASC(), 10000, 0, Player); }
  void Retry() { Encounter->Advance(Encounter->RespawnDelay); }
};
}
#endif
