#pragma once


#include "CoreMinimal.h"
#include "MovementInformation.generated.h"


/** The custom movement modes */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	MOVE_Custom_None				UMETA(DisplayName = "None"),
	MOVE_Custom_Slide				UMETA(DisplayName = "Slide"),
	MOVE_Custom_WallClimbing		UMETA(DisplayName = "Wall Climbing"),
	MOVE_Custom_Mantling			UMETA(DisplayName = "Mantling"),
	MOVE_Custom_WallRunning			UMETA(DisplayName = "Wall Running"),
	MOVE_Custom_MAX					UMETA(Hidden)
};


/** Whether the character (used for ai) is walking, jogging, or sprinting, etc. */
UENUM(BlueprintType)
enum class EMovementGait : uint8
{
	Gait_Walking						UMETA(DisplayName = "Walking"),
	Gait_Jogging						UMETA(DisplayName = "Jogging"),
	Gait_Sprinting						UMETA(DisplayName = "Sprinting"),
	Gait_None                           UMETA(DisplayName = "None")
};


/** The specific movement direction */
UENUM(BlueprintType)
enum class EMovementDirection : uint8
{
	MD_Forward		                    UMETA(DisplayName = "Forward"),
	MD_Left								UMETA(DisplayName = "Left"),
	MD_Right		                    UMETA(DisplayName = "Right"),
	MD_Backward		                    UMETA(DisplayName = "Backward"),
	MD_None                             UMETA(DisplayName = "None")
};


/** If the character is turning while not moving */
UENUM(BlueprintType)
enum class EVaultType : uint8
{
	Vault_High							UMETA(DisplayName = "High"),
	Vault_Low							UMETA(DisplayName = "Low"),
	Vault_VaultOver						UMETA(DisplayName = "Vault Over"),
	Vault_FallingCatch					UMETA(DisplayName = "Falling Catch"),
	Vault_None                          UMETA(DisplayName = "None")
};


/** The information to smoothly perform a vault -  TODO: Refactor this */
USTRUCT(BlueprintType)
struct F_VaultInformation
{
	GENERATED_USTRUCT_BODY()
		F_VaultInformation(
			UAnimMontage* Montage = nullptr,
			const float Duration = 0,
			UCurveFloat* Curve = nullptr,
			const float InterpSpeed = 0

			/*
				const float LowHeight = 0.0f,
				const float LowPlayRate = 0.0f,
				const float LowStartPosition = 0.0f,
				
				const float HighHeight = 0.0f,
				const float HighPlayRate = 0.0f,
				const float HighStartPosition = 0.0f
			*/
		) :

		Montage(Montage),
		Duration(Duration),
		Curve(Curve),
		InterpSpeed(InterpSpeed)

	{}

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UAnimMontage* Montage;

	/** The duration of the montage section (including accounting for montage blending) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration;

	/** The mantle curve for handling interping smoothly while handling any montage without lots of complications (this is kind of hacky, but it's better than the majority of other fixes for this) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UCurveFloat* Curve;

	/** The interp speed is used as a multiplier after everything else is set, and it's used to prioritize the overall speed of the mantle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float InterpSpeed;
};
