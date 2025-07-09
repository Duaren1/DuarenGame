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
	virtual void OnRep_Score() override;//playerstate�Ѿ�������Score������·����

	void AddtoScore(float ScoreAmount); //���·���

	//����������ǵ�Score��ʵ�ַ�ʽֻ������Ҫ�����ֶ�����һ�����縴�Ƶı���
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_Defeats();
	void AddtoDefeats(int32 DefeatsAmount); //�������ǵķ���
private:
	UPROPERTY()
	class AMyCharacter* Character;

	UPROPERTY()
	class AMyPlayerController* MyController;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats; //������
};
