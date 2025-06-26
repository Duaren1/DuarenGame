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
	ENetRole RemoteRole = Inpawn->GetRemoteRole(); //获取角色在该客户端的角色(枚举)
	FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	case ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy"); //客户端其他的角色
		break;
	case ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy"); //客户端自己的角色
		break;
	case ROLE_Authority:
		Role = FString("Authority"); //即作为服务端
		break;
	default:
		break;
	}
	FString RemoteRoleType = FString::Printf(TEXT("RemoteRoleType: %s"), *Role); //拼接
	SetDisplayText(RemoteRoleType);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
