#include "FirstPersonGameMode.h"
#include "TeamGameState.h"
// 添加构造函数，启用 Tick
AFirstPersonGameMode::AFirstPersonGameMode()
{
    // 启用每帧 Tick
    PrimaryActorTick.bCanEverTick = true;
    // 可选：指定 GameState 类（推荐）
    GameStateClass = ATeamGameState::StaticClass();
}
void AFirstPersonGameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("GameMode initialized!"));
    //if (ATeamGameState* MyGameState = Cast<ATeamGameState>(GameState))
    //{
    //    MyGameState->RemainingTime = GameDuration;
    //    MyGameState->bGameEnded = false;
    //}
}
void AFirstPersonGameMode::PostLogin(APlayerController* NewPlayer)

{

    Super::PostLogin(NewPlayer);


    // 只有服务器能控制游戏开始

    if (!HasAuthority()) return;


    // 获取当前玩家数量

    int32 PlayerCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)

    {

        if (APlayerController* PC = It->Get())

        {

            PlayerCount++;

        }

    }


    UE_LOG(LogTemp, Log, TEXT("Current player count: %d"), PlayerCount);


    // 如果已经有2人且游戏还没开始，则启动计时

    if (PlayerCount >= 2 && !bGameStarted)

    {

        if (ATeamGameState* MyGameState = Cast<ATeamGameState>(GameState))

        {

            MyGameState->RemainingTime = GameDuration;

            MyGameState->bGameEnded = false;

            bGameStarted = true;


            UE_LOG(LogTemp, Warning, TEXT("Game started with %d players! Timer: %.1fs"), PlayerCount, GameDuration);

        }

    }

}

void AFirstPersonGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority())
    {
        if (ATeamGameState* MyGameState = Cast<ATeamGameState>(GameState))
        {
            if (!MyGameState->bGameEnded && MyGameState->RemainingTime > 0.0f && bGameStarted)
            {
                MyGameState->RemainingTime -= DeltaSeconds;
                UE_LOG(LogTemp, Log, TEXT("RemainingTime: %.2f"), MyGameState->RemainingTime); // 调试用
                if (MyGameState->RemainingTime <= 0.0f)
                {
                    MyGameState->RemainingTime = 0.0f;
                    MyGameState->bGameEnded = true;
                    UE_LOG(LogTemp, Warning, TEXT("Game time is up!")); // 现在会输出了！
                }
            }
        }
    }
}