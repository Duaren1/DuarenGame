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
	//该函数用于定义哪些属性需要在网络同步,这个函数接收一个DOREPLIFETIME宏生成的属性数组，最后在数组中的每个元素都代表一个需要同步的属性

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
	void SimProxiesTurn(); //由于我们的网路复制有延迟所以我们旋转rootbone会导致代理角色抖动因此我们写了这个函数来解决问题

	void Elim(); //用于服务器上的函数

	//伤害事件的回调函数
	UFUNCTION()
	void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigerController, AActor* DamageCauser);
	void UpdateHealth();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Elim(); //用于服务器上的函数

	virtual void Destroyed() override; //记得我们在projectile中用过相同的技巧

	void PollInit(); //playerstate不是在第一帧就初始化的所以不能简单的在beginplay里初始化它，只能通过每帧轮询的方式来初始化

	void OnPlayerStateInitized(); //初始化PlayerState后该做的事

	FORCEINLINE class UCombatComponent* GetCombat() const { return Combat; }
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void AimOffset(float DeltaTime); //计算瞄准偏移

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

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) //当发生网络同步后就会调用这个函数
	class AWeapon* OverlappingWeapons; //服务端发生重叠后将武器复制到该变量中

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon); //这种回调函数可以有参数但只能有一个，这个就是指向发生同步的属性的参数

	//组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	//不可靠RPC无法保证必会到达预定目的地，但其发送速度和频率高于可靠的RPC。其最适用于对gameplay而言不重要或经常调用的函数。
	//可靠的RPC保证到达预定目的地，并在成功接收之前一直保留在队列中。其最适合用于对gameplay很关键或者不经常调用的函数。
	UFUNCTION(Server, Reliable)
	void Serve_Equipped(); //客户端调用服务端执行

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
	class UAnimMontage* FireWeaponMontage; //存储角色的开火蒙太奇

	UPROPERTY(EditAnywhere, Category = "Combat")//受击蒙太奇
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
	float CameraThreshold = 200.f; //标识摄像机离角色多近时该隐藏

	bool bRotateRootbone = false;

	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;

	float TimeSinceLastMovementReplication;

	//玩家血条
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

	//死亡溶解特效
	FOnTimelineFloat DissolveTrack; //Timeline的track

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissloveValue);

	void StartDissolve();

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; //在运行用于动态变化我们的材质的变量

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialInstance; //用于设置材质的变量

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialGlass; 

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialGlass; //用于设置材质的变量

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialEyes;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialEyes; //用于设置材质的变量

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialBrows;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialBrows; //用于设置材质的变量

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialMouth;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialMouth; //用于设置材质的变量

	//用于设置淘汰机器人特效的变量
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	//存储上面设置的变量
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

	ECombatState GetCombatState() const; //获取CombatState	
};
