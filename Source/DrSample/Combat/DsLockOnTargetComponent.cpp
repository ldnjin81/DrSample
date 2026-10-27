#include "Combat/DsLockOnTargetComponent.h"

#include "AbilitySystemGlobals.h"
#include "Abilities/DsAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/DsGameplayTags.h"

FVector UDsLockOnTargetComponent::GetTargetLocation() const {
  if (const auto* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
    if (!SocketName.IsNone() && Mesh->DoesSocketExist(SocketName))
      return Mesh->GetSocketLocation(SocketName);
  return GetOwner()->GetActorLocation() + ChestOffset;
}
bool UDsLockOnTargetComponent::IsAvailable() const {
  if (!bLockable || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed()) return false;
  const auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
  return !ASC || (!ASC->HasMatchingGameplayTag(DsTags::State_Dead) &&
    ASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()) > 0.f);
}
