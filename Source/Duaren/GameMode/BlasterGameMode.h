// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"
namespace MatchState
{
	extern DUAREN_API const FName Cooldown; //�Զ������ǵ�matchstate ����ʱ���ѵ�����ʾ��ʤ�߲���ʼ��ȴ��ʱ
}
UCLASS()
class DUAREN_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class AMyCharacter* EliminatedCharacter, class AMyPlayerController* VictimController, AMyPlayerController* AttackerPlayerController); //���������̭�ĺ���
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, class AController* ElimmedController); //�����ĺ���

	//void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving); //��������뿪��Ϸ���߼�

	//virtual float CalculateDamage(AController* Attacker, AController* Victim, float Damage);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 300.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0; //��¼�򿪵�ͼ������ȥ�˶��

	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

	bool bTeamsMatch = false;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;//MatchState�仯ʱ���ǻ�����������
private:
	float CountdownTime = 0.f; //���ڼ�¼Warmuptime�ĵ���ʱ

};
