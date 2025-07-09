// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType) //能在蓝图中使用
enum class EWeaponState :uint8
{
	EWS_Inital UMETA(DisplayName = "Inital State"), //初始化状态
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),

	EWS_MAX UMETA(DisplayName = "DefaultMax") //用于获取有多少个状态
};

UCLASS()
class DUAREN_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override; //不能简单的在设置了owner后直接设置弹药，因为不知道这两个哪个会先网络复制成功

	void ShowPickupWidget(bool bShowWidget);

	void SetWeaponState(EWeaponState State);

	virtual void Fire(const FVector& HitTarget);

	virtual void Dropped(); //武器掉落的相关逻辑

	void SetHUDAmmo();

	FORCEINLINE class USphereComponent* GetAreaSphere() { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return SkeletalMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomedInterpSpeed() const { return ZoomInterpSpeed; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMaxCapacity() const { return MaxCapacity; }

	void AddAmmo(int32 AmmoToAdd);

	//用于解决延迟问题的我给他一个记号+
	UFUNCTION(Client, Reliable)
	void Clinet_UpdateAmmo(int32 ServerAmmo);

	//+
	UFUNCTION(Client, Reliable)
	void Cline_AddAmmo(int32 AmmoToAdd);

	//+
	int32 Sequence = 0; //未处理的服务器请求(弹药相关)

	UPROPERTY(EditAnywhere)
	class USoundCue* EquippedSound;

	//准星会根据武器改变
	UPROPERTY(EditAnywhere, Category = "CrossHair")
	class UTexture2D* CrossHairsCenter;

	UPROPERTY(EditAnywhere, Category = "CrossHair")
	class UTexture2D* CrossHairsLeft;

	UPROPERTY(EditAnywhere, Category = "CrossHair")
	class UTexture2D* CrossHairsRight;

	UPROPERTY(EditAnywhere, Category = "CrossHair")
	class UTexture2D* CrossHairsTop;

	UPROPERTY(EditAnywhere, Category = "CrossHair")
	class UTexture2D* CrossHairsBottom;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = .15f; //射击延迟也就是射速

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true; //是否是自动武器

	bool IsAmmoEmpty();
	bool IsAmmoFull();

	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
        UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickUpText;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	//武器瞄准的FOV变化
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	//都是用于存储该武器的owner的相关信息的
	UPROPERTY()
	class AMyCharacter* MyOwnerCharacter;

	UPROPERTY()
	class AMyPlayerController* MyOwnerController;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo; //弹药数

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MaxCapacity; //最大弹夹数

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType; //武器种类的变量
};

