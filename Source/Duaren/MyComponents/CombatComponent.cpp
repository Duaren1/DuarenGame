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
		EquippedWeapon->Dropped(); //捡起武器时如果此时已有武器，则先丢弃
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
			InitializeCarriedAmmo(); //仅在服务器上操控弹药map
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
	//if (Character->IsLocallyControlled()) bAimingButtonPressed = bIsAiming; //只在本地上进行
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
		//为了确保一定是先禁用物理再Attach 我们再客户端这里也做Attach这一环节而不是只依靠网路复制，因为网络复制不一定保证先后
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); //设置武器状态

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
		StartFireTimer();//每次射击都要进行计时
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
	); //将二维坐标转换为三维
	if (bScreenToWorld)
	{
		FVector Start = CrossHairWorldPosition;
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrossHairWorldDriction * (DistanceToCharacter + 100.f); //我们将我们的线性追踪的起点提到角色面前，这样就不至于在一些特殊情况下检测到自己的角色，或者其他角色站到摄像机和角色中间导致子弹追踪的问题
		}
		FVector End = Start + CrossHairWorldDriction * TRACELINE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit) //未击中仍需要一个end以便我们发射子弹
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}
		else
		{
			HitTarget = TraceHitResult.ImpactPoint;
		}
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UMyInterface>()) //如果有击中Actor且该actor有接口（只有我们的MyCharacter角色有）
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
			//设置扩散
			//[0,600] -> [0,1]
			FVector2D WalkSpeedRnage(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMapRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrossHairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRnage, VelocityMapRange, Velocity.Size());//映射我们的速度到0到1
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
	//自动换弹
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
		HandleReload(); //本地执行一次
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}
	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled()) HandleReload(); //这个是在服务器上调用的
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
		//结束后在更新换弹动画
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
		CombatState = ECombatState::ECS_Unoccupied; //仅在服务端上改变，回忆下它是个网络复制的变量
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
	int RoomInMag = EquippedWeapon->GetMaxCapacity() - EquippedWeapon->GetAmmo(); //此时弹夹还有多少空间

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(AmountCarried, RoomInMag); //找出两者较小的那个，因为我们所携带的弹药可能不足以填满整个弹夹
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
			HandleReload(); //一旦服务器上的state改变，那么所有的客户端又都会执行这个函数，这样实现了全客户端同步
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