// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* Inpawn)
{
	ENetRole RemoteRole = Inpawn->GetRemoteRole(); //��ȡ��ɫ�ڸÿͻ��˵Ľ�ɫ(ö��)
	FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	case ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy"); //�ͻ��������Ľ�ɫ
		break;
	case ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy"); //�ͻ����Լ��Ľ�ɫ
		break;
	case ROLE_Authority:
		Role = FString("Authority"); //����Ϊ�����
		break;
	default:
		break;
	}
	FString RemoteRoleType = FString::Printf(TEXT("RemoteRoleType: %s"), *Role); //ƴ��
	SetDisplayText(RemoteRoleType);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
