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

	friend class AMyCharacter; //������Ԫ��

	void EquipWeapon(class AWeapon* WeaponToEquipped);

	void FireStart(bool bFire);

	//��װ������һ��������ֻ���Լ��Ŀͻ�����װ�����������������ǻ���Ҫһ��RPC������֪ͨ������ִ�л�������������ͬ�������пͻ���
	void Reload();

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload(); //����������Ϻ����пͻ����ϵ�Reload

	UFUNCTION(BlueprintCallable) //�������������ͼ���ϵ��ã�����ϣ������reload����������call(������montage�м���һ��notify)
	void FinishReload(); //��װ�����������ǻ�Ҫ�����ǵ�״̬���û���

	int32 AmountToReload(); //�����ʱ��Ҫ�����ٵ�

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

	UFUNCTION(Server, Reliable) //������֤
	void ServerFire(const FVector_NetQuantize& TraceHitTarget); // FVector_NetQuantize��Ȼ�ᵼ��С���㶪ʧ�������˴����ռ�ôӶ���С������
	//���fire delay������Ϊ�˽�����֤��ֹ����

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	//����ΪNetMulticastʱ���ڷ������ϵ���������������пͻ��˶���ִ�У�����ڿͻ��˵�����ֻ���ڸÿͻ���ִ��

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
	class AWeapon* EquippedWeapon; //����װ������ֻ�ڷ���˽��У�����Ҫ����ȷ�Ķ�����Ҫͬ��������

	UPROPERTY(Replicated) //���縴���Ƿ���˵��ͻ��˵ĸ��ƣ����Ե��ͻ�����׼ʱ����˲�������׼��ͨ��RPC���
	bool isAiming; //�Ƿ�����׼

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;
	bool bAimingButtonPressed = false; //�Ƿ���İ�������׼��

	bool BFireStart;

	FVector HitTarget;

	float CrossHairVelocityFactor; //׼����ɢ�Ĵ�С
	float CrossHairInAirFactor;
	float CrossHairAimFactor;
	float CrossHairShootFactor;

	FHUDPackage HUDPackage;

	//FOV�йر��� FOV:��Ұ�Ƕ�
	float DefaultFOV; //Ĭ�ϵ�FOV,������׼ʱ��FOV

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;

	float CurrentFOV;

	void InterpFOV(float DeltaTime);

	/*
	������������ļ�ʱ��
	*/
	FTimerHandle FireTimer;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay; //����ӳ�Ҳ��������

	void StartFireTimer();
	void FireTimerFinished(); //��ʱ�������Ļص�����

	bool bCanFire = true; //����ȴ���ʱ����������ܼ�����������ܵ�������

	bool CanFire();

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30; //��ǹ��ʼЯ���ĵ�ҩ

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0; //������ĳ�ʼ��ҩ

	UPROPERTY(EditAnywhere)
	int32 StartingPostolAmmo = 15; //��ǹ�ĳ�ʼ��ҩ

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 15; //���ǹ�ĳ�ʼ��ҩ

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 15; //����ǹ�ĳ�ʼ��ҩ

	UPROPERTY(EditAnywhere)
	int32 StartingSniperRiflegunAmmo = 10; //�ѻ�ǹ��ʼ��ҩ

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLaucherAmmo = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; //���Ϊ��ǰ������Я���ĵ�ҩ��

	TMap<EWeaponType, int32> CarriedAmmoMap; //��ͬ��������Я����ͬ�����ӵ�,����һ��map���洢ӳ���ϵ  ����һ��С֪ʶ��TMap�ǲ�֧������ͬ����

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;  //���Я�����ӵ���

	void InitializeCarriedAmmo(); //��ʼ��Я����ҩ

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
};
