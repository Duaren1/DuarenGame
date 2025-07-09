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

//��̬���½�ɫ�Ķ�����ز���
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

	FRotator AimRotation = MyCharacter->GetBaseAimRotation(); //������ӽǵ��ƶ�����
	FRotator MovementRoattion = UKismetMathLibrary::MakeRotFromX(MyCharacter->GetVelocity()); //��ɫ���ƶ�����
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRoattion, AimRotation); //����õ�������ƶ��ķ���
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.f); 
	//�ڶ�����ͼ��ֱ��ʹ�ò�ֵ�����������������������ڴ�180��-180�м����ж������ڲ�ͣ���Ȳ��ţ���ʹ�������������ֱ���ҵ���180��-180�����·�������ǻ����仯�Ӷ��ܽ����������������
	YawOffset = DeltaRotation.Yaw;

	LastFrame_CharacterRotation = ThisFrame_CharacterRotation;
	ThisFrame_CharacterRotation = MyCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ThisFrame_CharacterRotation, LastFrame_CharacterRotation); //��ȡ���ƫ�Ʒ���
	const float Target = Delta.Yaw / DeltaTime; //��ʱ��仯
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f); //��ֵ
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	Aim_Offset_Yaw = MyCharacter->GetAOYaw();
	Aim_Offset_Pitch = MyCharacter->GetAOPitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && MyCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FRotator OutRotation;
		FVector OutPosition;
		MyCharacter->GetMesh()->TransformToBoneSpace(FName("RightHand"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		//��һ������ת�������ǽ�ɫmesh��bone��Ҳ���������ǵĽ�ɫ�����������ҵ���ʱ�����������ڽ�ɫ�ϵ�λ��
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		//if (MyCharacter->IsLocallyControlled())
		//{
		//	bIslocallyControlled = true;
		//	FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), RTS_World);
		//	FVector WeaponLoc = RightHandTransform.GetLocation();
		//	// Ŀ��λ�ã������յ㣩
		//	FVector TargetLoc = MyCharacter->GetHitTarget();

		//	FVector TargetDirection = (TargetLoc - WeaponLoc).GetSafeNormal();

		//	// ֱ����ģ�͵�Y�Ḻ�����׼Ŀ��
		//	FRotator Right = UKismetMathLibrary::MakeRotFromY(-TargetDirection);
		//	RightRotation = FMath::RInterpTo(RightRotation, Right, DeltaTime, 30.f);
		//}
		//
		//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Blue);
		////��һ���ߴ�ǹ��ָ��Ŀ��
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MyCharacter->GetHitTarget(), FColor::Orange);

		//if (MyCharacter->IsLocallyControlled())
		//{
		//	bIslocallyControlled = true;
		//	FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("RightHand"), RTS_World);
		//	FRotator Right = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - MyCharacter->GetHitTarget())); //�ñ�������ʹ�������ֵ�����ָ��׼��
		//	RightRoatation = FMath::RInterpTo(RightRoatation, Right, DeltaTime, 30.f);
		//}
	}
	bRotateRootbone = MyCharacter->ShouldRotateRootBone();
	bElimmed = MyCharacter->IsElimed();

	bShouldUseIK = MyCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;

	bShouldUseAimOffset = MyCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
}
