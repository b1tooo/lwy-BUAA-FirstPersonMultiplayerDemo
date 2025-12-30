// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FirstPerson.h"
#include "GameFramework/GameModeBase.h"    // 也需要这个
#include <Net/UnrealNetwork.h>
#include <FirstPersonPlayerController.h>
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include <Kismet/GameplayStatics.h>
AFirstPersonCharacter::AFirstPersonCharacter()
{
	//初始化玩家生命值
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	bIsKilled = false;
	livetime = 3;
	Tags.Add(FName("Player"));
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	ProjectileClass = AFirstPersonProjectile::StaticClass();
	//初始化射速
	FireRate = 0.25f;
	bIsFiringWeapon = false;
}
void AFirstPersonCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 当角色被控制器控制时（游戏开始时）
	if (GetLocalRole() == ROLE_Authority)
	{
		InitializePlayer();
	}
}
void AFirstPersonCharacter::InitializePlayer()
{
	// 仅服务器执行
	if (GetLocalRole() != ROLE_Authority) return;
	// 设置初始位置
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	if (PlayerStarts.Num() > 0)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// 获取玩家ID
			int32 PlayerID = 0;
			if (PC->PlayerState)
			{
				PlayerID = PC->PlayerState->GetPlayerId();
			}

			// 使用玩家ID选择出生点
			int32 StartIndex = FMath::Abs(PlayerID) % PlayerStarts.Num();
			AActor* SelectedStart = PlayerStarts[StartIndex];

			SetActorLocation(SelectedStart->GetActorLocation());
			SetActorRotation(SelectedStart->GetActorRotation());

			UE_LOG(LogFirstPerson, Log, TEXT("Player %d spawned at start point %d"), PlayerID, StartIndex);
		}
	}
}

void AFirstPersonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//复制当前生命值。
	DOREPLIFETIME(AFirstPersonCharacter, CurrentHealth);
	DOREPLIFETIME(AFirstPersonCharacter, bIsKilled);
}

void AFirstPersonCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清理定时器
	GetWorld()->GetTimerManager().ClearTimer(livetimer);
	GetWorld()->GetTimerManager().ClearTimer(FiringTimer);
}

void AFirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFirstPersonCharacter::DoJumpEnd);

		// Firing
		EnhancedInputComponent->BindAction(Fire1Action, ETriggerEvent::Started, this, &AFirstPersonCharacter::DoFireStart);
		EnhancedInputComponent->BindAction(Fire1Action, ETriggerEvent::Completed, this, &AFirstPersonCharacter::DoFireEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogFirstPerson, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AFirstPersonCharacter::Healthlog()
{
	//客户端特定的功能
	if (IsLocallyControlled())
	{
		FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHealth <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
		}
	}

	//服务器特定的功能
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}

	//在所有机器上都执行的函数。
	/*
		因任何因伤害或死亡而产生的特殊功能都应放在这里。
	*/
}

void AFirstPersonCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AFirstPersonCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AFirstPersonCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AFirstPersonCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AFirstPersonCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AFirstPersonCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
void AFirstPersonCharacter::DoFireStart()
{
	//FString healthMessage = FString::Printf(TEXT("Fire!"));
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

	if (!bIsFiringWeapon)
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &AFirstPersonCharacter::DoFireEnd, FireRate, false);

		// 获取本地摄像机的旋转并传递给服务器
		FRotator FireRotation;
		if (FirstPersonCameraComponent)
		{
			FireRotation = FirstPersonCameraComponent->GetComponentRotation();
		}
		else if (GetController())
		{
			FireRotation = GetController()->GetControlRotation();
		}
		else
		{
			FireRotation = GetActorRotation();
		}

		HandleFire(FireRotation);
	}
}


void AFirstPersonCharacter::DoFireEnd()
{
	bIsFiringWeapon = false;
}

void AFirstPersonCharacter::HandleFire_Implementation(const FRotator& FireRotation)
{
	// 使用从客户端传来的旋转
	FVector spawnLocation;
	if (FirstPersonCameraComponent)
	{
		spawnLocation = FirstPersonCameraComponent->GetComponentLocation();
	}
	else
	{
		spawnLocation = GetActorLocation() + FVector(0, 0, 70.0f);
	}

	// 向前偏移
	spawnLocation += FireRotation.Vector() * 150.0f;

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = this;
	spawnParameters.Owner = this;

	GetWorld()->SpawnActor<AFirstPersonProjectile>(ProjectileClass, spawnLocation, FireRotation, spawnParameters);
}

void AFirstPersonCharacter::DisablePlayerInput()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AFirstPersonCharacter::EnablePlayerInput()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}
}
// 6. 添加 Die 函数（在 EnablePlayerInput 函数下面）：
void AFirstPersonCharacter::Die()
{
	// 仅服务器执行
	if (GetLocalRole() != ROLE_Authority || bIsKilled) return;

	bIsKilled = true;

	GetCharacterMovement()->StopMovementImmediately();
	DisablePlayerInput();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (FirstPersonMesh)
		FirstPersonMesh->SetVisibility(false, true);
	// 隐藏第一人称手臂（自己看不到）
	if (FirstPersonMesh)
		FirstPersonMesh->SetVisibility(false, true);
	// 重要：也隐藏第三人称身体网格（别人看不到）
	GetMesh()->SetVisibility(false, true);



	//// 设置摄像机看向地面
	//if (FirstPersonCameraComponent)
	//{
	//	// 设置摄像机看向地面（向下旋转90度）
	//	FRotator CurrentRotation = FirstPersonCameraComponent->GetRelativeRotation();
	//	CurrentRotation.Pitch = -90.0f; // 向下看90度
	//	CurrentRotation.Roll = 0.0f;    // 保持水平

	//	FirstPersonCameraComponent->SetRelativeRotation(CurrentRotation);
	//}

	// 方案1：直接设置摄像机世界旋转
	if (FirstPersonCameraComponent)
	{
		FirstPersonCameraComponent->bUsePawnControlRotation = false;

		// 计算看向地面的旋转
		FRotator GroundRotation = GetActorRotation();
		GroundRotation.Pitch = -90.0f;  // 看向地面
		GroundRotation.Roll = 0.0f;

		FirstPersonCameraComponent->SetWorldRotation(GroundRotation);
	}



	GetWorld()->GetTimerManager().SetTimer(livetimer, this, &AFirstPersonCharacter::Respawn, livetime, false);
}
// 7. 添加 Respawn 函数（在 Die 函数下面）：
void AFirstPersonCharacter::Respawn()
{
	// 仅服务器执行
	if (GetLocalRole() != ROLE_Authority) return;

	CurrentHealth = MaxHealth;
	bIsKilled = false;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (FirstPersonMesh)
		FirstPersonMesh->SetVisibility(true, true);

	// 重要：也显示第三人称身体网格（别人看到）
	GetMesh()->SetVisibility(true, true);

	//// 恢复摄像机视角
	//if (FirstPersonCameraComponent)
	//{
	//	// 恢复水平视角
	//	FRotator CurrentRotation = FirstPersonCameraComponent->GetRelativeRotation();
	//	CurrentRotation.Pitch = 0.0f;  // 恢复水平视角
	//	CurrentRotation.Roll = 0.0f;

	//	FirstPersonCameraComponent->SetRelativeRotation(CurrentRotation);
	//}
	if (FirstPersonCameraComponent)
	{
		// 1. 重新启用控制器控制
		FirstPersonCameraComponent->bUsePawnControlRotation = true;

		// 2. 恢复默认的相对旋转（构造函数中的设置）
		// 默认旋转是 (0.0f, 90.0f, -90.0f)
		FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	EnablePlayerInput();
	// 重置位置到出生点
	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AActor* PlayerStart = GameMode->FindPlayerStart(PC))
			{
				SetActorLocation(PlayerStart->GetActorLocation());
				SetActorRotation(PlayerStart->GetActorRotation());
			}
		}
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// 调用Controller更新健康值UI
		if (AFirstPersonPlayerController* FPPC = Cast<AFirstPersonPlayerController>(PC))
		{
			// 更新血条
			FPPC->UpdateHealthOnHUD(CurrentHealth, MaxHealth);
		}
	}
	//Healthlog();
}

// 8. 添加 OnRep_IsKilled 函数（在 OnRep_CurrentHealth 函数下面）：
void AFirstPersonCharacter::OnRep_IsKilled()
{
	if (bIsKilled)
	{
		// 客户端死亡处理
		GetCharacterMovement()->StopMovementImmediately();
		DisablePlayerInput();

		GetMesh()->SetVisibility(false, true);
		if (FirstPersonMesh)
			FirstPersonMesh->SetVisibility(false, true);

		// 客户端设置死亡视角
		if (FirstPersonCameraComponent)
		{
			FirstPersonCameraComponent->bUsePawnControlRotation = false;

			// 计算看向地面的旋转
			FRotator GroundRotation = GetActorRotation();
			GroundRotation.Pitch = -90.0f;  // 看向地面
			GroundRotation.Roll = 0.0f;

			FirstPersonCameraComponent->SetWorldRotation(GroundRotation);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true, true);
		if (FirstPersonMesh)
			FirstPersonMesh->SetVisibility(true, true);

		// 客户端恢复摄像机控制
		if (FirstPersonCameraComponent)
		{
			// 重新启用控制器控制
			FirstPersonCameraComponent->bUsePawnControlRotation = true;

			// 恢复默认的相对旋转
			FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
		}

		EnablePlayerInput();
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AFirstPersonCharacter::OnRep_CurrentHealth()
{
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = GetController<APlayerController>())
		{
			if (AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC))
			{
				FirstPersonPC->UpdateHealthOnHUD(CurrentHealth, MaxHealth);
			}
		}
	}
	//Healthlog();
}

void AFirstPersonCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		if (IsLocallyControlled())
		{
			if (APlayerController* PC = GetController<APlayerController>())
			{
				if (AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC))
				{
					FirstPersonPC->UpdateHealthOnHUD(CurrentHealth, MaxHealth);
				}
			}
		}
		//Healthlog();
		if (CurrentHealth <= 0.0f && !bIsKilled)
		{
			Die();
		}
	}
}
float AFirstPersonCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsKilled) return 0.0f;
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}
