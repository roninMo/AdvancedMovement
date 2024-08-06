// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BhopCharacter.generated.h"


class UAdvancedMovementComponent;

UCLASS(Blueprintable, Category=Character)
class ADVANCEDPLAYERMOVEMENT_API ABhopCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	ABhopCharacter(const FObjectInitializer& ObjectInitializer);
	

protected:
	virtual void BeginPlay() override;
	
	/** Templated convenience version for retrieving the movement component. */
	template<class T> T* GetMovementComp(void) const { return Cast<T>(GetMovementComponent()); }

	/** Retrieves the advanced movement component */
	UFUNCTION(BlueprintCallable, Category="Movement", DisplayName="Get Character Movement Component")
	virtual UAdvancedMovementComponent* GetAdvancedCharacterMovementComponent() const;

	/**
	 * Customizable event to check if the character can jump in the current state.
	 * Default implementation returns true if the character is on the ground and not crouching,
	 * has a valid CharacterMovementComponent and CanEverJump() returns true.
	 * Default implementation also allows for 'hold to jump higher' functionality: 
	 * As well as returning true when on the ground, it also returns true when GetMaxJumpTime is more
	 * than zero and IsJumping returns true.
	 * 
	 *
	 * @Return Whether the character can jump in the current state. 
	 */
	virtual bool CanJumpInternal_Implementation() const override;

};
