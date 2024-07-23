// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BhopCharacter.h"

#include "AdvancedMovementComponent.h"

ABhopCharacter::ABhopCharacter(const FObjectInitializer& ObjectInitializer) : Super( // The super initializer is how you subclass components
	ObjectInitializer.SetDefaultSubobjectClass<UAdvancedMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	
}


void ABhopCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

UAdvancedMovementComponent* ABhopCharacter::GetAdvancedCharacterMovementComponent() const
{
	return GetMovementComp<UAdvancedMovementComponent>();
}
