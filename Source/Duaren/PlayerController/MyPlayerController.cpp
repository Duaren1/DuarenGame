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
		const float HealthPercent = Health / MaxHealth; //生命值的百分比
		DuarenHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);//更新进度条显示比例
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));//格式化生命值文本
		DuarenHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else//如果组件无效，则缓存生命值数据
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
			DuarenHUD->CharacterOverlay->MatchCountdownText->SetText(FText()); //如果小于0就把文本设置为空，这是一个衔接的问题
			return;
		}

		int32 Mintues = FMath::FloorToInt(CountdownTime / 60.f); //计算分钟,向下取整
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
			DuarenHUD->Announcement->WarmTime->SetText(FText()); //如果小于0就把文本设置为空，这是一个衔接的问题
			return;
		}

		int32 Mintues = FMath::FloorToInt(CountdownTime / 60.f); //计算分钟,向下取整
		int32 Seconds = CountdownTime - Mintues * 60;

		FString CountdownTimeText = FString::Printf(TEXT("%02d:%02d"), Mintues, Seconds);
		DuarenHUD->Announcement->WarmTime->SetText(FText::FromString(CountdownTimeText));
	}
}

void AMyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//并不是所有的HUD都在BeginPlay时有效,所以我们还得在beginplay后更新下血条
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

	if (HasAuthority()) //如果是服务器我们直接获取Gamemode的倒计时以此来确保时间更精确
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if(BlasterGameMode) SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
	}

	if (CountdownInt != SecondsLeft) //时间已经更新
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
	CountdownInt = SecondsLeft; //更新剩余时间
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
		//虽然我们获得了这些变量但这些只是在服务端的我们需要一个RPC来将这些变量搞到客户端上
		LevelStartingTime = BlasterGameMode->LevelStartingTime;
		WarmupTime = BlasterGameMode->WarmupTime;
		MatchTime = BlasterGameMode->MatchTime;
		CooldownTime = BlasterGameMode->CooldownTime;
		MatchState = BlasterGameMode->GetMatchState(); //通过服务端获取后就能复制到客户端啦
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
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); //此时服务器的时间
	ClinetReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AMyPlayerController::ClinetReportServerTime_Implementation(float TimeOfClinetRequest, float TimeServeRecivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClinetRequest; //客户端调用时用自己的时间减去请求开始的时间这样就知道往返时间了
	SingleTripTime = RoundTripTime * 0.5f;
	float ServerCurrentTime = TimeServeRecivedClientRequest + SingleTripTime; //这里仍做了个近似认为往返的时间相同，我们用服务器应答的时间加上返回时间就能得出此时服务器的时间了
	ClientServerTimeDelta = ServerCurrentTime - GetWorld()->GetTimeSeconds();
}


float AMyPlayerController::GetServerTime()
{
    return GetWorld()->GetTimeSeconds() + ClientServerTimeDelta; //计算客户端时间
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
		MyCharacter->GetCombat()->FireStart(false); //如果结束时仍在开火应该禁止它
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
