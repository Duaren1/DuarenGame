// Fill out your copyright notice in the Description page of Project Settings.
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include <EnhancedInputLibrary.h>
#include <Duaren/Weapon/Weapon.h>
#include "Duaren/MyTypes/TurningInPlace.h"
#include "Duaren/Interface/MyInterface.h"
#include "Components/TimelineComponent.h"
#include "Duaren/MyTypes/CombatState.h"
#include "Duaren/MyTypes/Teams.h"
#include "MyCharacter.generated.h"

//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);
//
//class UInputMappingContext;
//class UInputAction;

UCLASS()
class DUAREN_API AMyCharacter : public ACharacter, public IMyInterface
{
	GENERATED_BODY()

public:
	AMyCharacter();

	//UFUNCTION(NetMulticast, Unreliable)
	//void MulticastHit();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	//�ú������ڶ�����Щ������Ҫ������ͬ��,�����������һ��DOREPLIFETIME�����ɵ��������飬����������е�ÿ��Ԫ�ض�����һ����Ҫͬ��������

	virtual void PostInitializeComponents() override;

	FORCEINLINE void SetOverlappingWeapon(AWeapon* Weapon);

	void PlayFireMonatge(bool bIsAiming);
	void PlayHitReactMonatge();
	void PlayElimMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();

	FVector GetHitTarget();

	virtual void OnRep_ReplicateMovement() override;

	void HideCameraIfCharacterClose();
	void SimProxiesTurn(); //�������ǵ���·�������ӳ�����������תrootbone�ᵼ�´����ɫ�����������д������������������

	void Elim(); //���ڷ������ϵĺ���

	//�˺��¼��Ļص�����
	UFUNCTION()
	void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigerController, AActor* DamageCauser);
	void UpdateHealth();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Elim(); //���ڷ������ϵĺ���

	virtual void Destroyed() override; //�ǵ�������projectile���ù���ͬ�ļ���

	void PollInit(); //playerstate�����ڵ�һ֡�ͳ�ʼ�������Բ��ܼ򵥵���beginplay���ʼ������ֻ��ͨ��ÿ֡��ѯ�ķ�ʽ����ʼ��

	void OnPlayerStateInitized(); //��ʼ��PlayerState���������

	FORCEINLINE class UCombatComponent* GetCombat() const { return Combat; }
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void AimOffset(float DeltaTime); //������׼ƫ��

	UFUNCTION()
	void Move(const FInputActionValue& value);

	UFUNCTION()
	void Look(const FInputActionValue& value);

	UFUNCTION()
	void Equipped(const FInputActionValue& value);

	UFUNCTION()
	void Reload();

	UFUNCTION()
	void StartCrouch();

	UFUNCTION()
	void Aim();

	UFUNCTION()
	void StopAim();

	UFUNCTION()
	void EndCrouch();

	UFUNCTION()
	void Shoot();

	UFUNCTION()
	void ShootComplete();

	UFUNCTION()
	void ThrowGrenade();

	UFUNCTION()
	void QuitGame();

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	//HUD
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) //����������ͬ����ͻ�����������
	class AWeapon* OverlappingWeapons; //����˷����ص����������Ƶ��ñ�����

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon); //���ֻص����������в�����ֻ����һ�����������ָ����ͬ�������ԵĲ���

	//���
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	//���ɿ�RPC�޷���֤�ػᵽ��Ԥ��Ŀ�ĵأ����䷢���ٶȺ�Ƶ�ʸ��ڿɿ���RPC�����������ڶ�gameplay���Բ���Ҫ�򾭳����õĺ�����
	//�ɿ���RPC��֤����Ԥ��Ŀ�ĵأ����ڳɹ�����֮ǰһֱ�����ڶ����С������ʺ����ڶ�gameplay�ܹؼ����߲��������õĺ�����
	UFUNCTION(Server, Reliable)
	void Serve_Equipped(); //�ͻ��˵��÷����ִ��

	//UPROPERTY(EditAnywhere)
	//TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	//UPROPERTY()
	//class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr <UInputMappingContext> MappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerLook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerEquipped;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerCrouch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerAim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerShoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerReload;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerThrowGrenade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PlayerQuit;

	float Aim_Offset_Yaw;
	float Interp_AOYaw;
	float Aim_Offset_Pitch;

	FRotator StartAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* FireWeaponMontage; //�洢��ɫ�Ŀ�����̫��

	UPROPERTY(EditAnywhere, Category = "Combat")//�ܻ���̫��
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* SwapMontage;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraThreshold = 200.f; //��ʶ��������ɫ���ʱ������

	bool bRotateRootbone = false;

	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;

	float TimeSinceLastMovementReplication;

	//���Ѫ��
	UPROPERTY(EditAnywhere, Category = "Player States")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, ReplicatedUsing = OnRep_Health, Category = "Player States")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class AMyPlayerController* MyPlayerController;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	bool bElimmed = false;
	FTimerHandle ElimTimer;
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 2.f;
	void ElimTimerFinished();

	//�����ܽ���Ч
	FOnTimelineFloat DissolveTrack; //Timeline��track

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissloveValue);

	void StartDissolve();

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; //���������ڶ�̬�仯���ǵĲ��ʵı���

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialInstance; //�������ò��ʵı���

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialGlass; 

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialGlass; //�������ò��ʵı���

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialEyes;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialEyes; //�������ò��ʵı���

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialBrows;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialBrows; //�������ò��ʵı���

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialMouth;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialMouth; //�������ò��ʵı���

	//����������̭��������Ч�ı���
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	//�洢�������õı���
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class AMyPlayerState* MyPlayerState;
public:
	bool IsWeaponEquipped();

	bool IsAiming();

	FORCEINLINE float GetAOYaw()const { return Aim_Offset_Yaw; }
	FORCEINLINE float GetAOPitch()const { return Aim_Offset_Pitch; }

	AWeapon* GetWeaponEquipped();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE class UCameraComponent* GetPlayerCamera()const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() { return bRotateRootbone; }
	FORCEINLINE bool IsElimed() const { return bElimmed; }

	FORCEINLINE float GetHealth()const { return Health; }
	FORCEINLINE void SetHealth(float HealthToSet) { Health = HealthToSet; }
	FORCEINLINE float GetMaxHealth()const { return MaxHealth; }

	ECombatState GetCombatState() const; //��ȡCombatState	
};
