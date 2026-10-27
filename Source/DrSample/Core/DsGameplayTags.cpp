#include "Core/DsGameplayTags.h"
// DrFramework에서 사용하는 네이티브 게임플레이 태그를 정의한다.
namespace DsTags {
UE_DEFINE_GAMEPLAY_TAG(State_Groggy, "State.Groggy");
// 버프 상태
UE_DEFINE_GAMEPLAY_TAG(State_Buff_Enrage, "State.Buff.Enrage");
UE_DEFINE_GAMEPLAY_TAG(State_Buff_Guard, "State.Buff.Guard");
UE_DEFINE_GAMEPLAY_TAG(State_Buff_Shield, "State.Buff.Shield");
// 디버프 상태
UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Burn, "State.Debuff.Burn");
UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Chill, "State.Debuff.Chill");
UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Sunder, "State.Debuff.Sunder");
UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Rot, "State.Debuff.Rot");
UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Bleed, "State.Debuff.Bleed");
UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Crush, "State.Debuff.Crush");
UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Invulnerable");
// 전투 동작 상태
UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "State.Attacking");
UE_DEFINE_GAMEPLAY_TAG(State_Guarding, "State.Guarding");
UE_DEFINE_GAMEPLAY_TAG(State_Parrying, "State.Parrying");
UE_DEFINE_GAMEPLAY_TAG(State_Dodging, "State.Dodging");
UE_DEFINE_GAMEPLAY_TAG(State_Staggered, "State.Staggered");
// 어빌리티 식별 태그
UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Light, "Ability.Attack.Light");
UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Heavy, "Ability.Attack.Heavy");
UE_DEFINE_GAMEPLAY_TAG(Ability_Guard, "Ability.Guard");
UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
// SetByCaller 데이터 태그
UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
UE_DEFINE_GAMEPLAY_TAG(Data_PoiseDamage, "Data.PoiseDamage");
} // namespace DsTags
