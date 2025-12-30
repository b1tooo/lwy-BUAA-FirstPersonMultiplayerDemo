// Copyright Epic Games, Inc.All Rights Reserved.


#pragma once


#include "CoreMinimal.h"

#include "GameFramework/GameModeBase.h"

#include "FirstPersonGameMode.generated.h"


UCLASS()

class AFirstPersonGameMode : public AGameModeBase

{

	GENERATED_BODY()


public:

	// 游戏总时长（可在编辑器中调整）

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Rules", meta = (ClampMin = "10.0"))

	float GameDuration = 300.0f; // 默认 5 分钟


protected:

	AFirstPersonGameMode();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	// 新增：是否已开始游戏计时

	bool bGameStarted = false;


	// 新增：重写 PostLogin，监听玩家加入

	virtual void PostLogin(APlayerController* NewPlayer) override;
};