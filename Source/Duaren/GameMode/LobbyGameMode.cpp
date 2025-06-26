// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (GameState)
	{
		int32 Num_Of_Players = GameState.Get()->PlayerArray.Num();

		UGameInstance* GameInstance = GetGameInstance();
		if (Num_Of_Players == 2)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;
				World->ServerTravel(FString("/Game/Maps/DuarenMap?listen"));
			}
		}
	}
}
