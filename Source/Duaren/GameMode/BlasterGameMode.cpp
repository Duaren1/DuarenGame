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
	bDelayedStart = true; //为了实现热身阶段，我们先设置游戏推迟开始
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	//我们把我们当前所在的GameState告诉所有的PlayerController
	FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); //控制器的迭代器
	for (; it; it++) //遍历所有控制器
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
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime; //加上启动地图所花的时间
		if (CountdownTime <= 0.f)
		{
			StartMatch(); //倒计时结束游戏开始
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

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState) //不是自杀
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
		ElimmedCharacter->Reset(); //将即将摧毁的玩家与控制器分离
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1); //上两行的作用就是随机获取一个玩家出生点
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]); //重生
	}
}
