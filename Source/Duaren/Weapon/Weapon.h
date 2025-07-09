// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType) //������ͼ��ʹ��
enum class EWeaponState :uint8
{
	EWS_Inital UMETA(DisplayName = "Inital State"), //��ʼ��״̬
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),

	EWS_MAX UMETA(DisplayName = "DefaultMax") //���ڻ�ȡ�ж��ٸ�״̬
};

UCLASS()
class DUAREN_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override; //���ܼ򵥵���������owner��ֱ�����õ�ҩ����Ϊ��֪���������ĸ��������縴�Ƴɹ�

	void ShowPickupWidget(bool bShowWidget);

	void SetWeaponState(EWeaponState State);

	virtual void Fire(const FVector& HitTarget);

	virtual void Dropped(); //�������������߼�

	void SetHUDAmmo();

	FORCEINLINE class USphereComponent* GetAreaSphere() { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return SkeletalMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomedInterpSpeed() const { return ZoomInterpSpeed; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMaxCapacity() const { return MaxCapacity; }

	void AddAmmo(int32 AmmoToAdd);

	//���ڽ���ӳ�������Ҹ���һ���Ǻ�+
	UFUNCTION(Client, Reliable)
	void Clinet_UpdateAmmo(int32 ServerAmmo);

	//+
	UFUNCTION(Client, Reliable)
	void Cline_AddAmmo(int32 AmmoToAdd);

	//+
	int32 Sequence = 0; //δ����ķ���������(��ҩ���)

	UPROPERTY(EditAnywhere)
	class USoundCue* EquippedSound;

	//׼�ǻ���������ı�
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
	float FireDelay = .15f; //����ӳ�Ҳ��������

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true; //�Ƿ����Զ�����

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

	//������׼��FOV�仯
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	//�������ڴ洢��������owner�������Ϣ��
	UPROPERTY()
	class AMyCharacter* MyOwnerCharacter;

	UPROPERTY()
	class AMyPlayerController* MyOwnerController;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo; //��ҩ��

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MaxCapacity; //��󵯼���

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType; //��������ı���
};

