#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimpleTreasure.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class FIRSTPERSON_API ASimpleTreasure : public AActor
{
    GENERATED_BODY()

public:
    ASimpleTreasure();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    /** 重叠事件处理 - 只在服务器执行 */
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    /** 生成随机位置 */
    FVector GenerateRandomLocation() const;

    /** 在拾取点显示特效（可选） */
    void ShowPickupEffect(const FVector& Location) const;

public:
    /** 碰撞组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionSphere;

    /** 视觉网格 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TreasureMesh;

    /** 宝藏价值（分数） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
    int32 ScoreValue = 1;

    /** 移动范围（半径） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
    float TeleportRadius = 2000.0f;

    /** 最小高度（避免掉到地下） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
    float MinHeight = 50.0f;

    /** 是否启用旋转效果 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
    bool bEnableRotation = true;

    /** 旋转速度（度/秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure", meta = (EditCondition = "bEnableRotation"))
    float RotationSpeed = 45.0f;

    /** 拾取冷却时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
    float PickupCooldown = 0.5f;

    /** 是否在拾取时播放特效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    bool bPlayPickupEffect = true;

private:
    /** 是否处于冷却状态 */
    bool bIsOnCooldown = false;

    /** 冷却计时器 */
    float CooldownTimer = 0.0f;

    /** 原始位置（用于边界检查） */
    FVector OriginalLocation;

    /** 边界盒（限制移动范围） */
    FBox BoundaryBox;
    FBox VisBox;
};