// Fill out your copyright notice in the Description page of Project Settings.

#include "MyAnimInstance.h"
#include "MyCharacter.h"
#include"GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Duaren/Weapon/Weapon.h"
#include "Duaren/MyTypes/CombatState.h"

void UMyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	MyCharacter = Cast<AMyCharacter>(TryGetPawnOwner());
}

//动态更新角色的动画相关参数
void UMyAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (MyCharacter == nullptr)
	{
		MyCharacter = Cast<AMyCharacter>(TryGetPawnOwner());
	}
	if (MyCharacter == nullptr) return;

	FVector Velocity = MyCharacter->GetVelocity();
	Velocity.Z = 0;
	speed = Velocity.Size();

	bIsInAir = MyCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = MyCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;
	bWeaponEquipped = MyCharacter->IsWeaponEquipped();
	EquippedWeapon = MyCharacter->GetWeaponEquipped();
	bIsCrouched = MyCharacter->bIsCrouched;
	bIsAiming = MyCharacter->IsAiming();
	TurningInPlace = MyCharacter->GetTurningInPlace();

	FRotator AimRotation = MyCharacter->GetBaseAimRotation(); //摄像机视角的移动方向
	FRotator MovementRoattion = UKismetMathLibrary::MakeRotFromX(MyCharacter->GetVelocity()); //角色的移动方向
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRoattion, AimRotation); //相减得到人物该移动的方向
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.f); 
	//在动画蓝图里直接使用插值会出现向后鬼畜的问题这是由于从180到-180中间所有动画都在不停过度播放，而使用这个函数它能直接找到从180到-180的最短路径而不是缓慢变化从而能解决我们遇到的问题
	YawOffset = DeltaRotation.Yaw;

	LastFrame_CharacterRotation = ThisFrame_CharacterRotation;
	ThisFrame_CharacterRotation = MyCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ThisFrame_CharacterRotation, LastFrame_CharacterRotation); //获取玩家偏移方向
	const float Target = Delta.Yaw / DeltaTime; //随时间变化
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f); //插值
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	Aim_Offset_Yaw = MyCharacter->GetAOYaw();
	Aim_Offset_Pitch = MyCharacter->GetAOPitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && MyCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FRotator OutRotation;
		FVector OutPosition;
		MyCharacter->GetMesh()->TransformToBoneSpace(FName("RightHand"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		//将一个物体转换成我们角色mesh的bone，也就是在我们的角色捡起武器后找到此时武器这个插槽在角色上的位置
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		//if (MyCharacter->IsLocallyControlled())
		//{
		//	bIslocallyControlled = true;
		//	FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), RTS_World);
		//	FVector WeaponLoc = RightHandTransform.GetLocation();
		//	// 目标位置（射线终点）
		//	FVector TargetLoc = MyCharacter->GetHitTarget();

		//	FVector TargetDirection = (TargetLoc - WeaponLoc).GetSafeNormal();

		//	// 直接让模型的Y轴负方向对准目标
		//	FRotator Right = UKismetMathLibrary::MakeRotFromY(-TargetDirection);
		//	RightRotation = FMath::RInterpTo(RightRotation, Right, DeltaTime, 30.f);
		//}
		//
		//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Blue);
		////画一条线从枪口指向目标
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MyCharacter->GetHitTarget(), FColor::Orange);

		//if (MyCharacter->IsLocallyControlled())
		//{
		//	bIslocallyControlled = true;
		//	FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("RightHand"), RTS_World);
		//	FRotator Right = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - MyCharacter->GetHitTarget())); //该变量用于使我们右手的武器指向准星
		//	RightRoatation = FMath::RInterpTo(RightRoatation, Right, DeltaTime, 30.f);
		//}
	}
	bRotateRootbone = MyCharacter->ShouldRotateRootBone();
	bElimmed = MyCharacter->IsElimed();

	bShouldUseIK = MyCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;

	bShouldUseAimOffset = MyCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
}
