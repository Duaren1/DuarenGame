// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include"GameFramework/SpringArmComponent.h"
#include"Camera/CameraComponent.h"
#include <EnhancedInputSubsystems.h>
#include <EnhancedInputComponent.h>
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Duaren/Weapon/Weapon.h"
#include "Duaren/MyComponents/CombatComponent.h"
#include "Duaren/PlayerController/MyPlayerController.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include <Kismet/KismetMathLibrary.h>
#include "Components/CapsuleComponent.h"
#include "particles/ParticleSystemComponent.h"
#include "Duaren/GameMode/BlasterGameMode.h"
#include "Duaren/PlayerState/MyPlayerState.h"
#include "Duaren/Weapon/WeaponTypes.h"
#include "Duaren/Duaren.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh()); //将摄像机臂附加到mesh上而不是根组件，这样蹲下的时候高度就不会改变了
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>("CombatComponent");
	Combat->SetIsReplicated(true); //设置网络复制，组件不需要注册生命周期

	GetMovementComponent()->NavAgentProps.bCanCrouch = true;//默认角色可蹲下

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f; //每次网络同步的频率
	MinNetUpdateFrequency = 33.f;  //当网络复制不频繁时的最低网络更新频率

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyCharacter, Health);
	DOREPLIFETIME_CONDITION(AMyCharacter, OverlappingWeapons, COND_OwnerOnly); //将重叠武器同步到客户端(只同步到用于该角色类的客户端)

}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AMyCharacter::ReceiveDamage);
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* LocalPlayerSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (LocalPlayerSubsystem)
		{
			LocalPlayerSubsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void AMyCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	FVector Velocity = this->GetVelocity();
	Velocity.Z = 0;
	float speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (speed == 0.f && !bIsInAir) //静止不动且在地面时我们才需要aimoffset，移动时我们只需要关注pitch的aimoffset
	{
		bRotateRootbone = true;
		FRotator CurrentAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartAimRotation); //获取此时角色的相对瞄准偏移，当我们停止运动时角色会正视前方，从而通过与静止时的视角偏移做差从而取得相对偏移量
		Aim_Offset_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			Interp_AOYaw = Aim_Offset_Yaw;
		}
		//设置转身
		TurnInPlace(DeltaTime);
	}
	if (speed > 0.f || bIsInAir)
	{
		bRotateRootbone = false;
		StartAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		Aim_Offset_Yaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning; //移动或跳跃时不该转身
	}

	Aim_Offset_Pitch = GetBaseAimRotation().Pitch; //由于ue5通过压缩来进行角度的网络传输，压缩会使得我们的值介于0到360中间，所以我们的负数就变为了正数的最大值，这就是问题所在
	if (Aim_Offset_Pitch > 90 && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90, 0);
		Aim_Offset_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, Aim_Offset_Pitch); //映射,将被改变的数重新映射到正确的数上
	}

}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy&&IsLocallyControlled()) //由于我们打的ENetRole枚举类，因此每个枚举都有其优先级也就是申明枚举时后面的数字，所以我们可以用大于号判断是否处于模拟代理
	{
		AimOffset(DeltaTime);
	}
	else //隔一段时间都会更新一下
	{

		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}

		Aim_Offset_Pitch = GetBaseAimRotation().Pitch;
		if (Aim_Offset_Pitch > 90 && !IsLocallyControlled())
		{
			FVector2D InRange(270.f, 360.f);
			FVector2D OutRange(-90, 0);
			Aim_Offset_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, Aim_Offset_Pitch);
		}
	}
	
	AimOffset(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void AMyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//一个问题：在设置Gamemode中bDelayStart时会导致服务器中的角色的增强输入子系统失效，网上查找发现是这段代码里会将controller判断为空而无法初始化增强输入子系统导致的，在InputComponent这个函数里再判断一次解决问题
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* LocalPlayerSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (LocalPlayerSubsystem)
		{
			LocalPlayerSubsystem->AddMappingContext(MappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* PlayerInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		PlayerInput->BindAction(PlayerMove, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
		PlayerInput->BindAction(PlayerLook, ETriggerEvent::Triggered, this, &AMyCharacter::Look);
		PlayerInput->BindAction(PlayerJump, ETriggerEvent::Started, this, &AMyCharacter::Jump);
		PlayerInput->BindAction(PlayerJump, ETriggerEvent::Completed, this, &AMyCharacter::StopJumping);
		PlayerInput->BindAction(PlayerEquipped, ETriggerEvent::Started, this, &AMyCharacter::Equipped);
		PlayerInput->BindAction(PlayerCrouch, ETriggerEvent::Triggered, this, &AMyCharacter::StartCrouch);
		PlayerInput->BindAction(PlayerCrouch, ETriggerEvent::Completed, this, &AMyCharacter::EndCrouch);
		PlayerInput->BindAction(PlayerAim, ETriggerEvent::Started, this, &AMyCharacter::Aim);
		PlayerInput->BindAction(PlayerAim, ETriggerEvent::Completed, this, &AMyCharacter::StopAim);
		PlayerInput->BindAction(PlayerShoot, ETriggerEvent::Started, this, &AMyCharacter::Shoot);
		PlayerInput->BindAction(PlayerShoot, ETriggerEvent::Completed, this, &AMyCharacter::ShootComplete);
		PlayerInput->BindAction(PlayerReload, ETriggerEvent::Started, this, &AMyCharacter::Reload);
		PlayerInput->BindAction(PlayerThrowGrenade, ETriggerEvent::Started, this, &AMyCharacter::ThrowGrenade);
		PlayerInput->BindAction(PlayerQuit, ETriggerEvent::Started, this, &AMyCharacter::QuitGame);
	}
}

void AMyCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapons)
	{
		OverlappingWeapons->ShowPickupWidget(true); //但是这样服务器上就无法显示了，因为服务器不需要进行网络同步的操作，所以就不可能调用该函数
	}
	else if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void AMyCharacter::OnRep_Health()
{
	UpdateHealth();
	PlayHitReactMonatge();
}

//判断是否装备武器
bool AMyCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AMyCharacter::IsAiming()
{
	return (Combat && Combat->isAiming);
}

AWeapon* AMyCharacter::GetWeaponEquipped()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return nullptr;
	}
	return Combat->EquippedWeapon;
}

void AMyCharacter::TurnInPlace(float DeltaTime)
{
	if (Aim_Offset_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (Aim_Offset_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		Interp_AOYaw = FMath::FInterpTo(Interp_AOYaw, 0.f, DeltaTime, 10.f); //我们在旋转的时候让rootbone乘以ao_yaw的负值从而让角色的下半身体固定，为了能让角色转到，我们在要转身时用插值改变其值为0从而转动
		Aim_Offset_Yaw = Interp_AOYaw;
		if (FMath::Abs(Aim_Offset_Yaw) < 15.f) //查看绝对值是否已经小于一个值，此时就已经转动到位了
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		}
	}
}

void AMyCharacter::PlayFireMonatge(bool bIsAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //获取动画实例
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bIsAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMyCharacter::PlayHitReactMonatge()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //获取动画实例
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMyCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void AMyCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //获取动画实例
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssultRifle:
			SectionName = FName("ReloadRifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("ReloadRocket");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("ReloadPistol");
			break;
		case EWeaponType::EWT_SMG:
			SectionName = FName("ReloadPistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("ReloadShotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("ReloadSniper");
			break;
		case EWeaponType::EWT_GrenadeLanucher:
			SectionName = FName("GrenadeLauncher");
			break;
		default:
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

FVector AMyCharacter::GetHitTarget()
{
	if (!Combat)  return FVector();
	else
	{
		return Combat->HitTarget;
	}
}

void AMyCharacter::OnRep_ReplicateMovement()
{
	Super::OnRep_ReplicatedMovement();
	if (GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
	{
		SimProxiesTurn();
	}
	TimeSinceLastMovementReplication = 0.f;
}

void AMyCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false); //只在本地隐藏，这样网络上其他人仍能看到你操控的角色
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		//if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		//{
		//	Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		//}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
		//if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		//{
		//	Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		//}
	}
}

void AMyCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	bRotateRootbone = false;
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	FVector Velocity = this->GetVelocity();
	Velocity.Z = 0;
	float speed = Velocity.Size();
	if (speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

//注意这个函数仅在服务端执行
void AMyCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigerController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth); //记住Health是网络复制的

	UpdateHealth();
	PlayHitReactMonatge();

	if (Health == 0.f)
	{
		BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			MyPlayerController = MyPlayerController == nullptr ? Cast<AMyPlayerController>(GetController()) : MyPlayerController;
			AMyPlayerController* AttackerPlayerController = Cast<AMyPlayerController>(InstigerController);
			BlasterGameMode->PlayerEliminated(this, MyPlayerController, AttackerPlayerController);
		}
	}
}

void AMyCharacter::UpdateHealth()
{
	MyPlayerController = MyPlayerController == nullptr ? Cast<AMyPlayerController>(Controller) : MyPlayerController;
	if (MyPlayerController)
	{
		MyPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AMyCharacter::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	Multicast_Elim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AMyCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void AMyCharacter::Multicast_Elim_Implementation()
{
	if (MyPlayerController)
	{
		MyPlayerController->SetHUDWeaponAmmo(0);
	}
	//禁用输入
	DisableInput(Cast<APlayerController>(GetController()));
	GetCharacterMovement()->DisableMovement(); //防止重力的作用导致角色被拖拽到地里
	GetCharacterMovement()->StopMovementImmediately();

	//禁用碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//AttachGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bElimmed = true;
	PlayElimMontage();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(3, DynamicDissolveMaterialInstance); //设置材质
		//设置材质参数
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialGlass)
	{
		DynamicDissolveMaterialGlass = UMaterialInstanceDynamic::Create(DissolveMaterialGlass, this);

		GetMesh()->SetMaterial(4, DynamicDissolveMaterialGlass); //设置材质
		//设置材质参数
		DynamicDissolveMaterialGlass->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialGlass->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialEyes)
	{
		DynamicDissolveMaterialEyes = UMaterialInstanceDynamic::Create(DissolveMaterialEyes, this);

		GetMesh()->SetMaterial(1, DynamicDissolveMaterialEyes); //设置材质
		//设置材质参数
		DynamicDissolveMaterialEyes->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialEyes->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialBrows)
	{
		DynamicDissolveMaterialBrows = UMaterialInstanceDynamic::Create(DissolveMaterialBrows, this);

		GetMesh()->SetMaterial(2, DynamicDissolveMaterialBrows); //设置材质
		//设置材质参数
		DynamicDissolveMaterialBrows->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialBrows->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialMouth)
	{
		DynamicDissolveMaterialMouth = UMaterialInstanceDynamic::Create(DissolveMaterialMouth, this);

		GetMesh()->SetMaterial(0, DynamicDissolveMaterialMouth); //设置材质
		//设置材质参数
		DynamicDissolveMaterialMouth->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialMouth->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//生成ElimBot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f); //在玩家头顶200厘米处生成我们的机器人
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	//if (CrownComponent)
	//{
	//	CrownComponent->DestroyComponent();
	//}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void AMyCharacter::ElimTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
	//这里我们没有直接摧毁ElimBot而是在角色Destroyed函数中摧毁，这样我们可以搭个网路上的“顺风车”
}

void AMyCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent(); //我们可以在角色摧毁时搭个“顺风车”
	}
}

void AMyCharacter::PollInit()
{
	if (MyPlayerState == nullptr)
	{
		MyPlayerState = GetPlayerState<AMyPlayerState>(); //如果没有初始化就初始化
		if (MyPlayerState)
		{
			OnPlayerStateInitized();
			//ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			//if (BlasterGameState && BlasterGameState->TopScorePlayers.Contains(BlasterPlayerState))
			//{
			//	MulticastGainedLead();
			//}
		}
	}
}

void AMyCharacter::OnPlayerStateInitized()
{
	MyPlayerState->AddtoScore(0.f);
	MyPlayerState->AddtoDefeats(0);
	//SetTeamColor(BlasterPlayerState->GetTeam());
	//SetSpawnPoint();
}


void AMyCharacter::UpdateDissolveMaterial(float DissloveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //动态改变溶解值
	}
	if (DynamicDissolveMaterialGlass)
	{
		DynamicDissolveMaterialGlass->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //动态改变溶解值
	}
	if (DynamicDissolveMaterialEyes)
	{
		DynamicDissolveMaterialEyes->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //动态改变溶解值
	}
	if (DynamicDissolveMaterialBrows)
	{
		DynamicDissolveMaterialBrows->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //动态改变溶解值
	}
	if (DynamicDissolveMaterialMouth)
	{
		DynamicDissolveMaterialMouth->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //动态改变溶解值
	}
}

void AMyCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AMyCharacter::UpdateDissolveMaterial); //动态绑定一个函数也就是每一帧都会调用
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack); //绑定曲线
		DissolveTimeline->Play();
	}
}

void AMyCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapons) //在未赋值之前我们都将显示文本设置为假，只有赋值后不为空才会显示
	{
		OverlappingWeapons->ShowPickupWidget(false);
	}
	OverlappingWeapons = Weapon;
	if (IsLocallyControlled()) //该函数只有在服务端上才会生效（只有在服务端，我们才设置碰撞）,同时判断是不是服务端拥有的本地角色发生的碰撞(我们让某个人作为服务端时)
	{
		if (OverlappingWeapons)
		{
			OverlappingWeapons->ShowPickupWidget(true);
		}
	}
}

void AMyCharacter::Move(const FInputActionValue& value)
{
	const FVector2D Vector2d = value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, rotation.Yaw, 0);

		//获取向前向量
		const FVector FowardDriction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		//获取向右向量
		const FVector RightDriction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(FowardDriction, Vector2d.X);
		AddMovementInput(RightDriction, Vector2d.Y);
	}
}

void AMyCharacter::Look(const FInputActionValue& value)
{
	const FVector2D Vector2d = value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerPitchInput(Vector2d.Y); //偏航角
		AddControllerYawInput(Vector2d.X); //俯仰角
	}
}

void AMyCharacter::Equipped(const FInputActionValue& value)
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapons);
		}
		else
		{
			Serve_Equipped();
		}
	}
}

void AMyCharacter::Serve_Equipped_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapons);
	}
}

void AMyCharacter::Reload()
{
	if (Combat)
	{
		//if (Combat->bHoldingFlag) return;
		Combat->Reload();
	}
}

void AMyCharacter::StartCrouch()
{
	if (GetMovementComponent()->IsFalling()) return;
	Crouch();
}

void AMyCharacter::Aim()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AMyCharacter::StopAim()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AMyCharacter::EndCrouch()
{
	if (GetMovementComponent()->IsFalling()) return;
	UnCrouch();
}

void AMyCharacter::Shoot()
{
	if (Combat)
	{
		Combat->FireStart(true);
	}
}

void AMyCharacter::ShootComplete()
{
	if (Combat)
	{
		Combat->FireStart(false);
	}
}

void AMyCharacter::ThrowGrenade()
{
}

void AMyCharacter::QuitGame()
{
}

ECombatState AMyCharacter::GetCombatState() const
{
	if (Combat == nullptr)
	{
		return ECombatState::ECS_MAX;
	}
	else
	{
		return Combat->CombatState;
	}
}

