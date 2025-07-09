// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "Duaren/HUD/MyHUD.h"
#include "Duaren/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Duaren/Character/MyCharacter.h"
#include "Duaren/MyComponents/CombatComponent.h"
#include <Net/UnrealNetwork.h>
#include "Kismet/GameplayStatics.h"
#include "Duaren/Weapon/Weapon.h"
#include "Components/Image.h"
#include "Duaren/Mytypes/Announcement.h"
#include "Duaren/GameMode/BlasterGameMode.h"
#include "Duaren/HUD/MyAnnouncement.h"

void AMyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyPlayerController, MatchState);
	//DOREPLIFETIME(AMyPlayerController, bShowTeamScores);
}

void AMyPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->CharacterOverlay && DuarenHUD->CharacterOverlay->HealthBar && DuarenHUD->CharacterOverlay->HealthText;
	if (bHUDValied)
	{
		const float HealthPercent = Health / MaxHealth; //����ֵ�İٷֱ�
		DuarenHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);//���½�������ʾ����
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));//��ʽ������ֵ�ı�
		DuarenHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else//��������Ч���򻺴�����ֵ����
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AMyPlayerController::SetHUDScore(float Score)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->CharacterOverlay && DuarenHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValied)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		DuarenHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void AMyPlayerController::SetHUDDefeats(int32 Defeats)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->CharacterOverlay && DuarenHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValied)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		DuarenHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void AMyPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->CharacterOverlay && DuarenHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValied)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		DuarenHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void AMyPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->CharacterOverlay && DuarenHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValied)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		DuarenHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void AMyPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->CharacterOverlay && DuarenHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValied)
	{
		if (CountdownTime < 0.f)
		{
			DuarenHUD->CharacterOverlay->MatchCountdownText->SetText(FText()); //���С��0�Ͱ��ı�����Ϊ�գ�����һ���νӵ�����
			return;
		}

		int32 Mintues = FMath::FloorToInt(CountdownTime / 60.f); //�������,����ȡ��
		int32 Seconds = CountdownTime - Mintues * 60;

		FString CountdownTimeText = FString::Printf(TEXT("%02d:%02d"), Mintues, Seconds);
		DuarenHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownTimeText));
	}
}

void AMyPlayerController::SetHUDAnnouncementConutdown(float CountdownTime)
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	bool bHUDValied = DuarenHUD && DuarenHUD->Announcement && DuarenHUD->Announcement->WarmTime;
	if (bHUDValied)
	{
		if (CountdownTime < 0.f)
		{
			DuarenHUD->Announcement->WarmTime->SetText(FText()); //���С��0�Ͱ��ı�����Ϊ�գ�����һ���νӵ�����
			return;
		}

		int32 Mintues = FMath::FloorToInt(CountdownTime / 60.f); //�������,����ȡ��
		int32 Seconds = CountdownTime - Mintues * 60;

		FString CountdownTimeText = FString::Printf(TEXT("%02d:%02d"), Mintues, Seconds);
		DuarenHUD->Announcement->WarmTime->SetText(FText::FromString(CountdownTimeText));
	}
}

void AMyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//���������е�HUD����BeginPlayʱ��Ч,�������ǻ�����beginplay�������Ѫ��
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(InPawn);
	if (MyCharacter)
	{
		SetHUDHealth(MyCharacter->GetHealth(), MyCharacter->GetMaxHealth());
		//SetHUDShield(MyCharacter->GetShield(), MyCharacter->GetMaxShield());
		//SetHUDWeaponAmmo(MyCharacter->GetWeaponEquipped()->GetAmmo());
		//SetHUDCarriedAmmo(MyCharacter->GetCombat()->GetCarriedAmmo());
		//SetHUDGrenades(MyCharacter->GetCombat()->GetGrenades());
	}
}

void AMyPlayerController::SetHUDTime()
{
	float Timeleft = 0.f;
	if (MatchState == MatchState::WaitingToStart) Timeleft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) Timeleft = MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) Timeleft = MatchTime + WarmupTime + CooldownTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(Timeleft);

	if (HasAuthority()) //����Ƿ���������ֱ�ӻ�ȡGamemode�ĵ���ʱ�Դ���ȷ��ʱ�����ȷ
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if(BlasterGameMode) SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
	}

	if (CountdownInt != SecondsLeft) //ʱ���Ѿ�����
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementConutdown(Timeleft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(Timeleft);
		}
	}
	CountdownInt = SecondsLeft; //����ʣ��ʱ��
}

void AMyPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	ChectTimeSync(DeltaTime);
	PollInit();
	//CheakPing(DeltaTime);
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	DuarenHUD = Cast<AMyHUD>(GetHUD());
	ServerCheckMatchState();
}

void AMyPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (DuarenHUD && DuarenHUD->CharacterOverlay)
		{
			CharacterOverlay = DuarenHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				//if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				//if (bInitializeGrenades) SetHUDGrenades(HUDGrenades);
				//if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				//if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
			}
		}
	}
}

void AMyPlayerController::ServerCheckMatchState_Implementation()
{
	BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (BlasterGameMode)
	{
		//��Ȼ���ǻ������Щ��������Щֻ���ڷ���˵�������Ҫһ��RPC������Щ�����㵽�ͻ�����
		LevelStartingTime = BlasterGameMode->LevelStartingTime;
		WarmupTime = BlasterGameMode->WarmupTime;
		MatchTime = BlasterGameMode->MatchTime;
		CooldownTime = BlasterGameMode->CooldownTime;
		MatchState = BlasterGameMode->GetMatchState(); //ͨ������˻�ȡ����ܸ��Ƶ��ͻ�����
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime);
	}
}

void AMyPlayerController::ClientJoinMidGame_Implementation(FName MatchS, float Warmup, float MatchT, float LevelStartT, float Cooldown)
{
	LevelStartingTime = LevelStartT;
	WarmupTime = Warmup;
	MatchTime = MatchT;
	MatchState = MatchS;
	CooldownTime = Cooldown;
	OnMatchStateSet(MatchS);
	if (DuarenHUD && MatchState == MatchState::WaitingToStart)
	{
		DuarenHUD->AddAnnouncement();
	}
}

void AMyPlayerController::ChectTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime >= TimeSyncFrequencs)
	{
		ServerRequstServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AMyPlayerController::ServerRequstServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); //��ʱ��������ʱ��
	ClinetReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AMyPlayerController::ClinetReportServerTime_Implementation(float TimeOfClinetRequest, float TimeServeRecivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClinetRequest; //�ͻ��˵���ʱ���Լ���ʱ���ȥ����ʼ��ʱ��������֪������ʱ����
	SingleTripTime = RoundTripTime * 0.5f;
	float ServerCurrentTime = TimeServeRecivedClientRequest + SingleTripTime; //���������˸�������Ϊ������ʱ����ͬ�������÷�����Ӧ���ʱ����Ϸ���ʱ����ܵó���ʱ��������ʱ����
	ClientServerTimeDelta = ServerCurrentTime - GetWorld()->GetTimeSeconds();
}


float AMyPlayerController::GetServerTime()
{
    return GetWorld()->GetTimeSeconds() + ClientServerTimeDelta; //����ͻ���ʱ��
}

void AMyPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequstServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AMyPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasSatate();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AMyPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasSatate();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AMyPlayerController::HandleMatchHasSatate()
{
	//if (HasAuthority()) bShowTeamScores = bTeamsMatch;
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;
	if (DuarenHUD)
	{
		if (DuarenHUD->CharacterOverlay == nullptr) DuarenHUD->AddCharacterOverlay();
		if (DuarenHUD->Announcement)
		{
			DuarenHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AMyPlayerController::HandleCooldown()
{
	DuarenHUD = DuarenHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : DuarenHUD;

	AMyCharacter* MyCharacter = Cast<AMyCharacter>(GetPawn());
	MyCharacter->DisableInput(this);
	if (MyCharacter && MyCharacter->GetCombat())
	{
		MyCharacter->GetCombat()->FireStart(false); //�������ʱ���ڿ���Ӧ�ý�ֹ��
	}

	if (DuarenHUD)
	{
		DuarenHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDBValid = DuarenHUD->Announcement && DuarenHUD->Announcement->AnnouncementText && DuarenHUD->Announcement->WinnerText;
		if (bHUDBValid)
		{
			DuarenHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Game Will Start!");
			DuarenHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			DuarenHUD->Announcement->WinnerText->SetText(FText());
		}
	}
}
