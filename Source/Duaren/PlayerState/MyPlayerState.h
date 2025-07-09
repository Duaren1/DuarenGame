// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class DUAREN_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void OnRep_Score() override;//playerstate已经内置了Score了且网路复制

	void AddtoScore(float ScoreAmount); //更新分数

	//可以类比我们的Score的实现方式只不过需要我们手动设置一个网络复制的变量
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_Defeats();
	void AddtoDefeats(int32 DefeatsAmount); //更新我们的分数
private:
	UPROPERTY()
	class AMyCharacter* Character;

	UPROPERTY()
	class AMyPlayerController* MyController;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats; //死亡数
};
