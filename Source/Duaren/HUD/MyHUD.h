// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MyHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY() //���ڷ���ϵͳ�ĺ�
public:
	class UTexture2D* CrossHairCenter;
	UTexture2D* CrossHairLeft;
	UTexture2D* CrossHairRight;
	UTexture2D* CrossHairTop;
	UTexture2D* CrossHairBottom;

	float CrossHairSpread; //׼�ǵ���ɢֵ
	FLinearColor LinearColor; //��ɫ
};

UCLASS()
class DUAREN_API AMyHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;



	UPROPERTY(EditAnywhere, Category = "Player States")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;
	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<class UUserWidget> AnnouncementClass;
	//UPROPERTY(EditAnywhere, Category = "Announcement")
	//TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	UPROPERTY()
	class UMyAnnouncement* Announcement;

	void AddCharacterOverlay();
	void AddAnnouncement();

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	void DrawCrossHair(UTexture2D* Texture, FVector2D ViewPortCenter, FVector2D Spread, FLinearColor CrossHairColor);
	//����spread���xy��ֵ��һ���ģ�����������������������ֻ��һ��float����ʶ��ֵ
protected:
	virtual void BeginPlay() override;
private:
	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrossHairSpreadMax = 16.f;

};
