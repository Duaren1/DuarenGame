// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include"GameFramework/SpringArmComponent.h"
#include"Camera/CameraComponent.h"
#include <EnhancedInputSubsystems.h>
#include <EnhancedInputComponent.h>
#include"GameFramework/CharacterMovementComponent.h"
#include"Components/WidgetComponent.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
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
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

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

// Called to bind functionality to input
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

}

void AMyCharacter::Reload()
{
}

void AMyCharacter::StartCrouch()
{

}

void AMyCharacter::Aim()
{
}

void AMyCharacter::StopAim()
{
}

void AMyCharacter::EndCrouch()
{
}

void AMyCharacter::Shoot()
{
}

void AMyCharacter::ShootComplete()
{
}

void AMyCharacter::ThrowGrenade()
{
}

void AMyCharacter::QuitGame()
{
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

