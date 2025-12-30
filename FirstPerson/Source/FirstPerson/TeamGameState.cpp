#include "TeamGameState.h"
#include <Net/UnrealNetwork.h>

ATeamGameState::ATeamGameState()
{
    TeamScores.Init(0, 2); // 初始化两支队伍分数为 0
}

void ATeamGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATeamGameState, TeamScores);
    DOREPLIFETIME(ATeamGameState, RemainingTime); // 新增
    DOREPLIFETIME(ATeamGameState, bGameEnded);    // 新增
}

void ATeamGameState::OnRep_TeamScores()
{
}

void ATeamGameState::AddScore(int32 TeamIndex, int32 Score)
{
    if (GetLocalRole() == ROLE_Authority && TeamIndex >= 0 && TeamIndex < TeamScores.Num())
    {
        TeamScores[TeamIndex] += Score;
    }
}
