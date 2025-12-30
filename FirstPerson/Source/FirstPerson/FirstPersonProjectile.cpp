// Fill out your copyright notice in the Description page of Project Settings.


#include "FirstPersonProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
// Sets default values
AFirstPersonProjectile::AFirstPersonProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	//定义将作为投射物及其碰撞的根组件的SphereComponent。
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	SphereComponent->InitSphereRadius(1.0f);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = SphereComponent;

	//在击中事件上注册此投射物撞击函数。
	if (GetLocalRole() == ROLE_Authority)
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &AFirstPersonProjectile::OnProjectileImpact);
	}


	//定义将作为视觉呈现的网格体。
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMesh->SetupAttachment(RootComponent);

	// 修复1: 使用引擎内置的基本形状
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (DefaultMesh.Succeeded())
	{
		StaticMesh->SetStaticMesh(DefaultMesh.Object);
		StaticMesh->SetRelativeScale3D(FVector(0.3f)); // 缩小到合适大小

		// 修复2: 给网格一个明显的颜色以便看到
		static ConstructorHelpers::FObjectFinder<UMaterial> FLASHMaterial(TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Projectiles/Materials/M_Explosion.M_Explosion"));
		StaticMesh->SetMaterial(0, FLASHMaterial.Object);
	}
	else
	{
		// 如果引擎内置的也找不到，至少确保组件存在
		StaticMesh->SetRelativeScale3D(FVector(0.2f));
	}

	// 修复3: 尝试不同的爆炸效果路径
	//static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	//// 如果上面的路径不对，用这个备选：
	//// static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Game/StarterContent/Particles/P_Explosion"));

	//if (DefaultExplosionEffect.Succeeded())
	//{
	//	ExplosionEffect = DefaultExplosionEffect.Object;
	//}

	//定义投射物移动组件。
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->SetUpdatedComponent(SphereComponent);
	ProjectileMovementComponent->InitialSpeed = 3000.0f; //加快速度以便看到
	ProjectileMovementComponent->MaxSpeed = 3000.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	DamageType = UDamageType::StaticClass();
	Damage = 50.0f;
}

// Called when the game starts or when spawned
void AFirstPersonProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (SphereComponent && GetInstigator())
	{
		SphereComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}
}
void AFirstPersonProjectile::Destroyed()
{
	FVector spawnLocation = GetActorLocation();
	UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, spawnLocation, FRotator::ZeroRotator, true, EPSCPoolMethod::AutoRelease);
}
void AFirstPersonProjectile::OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor)
	{
		// 不伤害发射者
		if (OtherActor == GetOwner() || OtherActor == GetInstigator())
		{
			// 如果击中的是自己，直接返回不销毁子弹
			return;
		}

		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, NormalImpulse, Hit,
			GetInstigator() ? GetInstigator()->Controller : nullptr, this, DamageType);
	}
	Destroy();
}
// Called every frame
void AFirstPersonProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

