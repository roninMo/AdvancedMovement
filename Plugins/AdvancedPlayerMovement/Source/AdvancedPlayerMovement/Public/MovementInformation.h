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
	MOVE_Custom_LedgeClimbing		UMETA(DisplayName = "Ledge Climbing"),
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


/** The type of ledge climb the player is performing */
UENUM(BlueprintType)
enum class EClimbType : uint8
{
	Normal							UMETA(DisplayName = "Normal"),
	Fast							UMETA(DisplayName = "Fast"),
	Slow							UMETA(DisplayName = "Slow"),
	None							UMETA(DisplayName = "None")
};


/** The information to smoothly perform a ledge climb. You'll probably want to refactor this to adjust things for how you handle different logic */
USTRUCT(BlueprintType)
struct FLedgeClimbInformation
{
	GENERATED_USTRUCT_BODY()
		FLedgeClimbInformation(
			const EClimbType LedgeClimbType = EClimbType::None,
			UCurveFloat* SpeedAdjustments = nullptr,
			const float InterpSpeed = 0
		) :

		SpeedAdjustments(SpeedAdjustments),
		InterpSpeed(InterpSpeed)
	{}

public:
	/** The ledge climb variation that this information stores */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EClimbType LedgeClimbType;
	
	/** The ledge climb curve for handling interping smoothly while handling any montage without lots of complications (this is kind of hacky, but it's better than the majority of other fixes for this) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UCurveFloat* SpeedAdjustments;

	/** The interp speed is used as a multiplier after everything else is set, and it's used to prioritize the overall speed of the ledge climb */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float InterpSpeed;
};
