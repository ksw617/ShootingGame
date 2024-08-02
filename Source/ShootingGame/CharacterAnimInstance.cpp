// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAnimInstance.h"


UCharacterAnimInstance::UCharacterAnimInstance()
{
	static ConstructorHelpers::FObjectFinder<UAnimMontage> AM(TEXT("/Script/Engine.AnimMontage'/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Fire_Fast_Montage.Fire_Fast_Montage'"));
	if (AM.Succeeded())
	{
		FireMontage = AM.Object;
	

	}
}



void UCharacterAnimInstance::PlayMontage()
{
	if (IsValid(FireMontage))
	{
		if (!Montage_IsPlaying(FireMontage))
		{
			Montage_Play(FireMontage);
		}
	}
}
