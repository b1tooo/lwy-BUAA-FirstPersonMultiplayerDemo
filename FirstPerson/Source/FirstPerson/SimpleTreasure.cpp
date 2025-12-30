#include "SimpleTreasure.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FirstPersonCharacter.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TeamGameState.h"
#include "GameFramework/PlayerState.h"  // 添加这行
ASimpleTreasure::ASimpleTreasure()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    // 启用位置复制
    SetReplicateMovement(true);

    // 创建碰撞组件
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(60.0f);
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionSphere->SetGenerateOverlapEvents(true);
    RootComponent = CollisionSphere;

    // 创建视觉网格
    TreasureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TreasureMesh"));
    TreasureMesh->SetupAttachment(RootComponent);
    TreasureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TreasureMesh->SetRelativeScale3D(FVector(0.5f)); // 适当缩放

    // 所有机器都绑定重叠事件
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASimpleTreasure::OnOverlapBegin);
}

void ASimpleTreasure::BeginPlay()
{
    Super::BeginPlay();

    // 保存原始位置
    OriginalLocation = GetActorLocation();

    // 计算边界盒
    float HalfRadius = TeleportRadius * 0.5f;
    BoundaryBox = FBox(
        OriginalLocation - FVector(HalfRadius*1.6, HalfRadius, 0),
        OriginalLocation + FVector(HalfRadius*0.4, HalfRadius, TeleportRadius* 0.5)
    );
    VisBox = FBox(
        OriginalLocation - FVector(HalfRadius * 1.6 + 50, HalfRadius + 50, 0),
        OriginalLocation + FVector(HalfRadius * 0.4 + 50, HalfRadius + 50, TeleportRadius * 0.5)
    );

    // 调试显示边界（仅在开发模式下）
#if !UE_BUILD_SHIPPING
    if (GetWorld() && GetWorld()->WorldType == EWorldType::PIE)
    {
        DrawDebugBox(GetWorld(), VisBox.GetCenter(), VisBox.GetExtent(),
            FColor::Green, true, -1.0f, 0, 5.0f);
    }
#endif
}

void ASimpleTreasure::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 处理冷却时间
    if (bIsOnCooldown)
    {
        CooldownTimer -= DeltaTime;
        if (CooldownTimer <= 0.0f)
        {
            bIsOnCooldown = false;
        }
    }

    // 旋转效果
    if (bEnableRotation && TreasureMesh)
    {
        FRotator CurrentRotation = TreasureMesh->GetRelativeRotation();
        CurrentRotation.Yaw += RotationSpeed * DeltaTime;
        TreasureMesh->SetRelativeRotation(CurrentRotation);
    }
}

void ASimpleTreasure::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 安全检查
    if (bIsOnCooldown) return;

    // 只在服务器上执行逻辑
    if (GetLocalRole() != ROLE_Authority) return;

    // 检查是否为玩家角色
    AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(OtherActor);
    if (!PlayerCharacter) return;
    // 检查玩家是否存活
    if (PlayerCharacter->IsKilled()) return;
    APlayerState* PS = PlayerCharacter->GetController()->GetPlayerState<APlayerState>();
    if (!PS) return;
    // 简单分队：用 PlayerId % 2（红=0, 蓝=1）
    int32 TeamIndex = PS->GetPlayerId() % 2;
    ATeamGameState* GS = GetWorld()->GetGameState<ATeamGameState>();
    if (GS)
    {
        GS->AddScore(TeamIndex, ScoreValue);
    }
    // 设置冷却时间
    bIsOnCooldown = true;
    CooldownTimer = PickupCooldown;

    // 显示拾取消息
    if (GEngine)
    {
        FString Message = FString::Printf(TEXT("Treasure collected! +%d points"), ScoreValue);

        // 根据玩家控制显示不同颜色
        if (!TeamIndex)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Message);
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, Message);
        }
    }


    // 显示拾取特效（在拾取点）
    if (bPlayPickupEffect)
    {
        ShowPickupEffect(GetActorLocation());
    }
    // 生成新位置
    FVector NewLocation = GenerateRandomLocation();
    // 设置新位置（自动复制到所有客户端）
    SetActorLocation(NewLocation);

    // 服务器日志
    //UE_LOG(LogTemp, Log, TEXT("SimpleTreasure moved to new location: %s"), *NewLocation.ToString());
}

FVector ASimpleTreasure::GenerateRandomLocation() const
{
    // 方法1：球形随机
    FVector RandomDirection = FMath::VRand();
    RandomDirection.Z = FMath::Abs(RandomDirection.Z); // 确保向上

    float RandomDistance = FMath::RandRange(TeleportRadius * 0.3f, TeleportRadius);
    FVector NewLocation = OriginalLocation + (RandomDirection * RandomDistance);

    // 方法2：平面随机（可选，更稳定）
    // FVector NewLocation = OriginalLocation;
    // NewLocation.X += FMath::RandRange(-TeleportRadius, TeleportRadius);
    // NewLocation.Y += FMath::RandRange(-TeleportRadius, TeleportRadius);
     NewLocation.Z = OriginalLocation.Z; // 保持高度

    // 确保在边界内
    NewLocation = BoundaryBox.GetClosestPointTo(NewLocation);

    // 确保最小高度
    //NewLocation.Z = FMath::Max(NewLocation.Z, OriginalLocation.Z + MinHeight);

    return NewLocation;
}

void ASimpleTreasure::ShowPickupEffect(const FVector& Location) const
{
    DrawDebugSphere(GetWorld(), Location, 100.0f, 12, FColor::Yellow,
        false, 1.0f, 0, 2.0f);
}