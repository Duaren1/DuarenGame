// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Duaren/Character/MyCharacter.h"
#include "Duaren/PlayerController/MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Duaren/PlayerState/MyPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true; //Ϊ��ʵ������׶Σ�������������Ϸ�Ƴٿ�ʼ
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	//���ǰ����ǵ�ǰ���ڵ�GameState�������е�PlayerController
	FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); //�������ĵ�����
	for (; it; it++) //�������п�����
	{
		AMyPlayerController* PlayerController = Cast<AMyPlayerController>(*it);
		if (PlayerController)
		{
			PlayerController->OnMatchStateSet(MatchState, bTeamsMatch);
		}
	}
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime; //����������ͼ������ʱ��
		if (CountdownTime <= 0.f)
		{
			StartMatch(); //����ʱ������Ϸ��ʼ
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::PlayerEliminated(AMyCharacter* EliminatedCharacter, AMyPlayerController* VictimController, AMyPlayerController* AttackerPlayerController)
{
	AMyPlayerState* AttackerPlayerState = AttackerPlayerController ? Cast<AMyPlayerState>(AttackerPlayerController->PlayerState) : nullptr;
	AMyPlayerState* VictimPlayerState = VictimController ? Cast<AMyPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState) //������ɱ
	{
		AttackerPlayerState->AddtoScore(1.f);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddtoDefeats(1);
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset(); //�������ݻٵ���������������
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1); //�����е����þ��������ȡһ����ҳ�����
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]); //����
	}
}
