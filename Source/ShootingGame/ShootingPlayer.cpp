// Fill out your copyright notice in the Description page of Project Settings.


#include "ShootingPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

//Apply Damage
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

#include "CharacterAnimInstance.h"

// Sets default values
AShootingPlayer::AShootingPlayer()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	static ConstructorHelpers::FObjectFinder<UParticleSystem> PS(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonMurdock/FX/Particles/Abilities/Primary/FX/P_PlasmaShot_Hit_World.P_PlasmaShot_Hit_World'"));

	if (PS.Succeeded())
	{
		HitParticle = PS.Object;
	}
}

// Called when the game starts or when spawned
void AShootingPlayer::BeginPlay()
{
	Super::BeginPlay();
	AnimInstance = Cast<UCharacterAnimInstance>(GetMesh()->GetAnimInstance());
	AnimInstance->OnMontageEnded.AddDynamic(this, &AShootingPlayer::OnAttackMontageEnded);
}

// Called every frame
void AShootingPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AShootingPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Jumping
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShootingPlayer::Fire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShootingPlayer::StopFire);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShootingPlayer::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShootingPlayer::Look);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed."), *GetNameSafe(this));
	}

}

void AShootingPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AShootingPlayer::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AShootingPlayer::Fire()
{
	if (IsValid(AnimInstance))
	{
		AnimInstance->PlayMontage();
	}
}

void AShootingPlayer::StopFire()
{
	UE_LOG(LogTemp, Log, TEXT("StopFire"));
}

void AShootingPlayer::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	float AttackRange = 10000.f;

	FHitResult HitResult;


	FVector Center = FollowCamera->GetComponentLocation();
	FVector Forwad = Center + FollowCamera->GetForwardVector() * AttackRange;
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	bool Result = GetWorld()->LineTraceSingleByChannel
	(
		OUT HitResult,
		Center,
		Forwad,
		ECollisionChannel::ECC_GameTraceChannel1,
		params
	);

	if (Result)
	{
		UE_LOG(LogTemp, Log, TEXT("Hit"));
		

		if (HitResult.GetActor())
		{
			DrawDebugLine(GetWorld(), Center, HitResult.ImpactPoint, FColor::Green, true);
			DrawDebugSolidBox(GetWorld(), HitResult.ImpactPoint, FVector(10.f, 10.f, 10.f), FColor::Blue, true);

			AActor* HitActor = HitResult.GetActor();
			UGameplayStatics::ApplyDamage(HitActor, 10.f, GetController(), HitActor, NULL);
		}

		if (HitParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticle, HitResult.ImpactPoint);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Not Hit"));
		DrawDebugLine(GetWorld(), Center, HitResult.Location, FColor::Red);
	}


		
}

