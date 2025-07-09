#pragma once

#define TRACELINE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250 //设置轮廓的颜色（紫色）
#define CUSTOM_DEPTH_BLUE 251 //蓝色
#define CUSTOM_DEPTH_TAN 252 //棕褐色

//枚举类 存储武器类型相关的参数

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssultRifle UMETA(DisplayNmae = "Assult Rifle"),
	EWT_RocketLauncher UMETA(DisplayNmae = "RocketLauncher"),
	EWT_Pistol UMETA(DisplayNmae = "Pistol"),
	EWT_SMG UMETA(DisplayNmae = "SMG"),
	EWT_Shotgun UMETA(DisplayNmae = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayNmae = "Sniper Rifle"),
	EWT_GrenadeLanucher UMETA(DisplayNmae = "GrenadeLanucher"),
	EWT_Flag UMETA(DisplayName = "Flag"),

	EWT_MAX UMETA(DisplayNmae = "DefaultMax")
};