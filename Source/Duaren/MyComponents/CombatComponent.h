// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Duaren/HUD/MyHUD.h"
#include "Duaren/Weapon/WeaponTypes.h"
#include "Duaren/MyTypes/CombatState.h"
#include "CombatComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUAREN_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	friend class AMyCharacter; //声明友元类

	void EquipWeapon(class AWeapon* WeaponToEquipped);

	void FireStart(bool bFire);

	//和装备武器一样，我们只在自己的客户端上装备了武器，所以我们还需要一个RPC函数来通知服务器执行换弹操作进而能同步给所有客户端
	void Reload();

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload(); //处理服务器上和所有客户端上的Reload

	UFUNCTION(BlueprintCallable) //这个函数将在蓝图上上调用，我们希望它在reload动画结束后被call(我们在montage中加了一个notify)
	void FinishReload(); //在装弹结束后我们还要将我们的状态设置回来

	int32 AmountToReload(); //计算此时需要换多少弹

	void ReloadEmptyWeapon();

	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void Fire();

	UFUNCTION(Server, Reliable) //进行验证
	void ServerFire(const FVector_NetQuantize& TraceHitTarget); // FVector_NetQuantize虽然会导致小数点丢失但降低了带宽的占用从而减小了消耗
	//添加fire delay参数是为了进行验证防止作弊

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	//声明为NetMulticast时，在服务器上调用则服务器和所有客户端都会执行，如果在客户端调用则只会在该客户端执行

	void TraceUnderCrossHair(FHitResult& TraceHitResult);

	void SetHudCrossHairs(float DeltaTime);

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UFUNCTION()
	void OnRep_CombatState();

private:
	UPROPERTY()
	class AMyCharacter* Character;

	UPROPERTY()
	class AMyPlayerController* Controller;

	UPROPERTY()
	class AMyHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon; //由于装备武器只在服务端进行，所以要用正确的动画需要同步该属性

	UPROPERTY(Replicated) //网络复制是服务端到客户端的复制，所以当客户端瞄准时服务端并不会瞄准，通过RPC解决
	bool isAiming; //是否在瞄准

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;
	bool bAimingButtonPressed = false; //是否真的按下了瞄准键

	bool BFireStart;

	FVector HitTarget;

	float CrossHairVelocityFactor; //准星扩散的大小
	float CrossHairInAirFactor;
	float CrossHairAimFactor;
	float CrossHairShootFactor;

	FHUDPackage HUDPackage;

	//FOV有关变量 FOV:视野角度
	float DefaultFOV; //默认的FOV,即不瞄准时的FOV

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;

	float CurrentFOV;

	void InterpFOV(float DeltaTime);

	/*
	用于连续射击的计时器
	*/
	FTimerHandle FireTimer;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay; //射击延迟也就是射速

	void StartFireTimer();
	void FireTimerFinished(); //计时器结束的回调函数

	bool bCanFire = true; //必须等待计时器结束后才能继续射击，不能点多快射多快

	bool CanFire();

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30; //步枪初始携带的弹药

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0; //火箭弹的初始弹药

	UPROPERTY(EditAnywhere)
	int32 StartingPostolAmmo = 15; //手枪的初始弹药

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 15; //冲锋枪的初始弹药

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 15; //霰弹枪的初始弹药

	UPROPERTY(EditAnywhere)
	int32 StartingSniperRiflegunAmmo = 10; //狙击枪初始弹药

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLaucherAmmo = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; //玩家为当前武器所携带的弹药量

	TMap<EWeaponType, int32> CarriedAmmoMap; //不同武器类型携带不同类型子弹,建立一个map来存储映射关系  还有一个小知识点TMap是不支持网络同步的

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;  //最大携带的子弹数

	void InitializeCarriedAmmo(); //初始化携带弹药

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
};
