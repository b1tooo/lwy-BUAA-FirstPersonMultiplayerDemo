// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FirstPersonPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(Blueprintable)
class FIRSTPERSON_API AFirstPersonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	AFirstPersonPlayerController();
	/** 更新血条显示 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateHealthOnHUD(float CurrentHealth, float MaxHealth);

protected:
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;
	TObjectPtr<UUserWidget> MobileControlsWidget;


	/** 主HUD控件类 - 包含血条和准星 ???????? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	/** 当前显示的HUD控件实例 */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	UUserWidget* CurrentHUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UUserWidget> TeamScoreHUDClass;
	UPROPERTY()
	UUserWidget* TeamScoreHUDInstance = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UUserWidget> TimeHUDClass;
	UPROPERTY()
	UUserWidget* TimeHUDInstance = nullptr;


	/** 创建并显示HUD */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void CreateAndShowHUD();
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetTeamScore(int32 TeamIndex) const;


	UFUNCTION(BlueprintPure, Category = "Timer")
	float GetRemainingTime() const;
	UFUNCTION(BlueprintPure, Category = "Timer")
	bool IsGameEnded() const;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** 蓝图可实现的健康更新事件 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHealthChanged(float CurrentHealth, float MaxHealth);

};
