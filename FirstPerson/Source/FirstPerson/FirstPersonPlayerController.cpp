// Copyright Epic Games, Inc. All Rights Reserved.


#include "FirstPersonPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "FirstPersonCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "FirstPerson.h"
#include "TeamGameState.h"  // 必须包含！否则 ATeamGameState 是不完整类型
#include "Widgets/Input/SVirtualJoystick.h"

AFirstPersonPlayerController::AFirstPersonPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AFirstPersonCameraManager::StaticClass();
	if (IsLocalController())
	{
		if (TeamScoreHUDClass && !TeamScoreHUDInstance)
		{
			TeamScoreHUDInstance = CreateWidget<UUserWidget>(this, TeamScoreHUDClass);
			if (TeamScoreHUDInstance)
			{
				TeamScoreHUDInstance->AddToViewport(); // 关键！添加到屏幕
			}

		}

	}
	// 初始化UI指针
}

void AFirstPersonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		}
		else {

			UE_LOG(LogFirstPerson, Error, TEXT("Could not spawn mobile controls widget."));

		}
	}

	if (IsLocalPlayerController())
	{
		CreateAndShowHUD();
		if (TeamScoreHUDClass && !TeamScoreHUDInstance)

		{

			TeamScoreHUDInstance = CreateWidget<UUserWidget>(this, TeamScoreHUDClass);

			if (TeamScoreHUDInstance)

			{

				TeamScoreHUDInstance->AddToViewport();

				UE_LOG(LogFirstPerson, Log, TEXT("TeamScore HUD created!"));

			}
		}
		if (TimeHUDClass && !TimeHUDInstance)
		{
			TimeHUDInstance = CreateWidget<UUserWidget>(this, TimeHUDClass);
			if (TimeHUDInstance)
			{
				TimeHUDInstance->AddToViewport(); // 关键！添加到屏幕
			}

		}

	}
	
	
}

void AFirstPersonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}
void AFirstPersonPlayerController::CreateAndShowHUD()
{
	// 安全检查
	if (!HUDWidgetClass)
	{
		UE_LOG(LogFirstPerson, Warning, TEXT("HUDWidgetClass is not set in FirstPersonPlayerController!"));
		return;
	}

	// 如果HUD已存在，先移除
	if (CurrentHUDWidget)
	{
		CurrentHUDWidget->RemoveFromParent();
		CurrentHUDWidget = nullptr;
	}

	// 创建新的HUD控件
	CurrentHUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (CurrentHUDWidget)
	{
		CurrentHUDWidget->AddToViewport();
		UE_LOG(LogFirstPerson, Log, TEXT("HUD created and displayed successfully."));


	}
	else
	{
		UE_LOG(LogFirstPerson, Error, TEXT("Failed to create HUD widget!"));
	}

}

void AFirstPersonPlayerController::UpdateHealthOnHUD(float CurrentHealth, float MaxHealth)
{
	OnHealthChanged(CurrentHealth, MaxHealth);
}

int32 AFirstPersonPlayerController::GetTeamScore(int32 TeamIndex) const

{
	if (const ATeamGameState* GS = GetWorld()->GetGameState<ATeamGameState>())
	{
		// 2. 检查 TeamIndex 是否有效
		if (TeamIndex >= 0 && TeamIndex < GS->TeamScores.Num())
		{
			return GS->TeamScores[TeamIndex];
		}
	}
	return 0;

}

// FirstPersonPlayerController.cpp

float AFirstPersonPlayerController::GetRemainingTime() const

{

	if (const ATeamGameState* GS = GetWorld()->GetGameState<ATeamGameState>())

	{

		return GS->RemainingTime;

	}

	return 0.0f;

}


bool AFirstPersonPlayerController::IsGameEnded() const

{

	if (const ATeamGameState* GS = GetWorld()->GetGameState<ATeamGameState>())

	{

		return GS->bGameEnded;

	}

	return true;

}