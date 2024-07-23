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
	
	
};
