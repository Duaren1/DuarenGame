// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class DUAREN_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementConutdown(float CountdownTime);
	virtual void OnPossess(APawn* InPawn) override; //���ڿ���һ��pawn��ʱ����ã����������ǽ�ɫ����ʱ�Ϳ��Ը���Ѫ����
	virtual void SetHUDTime();
	virtual void Tick(float DeltaTime) override;
	void CheakPing(float DeltaTime);
	virtual float GetServerTime();
	virtual void ReceivedPlayer() override; //��д��������ڴ˴��������ǵ�ʱ�������������ͬ��
	void OnMatchStateSet(FName State, bool bTeamsMatch = false); //��������MatchState�ĺ���
	void HandleMatchHasSatate();
	void HandleCooldown();
protected:
	virtual void BeginPlay() override;

	//�����������ڽ���������Ϳͻ���ʱ�䲻ͳһ����ı���
	UFUNCTION(Server, Reliable)
	void ServerRequstServerTime(float TimeOfClientRequest); //���ڷ��ͷ�������ǰʱ��ĺ���,�������ӳ���Ϊ������Ҫʱ��,���͵Ĳ�����ʾ��ʱ������ʱ��

	UFUNCTION(Client, Reliable) //���������ÿͻ���ִ��
		void ClinetReportServerTime(float TimeOfClinetRequest, float TimeServeRecivedClientRequest);
	//�ͻ������ڽ��շ�����ʱ��ĺ���������������ͨ��һЩ������RTT�������ӳ�����

	float ClientServerTimeDelta = 0.f; //��������ͻ��˵�ʱ���

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequencs = 5.f; //����ͬ��ʱ�������������

	float TimeSyncRunningTime = 0.f; //�ϴθ���ʱ���ȥ�˶��

	void ChectTimeSync(float DeltaTime);
	void PollInit();

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState(); //��������ֻ���ڷ������Ϸ���gameĪ������������Ҫһ��rpc������ʱ��matchstate

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName MatchS, float Warmup, float MatchT, float LevelStartT, float Cooldown); 
	//һ��Clinet��RPC���𽫷�����ϵ��й���Ϣ���д���
private:
	UPROPERTY()
	class AMyHUD* DuarenHUD;

	float MatchTime = 0.f;//����ʱ��

	float LevelStartingTime = 0.f;
	float WarmupTime = 0.f;
	uint32 CountdownInt = 0; //��¼ʣ��ʱ��
	float CooldownTime= 0;

	float SingleTripTime = 0.f; //ʱ��

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	//�洢��Ҫ��ʼ����hud��������Overlay��ʼ����ʹ����Щֵ��ʼ��
	bool bInitializeHealth = false;
	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeScore = false;
	float HUDScore;
	bool bInitializeDefeats = false;
	int32 HUDDefeats;
	bool bInitializeGrenades = false;
	int32 HUDGrenades;
	bool bInitializeShield = false;
	float HUDShield;
	float HUDMaxShield;
	bool bInitializeCarriedAmmo = false;
	float HUDCarriedAmmo;
	bool bInitializeWeaponAmmo = false;
	float HUDWeaponAmmo;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState; //�洢MatchState�ı���
	UFUNCTION()
	void OnRep_MatchState();
};
