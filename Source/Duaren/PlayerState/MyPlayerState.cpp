// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerState.h"
#include "Duaren/Character/MyCharacter.h"
#include "Duaren/PlayerController/MyPlayerController.h"
#include <Net/UnrealNetwork.h>

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyPlayerState, Defeats);
}

void AMyPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<AMyCharacter>(GetPawn()) : Character;
	if (Character)
	{
		MyController = MyController == nullptr ? Cast<AMyPlayerController>(Character->Controller) : MyController;
		if (MyController)
		{
			MyController->SetHUDScore(GetScore());
		}
	}
}

void AMyPlayerState::AddtoScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = Character == nullptr ? Cast<AMyCharacter>(GetPawn()) : Character;
	if (Character)
	{
		MyController = MyController == nullptr ? Cast<AMyPlayerController>(Character->Controller) : MyController;
		if (MyController)
		{
			MyController->SetHUDScore(GetScore());
		}
	}
}

void AMyPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<AMyCharacter>(GetPawn()) : Character;
	if (Character)
	{
		MyController = MyController == nullptr ? Cast<AMyPlayerController>(Character->Controller) : MyController;
		if (MyController)
		{
			MyController->SetHUDDefeats(Defeats);
		}
	}
}

void AMyPlayerState::AddtoDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr ? Cast<AMyCharacter>(GetPawn()) : Character;
	if (Character)
	{
		MyController = MyController == nullptr ? Cast<AMyPlayerController>(Character->Controller) : MyController;
		if (MyController)
		{
			MyController->SetHUDDefeats(Defeats);
		}
	}
}
