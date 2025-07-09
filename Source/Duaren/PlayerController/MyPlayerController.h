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
	virtual void OnPossess(APawn* InPawn) override; //会在控制一个pawn的时候调用，这样当我们角色重生时就可以更新血条了
	virtual void SetHUDTime();
	virtual void Tick(float DeltaTime) override;
	void CheakPing(float DeltaTime);
	virtual float GetServerTime();
	virtual void ReceivedPlayer() override; //重写这个函数在此处计算我们的时间差能做到尽早同步
	void OnMatchStateSet(FName State, bool bTeamsMatch = false); //用于设置MatchState的函数
	void HandleMatchHasSatate();
	void HandleCooldown();
protected:
	virtual void BeginPlay() override;

	//接下来是用于解决服务器和客户端时间不统一问题的变量
	UFUNCTION(Server, Reliable)
	void ServerRequstServerTime(float TimeOfClientRequest); //用于发送服务器当前时间的函数,但还有延迟因为往返需要时间,发送的参数表示此时的请求时间

	UFUNCTION(Client, Reliable) //服务器调用客户端执行
		void ClinetReportServerTime(float TimeOfClinetRequest, float TimeServeRecivedClientRequest);
	//客户端用于接收服务器时间的函数，我们在其中通过一些计算解决RTT带来的延迟问题

	float ClientServerTimeDelta = 0.f; //服务器与客户端的时间差

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequencs = 5.f; //定期同步时间以免出现意外

	float TimeSyncRunningTime = 0.f; //上次更新时间过去了多久

	void ChectTimeSync(float DeltaTime);
	void PollInit();

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState(); //由于我们只能在服务器上访问game莫得所以我们需要一个rpc来检查此时的matchstate

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName MatchS, float Warmup, float MatchT, float LevelStartT, float Cooldown); 
	//一个Clinet的RPC负责将服务端上的有关信息进行传递
private:
	UPROPERTY()
	class AMyHUD* DuarenHUD;

	float MatchTime = 0.f;//比赛时间

	float LevelStartingTime = 0.f;
	float WarmupTime = 0.f;
	uint32 CountdownInt = 0; //记录剩余时间
	float CooldownTime= 0;

	float SingleTripTime = 0.f; //时延

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	//存储需要初始化的hud变量，等Overlay初始化后，使用这些值初始化
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
	FName MatchState; //存储MatchState的变量
	UFUNCTION()
	void OnRep_MatchState();
};
