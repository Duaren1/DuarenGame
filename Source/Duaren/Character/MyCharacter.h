// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include <EnhancedInputLibrary.h>
#include "MyCharacter.generated.h"

UCLASS()
class DUAREN_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

    // Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	//HUD
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

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

};
