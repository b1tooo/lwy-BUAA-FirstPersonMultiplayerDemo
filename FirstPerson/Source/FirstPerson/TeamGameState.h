// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTPERSON_API ATeamGameState : public AGameStateBase
{
    GENERATED_BODY()


public:
    ATeamGameState();

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

    // 队伍分数：[0] = 红队, [1] = 蓝队
    // ===== 计分系统（已有）=====
    UPROPERTY(ReplicatedUsing = OnRep_TeamScores)
    TArray<int32> TeamScores;
    UFUNCTION()
    void OnRep_TeamScores();
    void AddScore(int32 TeamIndex, int32 Score);

    // ===== 倒计时（新增）=====

    UPROPERTY(Replicated)  // 只需要 Replicated！
    float RemainingTime = 300.0f;

    UPROPERTY(Replicated)
    bool bGameEnded = false;
};
