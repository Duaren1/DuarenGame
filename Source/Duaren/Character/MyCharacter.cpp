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
	CameraBoom->SetupAttachment(GetMesh()); //��������۸��ӵ�mesh�϶����Ǹ�������������µ�ʱ��߶ȾͲ���ı���
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
	Combat->SetIsReplicated(true); //�������縴�ƣ��������Ҫע����������

	GetMovementComponent()->NavAgentProps.bCanCrouch = true;//Ĭ�Ͻ�ɫ�ɶ���

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f; //ÿ������ͬ����Ƶ��
	MinNetUpdateFrequency = 33.f;  //�����縴�Ʋ�Ƶ��ʱ������������Ƶ��

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyCharacter, Health);
	DOREPLIFETIME_CONDITION(AMyCharacter, OverlappingWeapons, COND_OwnerOnly); //���ص�����ͬ�����ͻ���(ֻͬ�������ڸý�ɫ��Ŀͻ���)

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

	if (speed == 0.f && !bIsInAir) //��ֹ�������ڵ���ʱ���ǲ���Ҫaimoffset���ƶ�ʱ����ֻ��Ҫ��עpitch��aimoffset
	{
		bRotateRootbone = true;
		FRotator CurrentAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartAimRotation); //��ȡ��ʱ��ɫ�������׼ƫ�ƣ�������ֹͣ�˶�ʱ��ɫ������ǰ�����Ӷ�ͨ���뾲ֹʱ���ӽ�ƫ������Ӷ�ȡ�����ƫ����
		Aim_Offset_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			Interp_AOYaw = Aim_Offset_Yaw;
		}
		//����ת��
		TurnInPlace(DeltaTime);
	}
	if (speed > 0.f || bIsInAir)
	{
		bRotateRootbone = false;
		StartAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		Aim_Offset_Yaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning; //�ƶ�����Ծʱ����ת��
	}

	Aim_Offset_Pitch = GetBaseAimRotation().Pitch; //����ue5ͨ��ѹ�������нǶȵ����紫�䣬ѹ����ʹ�����ǵ�ֵ����0��360�м䣬�������ǵĸ����ͱ�Ϊ�����������ֵ���������������
	if (Aim_Offset_Pitch > 90 && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90, 0);
		Aim_Offset_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, Aim_Offset_Pitch); //ӳ��,�����ı��������ӳ�䵽��ȷ������
	}

}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy&&IsLocallyControlled()) //�������Ǵ��ENetRoleö���࣬���ÿ��ö�ٶ��������ȼ�Ҳ��������ö��ʱ��������֣��������ǿ����ô��ں��ж��Ƿ���ģ�����
	{
		AimOffset(DeltaTime);
	}
	else //��һ��ʱ�䶼�����һ��
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

	//һ�����⣺������Gamemode��bDelayStartʱ�ᵼ�·������еĽ�ɫ����ǿ������ϵͳʧЧ�����ϲ��ҷ�������δ�����Ὣcontroller�ж�Ϊ�ն��޷���ʼ����ǿ������ϵͳ���µģ���InputComponent������������ж�һ�ν������
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
		OverlappingWeapons->ShowPickupWidget(true); //���������������Ͼ��޷���ʾ�ˣ���Ϊ����������Ҫ��������ͬ���Ĳ��������ԾͲ����ܵ��øú���
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

//�ж��Ƿ�װ������
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
		Interp_AOYaw = FMath::FInterpTo(Interp_AOYaw, 0.f, DeltaTime, 10.f); //��������ת��ʱ����rootbone����ao_yaw�ĸ�ֵ�Ӷ��ý�ɫ���°�����̶���Ϊ�����ý�ɫת����������Ҫת��ʱ�ò�ֵ�ı���ֵΪ0�Ӷ�ת��
		Aim_Offset_Yaw = Interp_AOYaw;
		if (FMath::Abs(Aim_Offset_Yaw) < 15.f) //�鿴����ֵ�Ƿ��Ѿ�С��һ��ֵ����ʱ���Ѿ�ת����λ��
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
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //��ȡ����ʵ��
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

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //��ȡ����ʵ��
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

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //��ȡ����ʵ��
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
		GetMesh()->SetVisibility(false); //ֻ�ڱ������أ��������������������ܿ�����ٿصĽ�ɫ
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

//ע������������ڷ����ִ��
void AMyCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigerController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth); //��סHealth�����縴�Ƶ�

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
	//��������
	DisableInput(Cast<APlayerController>(GetController()));
	GetCharacterMovement()->DisableMovement(); //��ֹ���������õ��½�ɫ����ק������
	GetCharacterMovement()->StopMovementImmediately();

	//������ײ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//AttachGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bElimmed = true;
	PlayElimMontage();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(3, DynamicDissolveMaterialInstance); //���ò���
		//���ò��ʲ���
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialGlass)
	{
		DynamicDissolveMaterialGlass = UMaterialInstanceDynamic::Create(DissolveMaterialGlass, this);

		GetMesh()->SetMaterial(4, DynamicDissolveMaterialGlass); //���ò���
		//���ò��ʲ���
		DynamicDissolveMaterialGlass->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialGlass->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialEyes)
	{
		DynamicDissolveMaterialEyes = UMaterialInstanceDynamic::Create(DissolveMaterialEyes, this);

		GetMesh()->SetMaterial(1, DynamicDissolveMaterialEyes); //���ò���
		//���ò��ʲ���
		DynamicDissolveMaterialEyes->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialEyes->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialBrows)
	{
		DynamicDissolveMaterialBrows = UMaterialInstanceDynamic::Create(DissolveMaterialBrows, this);

		GetMesh()->SetMaterial(2, DynamicDissolveMaterialBrows); //���ò���
		//���ò��ʲ���
		DynamicDissolveMaterialBrows->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialBrows->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialMouth)
	{
		DynamicDissolveMaterialMouth = UMaterialInstanceDynamic::Create(DissolveMaterialMouth, this);

		GetMesh()->SetMaterial(0, DynamicDissolveMaterialMouth); //���ò���
		//���ò��ʲ���
		DynamicDissolveMaterialMouth->SetScalarParameterValue(TEXT("Dissolve"), -0.55);
		DynamicDissolveMaterialMouth->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//����ElimBot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f); //�����ͷ��200���״��������ǵĻ�����
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
	//��������û��ֱ�Ӵݻ�ElimBot�����ڽ�ɫDestroyed�����дݻ٣��������ǿ��Դ����·�ϵġ�˳�糵��
}

void AMyCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent(); //���ǿ����ڽ�ɫ�ݻ�ʱ�����˳�糵��
	}
}

void AMyCharacter::PollInit()
{
	if (MyPlayerState == nullptr)
	{
		MyPlayerState = GetPlayerState<AMyPlayerState>(); //���û�г�ʼ���ͳ�ʼ��
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
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //��̬�ı��ܽ�ֵ
	}
	if (DynamicDissolveMaterialGlass)
	{
		DynamicDissolveMaterialGlass->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //��̬�ı��ܽ�ֵ
	}
	if (DynamicDissolveMaterialEyes)
	{
		DynamicDissolveMaterialEyes->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //��̬�ı��ܽ�ֵ
	}
	if (DynamicDissolveMaterialBrows)
	{
		DynamicDissolveMaterialBrows->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //��̬�ı��ܽ�ֵ
	}
	if (DynamicDissolveMaterialMouth)
	{
		DynamicDissolveMaterialMouth->SetScalarParameterValue(TEXT("Dissolve"), DissloveValue); //��̬�ı��ܽ�ֵ
	}
}

void AMyCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AMyCharacter::UpdateDissolveMaterial); //��̬��һ������Ҳ����ÿһ֡�������
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack); //������
		DissolveTimeline->Play();
	}
}

void AMyCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapons) //��δ��ֵ֮ǰ���Ƕ�����ʾ�ı�����Ϊ�٣�ֻ�и�ֵ��Ϊ�ղŻ���ʾ
	{
		OverlappingWeapons->ShowPickupWidget(false);
	}
	OverlappingWeapons = Weapon;
	if (IsLocallyControlled()) //�ú���ֻ���ڷ�����ϲŻ���Ч��ֻ���ڷ���ˣ����ǲ�������ײ��,ͬʱ�ж��ǲ��Ƿ����ӵ�еı��ؽ�ɫ��������ײ(������ĳ������Ϊ�����ʱ)
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

		//��ȡ��ǰ����
		const FVector FowardDriction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		//��ȡ��������
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
		AddControllerPitchInput(Vector2d.Y); //ƫ����
		AddControllerYawInput(Vector2d.X); //������
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

