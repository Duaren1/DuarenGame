// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Duaren/Weapon/Weapon.h"
#include "Duaren/Character/MyCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Duaren/Weapon/WeaponTypes.h"
#include "Sound/SoundCue.h"
#include "Duaren/PlayerController/MyPlayerController.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, isAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquipped)
{
	if (Character == nullptr || WeaponToEquipped == nullptr)
	{
		return;
	}
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped(); //��������ʱ�����ʱ�������������ȶ���
	}
	EquippedWeapon = WeaponToEquipped;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<AMyPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	if (EquippedWeapon->EquippedSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquippedSound,
			Character->GetActorLocation()
		);
	}
	ReloadEmptyWeapon();
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetPlayerCamera())
		{
			DefaultFOV = Character->GetPlayerCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo(); //���ڷ������ϲٿص�ҩmap
		}
		//UpdateHUDGrenades();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	isAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	//if (Character->IsLocallyControlled()) bAimingButtonPressed = bIsAiming; //ֻ�ڱ����Ͻ���
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	isAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (Character && EquippedWeapon)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		//Ϊ��ȷ��һ�����Ƚ���������Attach �����ٿͻ�������Ҳ��Attach��һ���ڶ�����ֻ������·���ƣ���Ϊ���縴�Ʋ�һ����֤�Ⱥ�
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); //��������״̬

		if (EquippedWeapon->EquippedSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EquippedSound,
				Character->GetActorLocation()
			);
		}

	}
}

void UCombatComponent::FireStart(bool bFire)
{
	BFireStart = bFire;
	if (BFireStart && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		ServerFire(HitTarget);
		if (EquippedWeapon)
		{
			CrossHairShootFactor = 1.f;
		}
		StartFireTimer();//ÿ�������Ҫ���м�ʱ
		bCanFire = false;
	}
}

void UCombatComponent::TraceUnderCrossHair(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrossHairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDriction;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrossHairLocation,
		CrossHairWorldPosition,
		CrossHairWorldDriction
	); //����ά����ת��Ϊ��ά
	if (bScreenToWorld)
	{
		FVector Start = CrossHairWorldPosition;
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrossHairWorldDriction * (DistanceToCharacter + 100.f); //���ǽ����ǵ�����׷�ٵ�����ᵽ��ɫ��ǰ�������Ͳ�������һЩ��������¼�⵽�Լ��Ľ�ɫ������������ɫվ��������ͽ�ɫ�м䵼���ӵ�׷�ٵ�����
		}
		FVector End = Start + CrossHairWorldDriction * TRACELINE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit) //δ��������Ҫһ��end�Ա����Ƿ����ӵ�
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}
		else
		{
			HitTarget = TraceHitResult.ImpactPoint;
		}
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UMyInterface>()) //����л���Actor�Ҹ�actor�нӿڣ�ֻ�����ǵ�MyCharacter��ɫ�У�
		{
			HUDPackage.LinearColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.LinearColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHudCrossHairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<AMyPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<AMyHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrossHairCenter = EquippedWeapon->CrossHairsCenter;
				HUDPackage.CrossHairBottom = EquippedWeapon->CrossHairsBottom;
				HUDPackage.CrossHairLeft = EquippedWeapon->CrossHairsLeft;
				HUDPackage.CrossHairRight = EquippedWeapon->CrossHairsRight;
				HUDPackage.CrossHairTop = EquippedWeapon->CrossHairsTop;
			}
			else
			{
				HUDPackage.CrossHairCenter = nullptr;
				HUDPackage.CrossHairBottom = nullptr;
				HUDPackage.CrossHairLeft = nullptr;
				HUDPackage.CrossHairRight = nullptr;
				HUDPackage.CrossHairTop = nullptr;
			}
			//������ɢ
			//[0,600] -> [0,1]
			FVector2D WalkSpeedRnage(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMapRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrossHairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRnage, VelocityMapRange, Velocity.Size());//ӳ�����ǵ��ٶȵ�0��1
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrossHairInAirFactor = FMath::FInterpTo(CrossHairVelocityFactor, 2.25f, DeltaTime, 22.5f);
			}
			else
			{
				CrossHairInAirFactor = FMath::FInterpTo(CrossHairVelocityFactor, 0.f, DeltaTime, 30.f);
			}

			if (isAiming)
			{
				CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, -0.65f, DeltaTime, 30.f);
			}
			else
			{
				CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0, DeltaTime, 30.f);
			}

			CrossHairShootFactor = FMath::FInterpTo(CrossHairShootFactor, 0.f, DeltaTime, 20.f);

			HUDPackage.CrossHairSpread =
				0.5f +
				CrossHairVelocityFactor +
				CrossHairInAirFactor +
				CrossHairAimFactor +
				CrossHairShootFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<AMyPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
		return;

	if (isAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomedInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetPlayerCamera())
	{
		Character->GetPlayerCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (BFireStart && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	//�Զ�����
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPostolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperRiflegunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLanucher, StartingGrenadeLaucherAmmo);
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMonatge(isAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		SetHudCrossHairs(DeltaTime);
		FHitResult HitResult;
		TraceUnderCrossHair(HitResult);
		HitTarget = HitResult.ImpactPoint;
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerReload();
		HandleReload(); //����ִ��һ��
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}
	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled()) HandleReload(); //������ڷ������ϵ��õ�
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

void UCombatComponent::FinishReload()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		//�������ڸ��»�������
		if (Character == nullptr || EquippedWeapon == nullptr)
		{
			return;
		}
		int32 ReloadAmount = AmountToReload();
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
			CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		}
		EquippedWeapon->AddAmmo(ReloadAmount);
		Controller = Controller == nullptr ? Cast<AMyPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDCarriedAmmo(CarriedAmmo);
		}
		CombatState = ECombatState::ECS_Unoccupied; //���ڷ�����ϸı䣬���������Ǹ����縴�Ƶı���
	}
	if (BFireStart)
	{
		Fire();
	}
	//bLocallyReload = false;
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int RoomInMag = EquippedWeapon->GetMaxCapacity() - EquippedWeapon->GetAmmo(); //��ʱ���л��ж��ٿռ�

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(AmountCarried, RoomInMag); //�ҳ����߽�С���Ǹ�����Ϊ������Я���ĵ�ҩ���ܲ�����������������
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsAmmoEmpty())
	{
		Reload();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		if (BFireStart)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) 
			HandleReload(); //һ���������ϵ�state�ı䣬��ô���еĿͻ����ֶ���ִ���������������ʵ����ȫ�ͻ���ͬ��
		break;
	//case ECombatState::ECS_ThrowingGrenade:
	//	if (Character && !Character->IsLocallyControlled())
	//	{
	//		Character->PlayThrowGrenadeMontage();
	//		AttachWeaponToLeft(EquippedWeapon);
	//		ShowAttachGrenade(true);
	//	}
	//	break;
	//case ECombatState::ECS_SwappingWeapons:
	//	if (Character && !Character->IsLocallyControlled())
	//	{
	//		Character->PlaySwapMontage();

	//	}
	//	break;
	}
}