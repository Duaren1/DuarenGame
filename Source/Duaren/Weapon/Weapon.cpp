// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include"Components/SphereComponent.h"
#include"Components/WidgetComponent.h"
#include "Duaren/Character/MyCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMeshSocket.h"
#include "WeaponTypes.h"
#include "Casing.h"
#include "Duaren/PlayerController/MyPlayerController.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; //启用网络同步，也就是将当前状态复制给各客户端进行同步

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponSkelectal");
	SkeletalMesh->SetupAttachment(RootComponent);
	SetRootComponent(SkeletalMesh);

	SkeletalMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block); //对所有的通道的碰撞类型均为阻挡
	SkeletalMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore); //对于角色则是忽略碰撞
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); //一开始禁用碰撞

	AreaSphere = CreateDefaultSubobject<USphereComponent>("AreaSphere");
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //我们希望在服务端进行检测，所以这里先设置成忽略
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickUpText = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpText"));
	PickUpText->SetupAttachment(RootComponent);
}

bool AWeapon::IsAmmoEmpty()
{
	if (Ammo <= 0) return true;
	return false;
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
	if (PickUpText)
	{
		PickUpText->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		MyOwnerCharacter = nullptr;
		MyOwnerController = nullptr;
	}
	else
	{
		MyOwnerCharacter = MyOwnerCharacter == nullptr ? Cast<AMyCharacter>(Owner) : MyOwnerCharacter;
		if (MyOwnerCharacter && MyOwnerCharacter->GetWeaponEquipped()) //只有当捡起主武器的时候才更新HUD
		{
			SetHUDAmmo();
		}
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(OtherActor);
	if (MyCharacter && PickUpText)
	{
		MyCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AMyCharacter* MyCharcter = Cast<AMyCharacter>(OtherActor);
	if (MyCharcter)
	{
		MyCharcter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMesh->SetSimulatePhysics(false);
		SkeletalMesh->SetEnableGravity(false);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		SkeletalMesh->SetSimulatePhysics(true);
		SkeletalMesh->SetEnableGravity(true);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		SkeletalMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		APawn* SpawnInstigator = Cast<APawn>(GetOwner());
		const USkeletalMeshSocket* AmmoEjectSocket = GetWeaponMesh()->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform AmmoEjectSocketTransform = AmmoEjectSocket->GetSocketTransform(GetWeaponMesh());

			UWorld* world = GetWorld();
			if (world)
			{
				world->SpawnActor<ACasing>(
					CasingClass,
					AmmoEjectSocketTransform.GetLocation(),
					AmmoEjectSocketTransform.GetRotation().Rotator()
				);
			}
		}
	}
	SpendRound(); //消耗一枚子弹
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true); //保持世界位置
	SkeletalMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	MyOwnerCharacter = nullptr;
	MyOwnerController = nullptr;
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		SkeletalMesh->SetSimulatePhysics(false);
		SkeletalMesh->SetEnableGravity(false);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		SkeletalMesh->SetSimulatePhysics(true);
		SkeletalMesh->SetEnableGravity(true);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

void AWeapon::OnRep_Ammo()
{
	MyOwnerCharacter = MyOwnerCharacter == nullptr ? Cast<AMyCharacter>(GetOwner()) : MyOwnerCharacter;
	if (MyOwnerCharacter)
	{
		MyOwnerController = MyOwnerController == nullptr ? Cast<AMyPlayerController>(MyOwnerCharacter->Controller) : MyOwnerController;
		if (MyOwnerController)
		{
			MyOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SetHUDAmmo()
{
	MyOwnerCharacter = MyOwnerCharacter == nullptr ? Cast<AMyCharacter>(GetOwner()) : MyOwnerCharacter;
	if (MyOwnerCharacter)
	{
		MyOwnerController = MyOwnerController == nullptr ? Cast<AMyPlayerController>(MyOwnerCharacter->Controller) : MyOwnerController;
		if (MyOwnerController)
		{
			MyOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MaxCapacity);
	SetHUDAmmo(); //服务器上更新下HUD
	Cline_AddAmmo(AmmoToAdd);
}

void AWeapon::Cline_AddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MaxCapacity);
	//这是我们之前删除的On_Rep的内容
	MyOwnerCharacter = MyOwnerCharacter == nullptr ? Cast<AMyCharacter>(GetOwner()) : MyOwnerCharacter;
	SetHUDAmmo();
}

void AWeapon::Clinet_UpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence; //此时sequence代表还有多少弹药我们还未同步，故我们先减去这些未同步的弹药,从而实现客户端的预测（也就是客户端先去减了弹药，再等待服务器回应）
	SetHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MaxCapacity);
	SetHUDAmmo();
	if (HasAuthority()) //（服务器协调） 此时如果在服务器执行
	{
		Clinet_UpdateAmmo(Ammo); //通知客户端,用权威数据更新客户端上的数据
	}
	else
	{
		++Sequence; //如果此时在客户端，请求加一(相当于将要减去的弹药)
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickUpText)
	{
		PickUpText->SetVisibility(bShowWidget);
	}
}


