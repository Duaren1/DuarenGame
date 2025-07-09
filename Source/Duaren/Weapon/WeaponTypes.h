#pragma once

#define TRACELINE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250 //������������ɫ����ɫ��
#define CUSTOM_DEPTH_BLUE 251 //��ɫ
#define CUSTOM_DEPTH_TAN 252 //�غ�ɫ

//ö���� �洢����������صĲ���

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