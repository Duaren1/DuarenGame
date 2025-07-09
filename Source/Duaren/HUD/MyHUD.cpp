// Fill out your copyright notice in the Description page of Project Settings.


#include "MyHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "MyAnnouncement.h"

void AMyHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewPortsize;
	GEngine->GameViewport->GetViewportSize(ViewPortsize);
	const FVector2D ViewPortCenter(ViewPortsize.X / 2.f, ViewPortsize.Y / 2.f);

	float SpreadScaled = CrossHairSpreadMax * HUDPackage.CrossHairSpread;

	if (HUDPackage.CrossHairCenter)
	{
		FVector2D Spread(0.f, 0.f);
		DrawCrossHair(HUDPackage.CrossHairCenter, ViewPortCenter, Spread, HUDPackage.LinearColor);
	}
	if (HUDPackage.CrossHairLeft)
	{
		FVector2D Spread(-SpreadScaled, 0.f);
		DrawCrossHair(HUDPackage.CrossHairLeft, ViewPortCenter, Spread, HUDPackage.LinearColor);
	}
	if (HUDPackage.CrossHairRight)
	{
		FVector2D Spread(SpreadScaled, 0.f);
		DrawCrossHair(HUDPackage.CrossHairRight, ViewPortCenter, Spread, HUDPackage.LinearColor);
	}
	if (HUDPackage.CrossHairTop)
	{
		FVector2D Spread(0.f, -SpreadScaled);
		DrawCrossHair(HUDPackage.CrossHairTop, ViewPortCenter, Spread, HUDPackage.LinearColor);
	}
	if (HUDPackage.CrossHairBottom)
	{
		FVector2D Spread(0.f, SpreadScaled);
		DrawCrossHair(HUDPackage.CrossHairBottom, ViewPortCenter, Spread, HUDPackage.LinearColor);
	}
}

void AMyHUD::DrawCrossHair(UTexture2D* Texture, FVector2D ViewPortCenter, FVector2D Spread, FLinearColor CrossHairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint(
		ViewPortCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewPortCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);
	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrossHairColor
	);
}

void AMyHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AMyHUD::AddCharacterOverlay()
{
	APlayerController* OwningPlayer = GetOwningPlayerController();
	if (OwningPlayer && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(OwningPlayer, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void AMyHUD::AddAnnouncement()
{
	APlayerController* OwningPlayer = GetOwningPlayerController();
	if (OwningPlayer && AnnouncementClass)
	{
		Announcement = CreateWidget<UMyAnnouncement>(OwningPlayer, AnnouncementClass);
		Announcement->AddToViewport();
	}
}
