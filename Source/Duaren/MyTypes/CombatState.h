#pragma once

UENUM(BlueprintType)
enum class ECombatState :uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"), //ø’œ–
	ECS_Reloading UMETA(DisplayName = "Reloading"), //ªªµØ
	ECS_ThrowingGrenade UMETA(DisplayName = "ThrowingGrenade"), //»” ÷¿◊
	ECS_SwappingWeapons UMETA(DisplayName = "SwappingWeapons"),

	ECS_MAX UMETA(DisplayName = "DefalutMax")
};
