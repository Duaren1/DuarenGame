// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"
namespace MatchState
{
	extern DUAREN_API const FName Cooldown; //自定义我们的matchstate 比赛时间已到，显示获胜者并开始冷却计时
}
UCLASS()
class DUAREN_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class AMyCharacter* EliminatedCharacter, class AMyPlayerController* VictimController, AMyPlayerController* AttackerPlayerController); //处理玩家淘汰的函数
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, class AController* ElimmedController); //重生的函数

	//void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving); //处理玩家离开游戏的逻辑

	//virtual float CalculateDamage(AController* Attacker, AController* Victim, float Damage);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 300.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0; //记录打开地图真正过去了多久

	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

	bool bTeamsMatch = false;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;//MatchState变化时我们会调用这个函数
private:
	float CountdownTime = 0.f; //用于记录Warmuptime的倒计时

};
