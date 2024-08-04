// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvancedMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/StructuredLog.h"


DEFINE_LOG_CATEGORY(Movement);

namespace CharacterMovementConstants // @ref 5.4
{
	// MAGIC NUMBERS
	const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps
	const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.
}



UAdvancedMovementComponent::UAdvancedMovementComponent()
{
	SetNetworkMoveDataContainer(CustomMoveDataContainer);
	
	// Movement
	SprintSpeedMultiplier = 2.0;
	CrouchSprintSpeedMultiplier = 2.0;
	AimSpeedMultiplier = 1.0;
	WalkSpeedMultiplier = 0.74;
	SlideSpeedLimit = 9000.0;

	// Bunny hopping
	bUseBhopping = true;
	StrafingMaxAcceleration = 6400;
	AirStrafeSpeedGainMultiplier = 1.64;
	AirStrafeRotationRate = 3;

	// Strafe Swaying
	StrafeSwayDuration = 0.2;
	StrafeSwaySpeedGainMultiplier = 1;
	StrafeSwayRotationRate = 3;

	// Strafe Lurching
	StrafeLurchDuration = 0.45;
	StrafeLurchFullStrengthDuration = 0.1;
	StrafeLurchStrength = 1;
	StrafeLurchFriction = 2;
	
	// Sliding
	bUseSliding = true;
	SlideEnterThreshold = 600;
	SlideEnterBoost = 450;
	SlidingRotationRate = 1;
	SlidingFriction = 0.64;
	SlideAngleFrictionMultiplier = 3.4;
	SlideBrakingFriction = 2;
	SlideJumpBoost = 100;
	SlideDelay = 1.5;
	WalkingDurationToSlide = 0.2;

	// Wall Jumping
	bUseWallJumping = true;
	WallJumpSpeed = 640;
	WallJumpBoost = FVector2D(100, 640);
	WallJumpBoostDuringWallClimbs = FVector2D(164, 340);
	WallJumpBoostDuringWallRuns = FVector2D(340, 540);
	WallJumpLimit = 2;
	WallJumpValidDistance = 45;
	WallJumpHeightFromGroundThreshold = 64.0;
	WallJumpSpacing = 50;

	// Mantle Jumping
	bUseMantleJumping = true;
	MantleJumpDuration = 0.2;
	MantleJumpBoost = FVector2D(450, 450);
	SuperGlideDuration = 0.05;
	SuperGlideBoost = FVector2D(450, 100);
	
	// Wall Climbing
	bUseWallClimbing = true;
	WallClimbDuration = 1.5;
	WallClimbInterval = 0.74;
	WallClimbSpeed = 400;
	WallClimbAcceleration = 20;
	WallClimbMultiplier = FVector2D(0.64, 1);
	WallClimbAcceptableAngle = 45;
	WallClimbFriction = 2.5;
	WallClimbGravityLimit = -45;
	WallClimbAddSpeedThreshold = -10;

	// Mantling
	bUseMantling = true;
	MantleSpeed = 200.0;
	MantleRotationSpeed = 1;
	MantleLedgeLocationOffset = -55;
	MantleSurfaceTraceFromLedgeOffset = 10;
	MantleTraceDistance = 64;
	MantleSecondTraceDistance = 100;
	MantleTraceHeightOffset = 10;
	ClimbLocationSpaceOffset = 2.0;
	MantleLocationSpaceOffset = 2.0;
	MantleObjects.Add(EObjectTypeQuery::ObjectTypeQuery1);
	MantleObjects.Add(EObjectTypeQuery::ObjectTypeQuery2);
	MantleObjects.Add(EObjectTypeQuery::ObjectTypeQuery5);
	
	// Ledge Climbing
	bUseLedgeClimbing = true;
	LedgeClimbOffset = 3.4;
	CollisionResponsesDuringLedgeClimbing.SetResponse(ECC_Visibility, ECR_Ignore);
	CollisionResponsesDuringLedgeClimbing.SetResponse(ECC_Camera, ECR_Ignore);
	CollisionResponsesDuringLedgeClimbing.SetResponse(ECC_WorldStatic, ECR_Ignore);
	CollisionResponsesDuringLedgeClimbing.SetResponse(ECC_WorldDynamic, ECR_Ignore);
	CollisionResponsesDuringLedgeClimbing.SetResponse(ECC_Vehicle, ECR_Ignore);
	
	// Wall Running
	bUseWallRunning = true;
	WallRunDuration = 2;
	WallRunSpeed = 740;
	WallRunAcceleration = 7.4;
	WallRunMultiplier = FVector2D(1, 0);
	WallRunSpeedThreshold = 600;
	WallRunAcceptableAngleRadius = 30;
	WallRunHeightThreshold = 300;
	
	// // CharacterMovement (General Settings)
	MaxAcceleration = 1200; // Derived from the Acceleration values 
	BrakingFrictionFactor = 2;
	BrakingFriction = 2.5;
	SetCrouchedHalfHeight(64);
	bUseSeparateBrakingFriction = true;
	Mass = 100;
	DefaultLandMovementMode = EMovementMode::MOVE_Walking;
	DefaultWaterMovementMode = EMovementMode::MOVE_Swimming;
	RotationRate = FRotator(0.0, 690.0, 0.0);
	
	// Character Movement: Walking
	MaxStepHeight = 45;
	SetWalkableFloorAngle(46.35);
	SetWalkableFloorZ(0.690251);
	GroundFriction = 11.0;
	MaxWalkSpeed = 340;
	MaxWalkSpeedCrouched = 200;
	MinAnalogWalkSpeed = 0;
	BrakingDecelerationWalking = 200;
	bSweepWhileNavWalking = true;
	bCanWalkOffLedges = true;
	bCanWalkOffLedgesWhenCrouching = true;
	bMaintainHorizontalGroundVelocity = false; 
	bIgnoreBaseRotation = false;
	PerchRadiusThreshold = 0;
	PerchAdditionalHeight = 40;
	LedgeCheckThreshold = 4;
	bAlwaysCheckFloor = false;
	bUseFlatBaseForFloorChecks = false;
	
	// Character Movement: Jumping/Falling
	GravityScale = 2.03;
	BrakingDecelerationFalling = 340;
	AirControl = 1.0; // this eases the slow down but doesn't necessarily prevent it
	AirControlBoostMultiplier = 1;
	AirControlBoostVelocityThreshold = 1;
	FallingLateralFriction = 0;
	JumpZVelocity = 740;
	JumpOffJumpZFactor = 0.5;
	bImpartBaseVelocityX = true;
	bImpartBaseVelocityY = true;
	bImpartBaseVelocityZ = true;
	bImpartBaseAngularVelocity = true;
	bNotifyApex = false;
	bApplyGravityWhileJumping = true;
	
	// Character Movement (Networking)
	NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	bNetworkSkipProxyPredictionOnNetUpdate = false;
	NetworkMaxSmoothUpdateDistance = 256;
	NetworkNoSmoothUpdateDistance = 384;
	NetworkMinTimeBetweenClientAckGoodMoves = 0.1;
	NetworkMinTimeBetweenClientAdjustments = 0.1;
	NetworkMinTimeBetweenClientAdjustmentsLargeCorrection = 0.05;
	NetworkLargeClientCorrectionDistance = 15;
	bNetworkAlwaysReplicateTransformUpdateTimestamp = false;
	NetworkSimulatedSmoothLocationTime = 0.1;
	NetworkSimulatedSmoothRotationTime = 0.05;
	ListenServerNetworkSimulatedSmoothLocationTime = 0.04;
	ListenServerNetworkSimulatedSmoothRotationTime = 0.033;
	NetProxyShrinkRadius = 0.01;
	NetProxyShrinkHalfHeight = 0.01;	
	
	// Movement Capabilities
	NavAgentProps.AgentHeight = 48; 
	NavAgentProps.AgentRadius = 192;
	NavAgentProps.AgentStepHeight = -1;
	NavAgentProps.NavWalkingSearchHeightScale = 0.5;
	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanJump = true;
	NavAgentProps.bCanWalk = true;
	NavAgentProps.bCanSwim = true;
	NavAgentProps.bCanFly = false;
	
	// Movement Component
	bUpdateOnlyIfRendered = false;
	bAutoUpdateTickRegistration = true;
	bTickBeforeOwner = true;
	bAutoRegisterUpdatedComponent = true;
	bAutoRegisterPhysicsVolumeUpdates = true;
	bComponentShouldUpdatePhysicsVolume = true;

	// Root motion
	bAllowPhysicsRotationDuringAnimRootMotion = true;

	// Other
	TraceDuration = 5;
}


void UAdvancedMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}




//------------------------------------------------------------------------------//
// General Movement Logic														//
//------------------------------------------------------------------------------//

#pragma region Conditional Checks
bool UAdvancedMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const { return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode; }
bool UAdvancedMovementComponent::IsMovingOnGround() const { return ((MovementMode == MOVE_Walking) || (MovementMode == MOVE_NavWalking) || IsSliding()) && UpdatedComponent; }
bool UAdvancedMovementComponent::IsSliding() const { return MovementMode == MOVE_Custom && CustomMovementMode == MOVE_Custom_Slide; }
bool UAdvancedMovementComponent::IsRunning() const { return MovementMode == MOVE_Walking && SprintPressed; }
bool UAdvancedMovementComponent::IsWallClimbing() const { return MovementMode == MOVE_Custom && CustomMovementMode == MOVE_Custom_WallClimbing; }
bool UAdvancedMovementComponent::IsWallRunning() const { return MovementMode == MOVE_Custom && CustomMovementMode == MOVE_Custom_WallRunning; }
bool UAdvancedMovementComponent::IsMantling() const { return MovementMode == MOVE_Custom_Mantling; }
bool UAdvancedMovementComponent::IsLedgeClimbing() const { return MovementMode == MOVE_Custom && CustomMovementMode == MOVE_Custom_LedgeClimbing; }
bool UAdvancedMovementComponent::IsAiming() const { return AimPressed; }
bool UAdvancedMovementComponent::IsStrafeSwaying() { return AirStrafeSwayPhysics; }
bool UAdvancedMovementComponent::IsStrafeLurching() { return AirStrafeLurchPhysics; }
#pragma endregion 



//------------------------------------------------------------------------------//
// Update Movement Mode Logic													//
//------------------------------------------------------------------------------//
#pragma region Update Movement Mode
void UAdvancedMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if (bDebugMovementMode)
	{
		UE_LOGFMT(Movement, Warning, "{0}: Movement Mode Updated: {1}, previous movement mode: {2}",
			*GetNameSafe(CharacterOwner),
			*UEnum::GetValueAsString(MovementMode), 
			*UEnum::GetValueAsString(PreviousMovementMode)
		);
	}
	
	// Handle exiting previous modes
	if		(PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Custom_Slide) ExitSlide();
	else if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Custom_WallClimbing) ExitWallClimb();
	else if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Custom_Mantling) ExitMantle();
	else if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Custom_LedgeClimbing) ExitLedgeClimb();
	else if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Custom_WallRunning) ExitWallRun();

	// Handle entering new modes
	if		(IsCustomMovementMode(MOVE_Custom_Slide)) EnterSlide(PreviousMovementMode, static_cast<ECustomMovementMode>(PreviousCustomMode));
	else if (IsCustomMovementMode(MOVE_Custom_WallClimbing)) EnterWallClimb(PreviousMovementMode, static_cast<ECustomMovementMode>(PreviousCustomMode));
	else if (IsCustomMovementMode(MOVE_Custom_Mantling)) EnterMantle(PreviousMovementMode, static_cast<ECustomMovementMode>(PreviousCustomMode));
	else if (IsCustomMovementMode(MOVE_Custom_LedgeClimbing)) EnterLedgeClimb(PreviousMovementMode, static_cast<ECustomMovementMode>(PreviousCustomMode));
	else if (IsCustomMovementMode(MOVE_Custom_WallRunning)) EnterWallRun(PreviousMovementMode, static_cast<ECustomMovementMode>(PreviousCustomMode));

	// State information
	ResetWallJumpInformation(PreviousMovementMode, PreviousCustomMode);
	ResetWallClimbInformation(PreviousMovementMode, PreviousCustomMode);
	ResetWallRunInformation(PreviousMovementMode, PreviousCustomMode);
	ResetFallingStateInformation(PreviousMovementMode, PreviousCustomMode);
	ResetGroundStateInformation(PreviousMovementMode, PreviousCustomMode);

	// Update player state
	// if (CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_Authority)
	// {
	//		UpdateExternalMovementModeInformation(CharacterOwner->AnimMovementMode, CharacterOwner->AnimCustomMovementMode);
	// }
}


void UAdvancedMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}


void UAdvancedMovementComponent::UpdateCharacterStateBeforeMovement(const float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
	if (CharacterOwner->IsLocallyControlled() && GetWorld())
	{
		Time = GetWorld()->GetTimeSeconds(); // TODO: after a duration, have the server (uses the client's) and client time be in sync with each other and prevent cheating on the client side
	}

	// Handle Strafe sway duration
	if (IsStrafeSwaying() && StrafeSwayStartTime + StrafeSwayDuration <= Time)
	{
		DisableStrafeSwayPhysics();
	}

	// Handle Strafe lurch duration
	if (IsStrafeLurching() && StrafeLurchStartTime + StrafeLurchDuration <= Time)
	{
		DisableStrafeLurchPhysics();
	}
	
	// Slide
	if (!IsSliding() && CanSlide() && Velocity.Size2D() > SlideEnterThreshold && WalkingStartTime + WalkingDurationToSlide <= Time)
	{
		SetMovementMode(MOVE_Custom, MOVE_Custom_Slide);
	}

	if (bDebugNetworkReplication)
	{
		UE_LOGFMT(Movement, Log, "{0}::{1} -> Time: ({2}), PlayerInput: ({3}) Sprinting: ({4}), WallJumping: ({5})",
			CharacterOwner->HasAuthority() ? "Server" : "Client",
			*GetNameSafe(CharacterOwner),
			*FString::SanitizeFloat(Time),
			*PlayerInput.ToString(),
			SprintPressed ? "true" : "false",
			WallJumpPressed ? "true" : "false"
		);
	}
}


void UAdvancedMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
}
#pragma endregion 




//------------------------------------------------------------------------------//
// Physics Functions															//
//------------------------------------------------------------------------------//
void UAdvancedMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	const float BaseSpeed = Velocity.Size2D();
	FRotator MovementRotation(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
	FVector OldVelocity = Velocity;

	FVector AccelDir = Acceleration.GetSafeNormal();
	// FVector AccelDir;
	
	//--------------------------------------------------------------------------------------------------------------//
	// Sliding																										//
	//--------------------------------------------------------------------------------------------------------------//
	if (IsSliding())
	{
		// Only Allow side strafing and prevent velocity being added during a strafe
		Acceleration = FVector(UKismetMathLibrary::GetRightVector(MovementRotation) * PlayerInput.Y).GetSafeNormal() * Acceleration.Size2D();
		
		// Find strafe
		AccelDir = (UKismetMathLibrary::GetRightVector(MovementRotation) * PlayerInput.Y).GetSafeNormal(); // Allow sideways movements, however don't add to the velocity
		const float ProjVelocity = Velocity.X * AccelDir.X + Velocity.Y * AccelDir.Y; // Strafing subtracts from this value, neutral defaults to the player speed, and negative values add to speed 
		const float AirSpeedCap = (GetMaxAcceleration() / 100) * StrafeSwaySpeedGainMultiplier; // Strafe sway should have more control without slowing down the character's momentum

		// Friction affects our ability to change direction. This is only done for input acceleration, not path following.
		const FVector AccelerationDir = Acceleration.GetSafeNormal();
		const float VelSize = Velocity.Size();
		Velocity = Velocity - (Velocity - AccelerationDir * VelSize) * FMath::Min(DeltaTime * Friction, 1.f);

		const float AddSpeed = Acceleration.GetClampedToMaxSize2D(AirSpeedCap).Size2D() - ProjVelocity;
		if (AddSpeed > 0.0f)
		{
			FVector RedirectedVelocity = Acceleration * SlidingRotationRate * DeltaTime;
			RedirectedVelocity = RedirectedVelocity.GetClampedToMaxSize2D(AddSpeed);

			Velocity += RedirectedVelocity;
		}

		// If strafing added extra speed while turning, add friction to slow us down
		if (Velocity.Size2D() > OldVelocity.Size2D())
		{
			Velocity = Velocity.GetSafeNormal2D() * OldVelocity.Size2D();
			ApplyVelocityBraking(DeltaTime, SlideBrakingFriction, BrakingDeceleration);
		}

		
		if (bDebugSlide)
		{
			float FloorAngle = CurrentFloor.HitResult.ImpactNormal.Dot(UpdatedComponent->GetForwardVector()) * SlideAngleFrictionMultiplier;
			float FrictionCalc = FMath::Clamp(SlidingFriction - FloorAngle, 0, SlidingFriction * 3);
			
			UE_LOGFMT(Movement, Log, "{0}::Sliding ({1}) ->  ({2})({3}) Accel/Vel: ({4})({5}), Input: ({6}), Friction/FloorAngle: ({7})({8})",
				CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"), *FString::SanitizeFloat(Time),
				*GetMovementDirection(PlayerInput),
				FMath::CeilToInt(Velocity.Size2D()),
				*FVector2D(Acceleration.X, Acceleration.Y).ToString(),
				*FVector2D(Velocity.X, Velocity.Y).ToString(),
				*PlayerInput.ToString(),
				FMath::CeilToInt(FrictionCalc),
				FMath::CeilToInt(FloorAngle)
			);
		}
		
		return;
	}
	
	
	//--------------------------------------------------------------------------------------------------------------//
	// All movement modes except for air																			//
	//--------------------------------------------------------------------------------------------------------------//
	if (MovementMode != MOVE_Falling) return Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	
	
	//--------------------------------------------------------------------------------------------------------------------------------------//
	// Third person (with orient rotation to movement) physics																				//
	//--------------------------------------------------------------------------------------------------------------------------------------//
	// Strafing calculations causes problems if the character's movement isn't oriented to their movement
	if (!bUseBhopping || bOrientRotationToMovement) return Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	// If the player's rotation is oriented to the camera, it messes up the fp strafing calculations, and this prevents that from happening
	
	
	//--------------------------------------------------------------------------------------------------------------------------------------//
	// Air Movement Logic																													//
	//--------------------------------------------------------------------------------------------------------------------------------------//
	// Do not update velocity when using root motion or when SimulatedProxy and not simulating root motion - SimulatedProxy are repped their Velocity
	if (!HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME || (CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy && !bWasSimulatingRootMotion))
	{
		return;
	}
	
	// Air strafe values
	float ProjVelocity;
	FVector AddedVelocity;
	float AirSpeedCap; // How much speed is gained during air strafing?
	float AirAccelerationMultiplier; // Drag is added to the equation if the accelerationMultiplier / 10 isn't the same as the player's velocity 
	float AddSpeed;

	
	//------------------------------------------------------------------------------------------------------------------//
	// AirStrafe Sway ->  Prevent Air resistance during specific movement actions (Wall jumps, ledge jumps, etc.)		//
	//------------------------------------------------------------------------------------------------------------------//
	if (IsStrafeSwaying())
	{
		// Find strafe
		ProjVelocity = Velocity.X * AccelDir.X + Velocity.Y * AccelDir.Y; // Strafing subtracts from this value, neutral defaults to the player speed, and negative values add to speed 
		AirSpeedCap = (GetMaxAcceleration() / 100) * StrafeSwaySpeedGainMultiplier; // Strafe sway should have more control without slowing down the character's momentum
		AirAccelerationMultiplier = StrafeSwayRotationRate;
		
		// Update the acceleration based on the character's air control
		Acceleration = GetFallingLateralAcceleration(DeltaTime);

		// Allow Strafing, don't let the player stop the momentum from pressing an input in the opposite direction of the momentum
		AddSpeed = Acceleration.GetClampedToMaxSize2D(AirSpeedCap).Size2D() - ProjVelocity;
		if (AddSpeed > 0.0f)
		{
			AddedVelocity = Acceleration * AirAccelerationMultiplier * AirControl * DeltaTime;
			AddedVelocity = AddedVelocity.GetClampedToMaxSize2D(AddSpeed);

			// Don't allow the player to move against the strafe
			const float AngleFromCurrentVelocity = Velocity.GetSafeNormal2D().Dot(AccelDir); 
			if (AngleFromCurrentVelocity > -0.34) Velocity += AddedVelocity;
		}
	}

	//------------------------------------------------------------------------------------------------------------------//
	// AirStrafe Lurch ->  Air influence during specific movement actions (Mantle jumps, ledge jumps, etc.)				//
	//------------------------------------------------------------------------------------------------------------------//
	else if (IsStrafeLurching())
	{
		FVector AirStrafeVelocity = OldVelocity;
		FVector AirStrafeLurchVelocity = OldVelocity;
		
		// Update the acceleration based on the character's air control
		Acceleration = GetFallingLateralAcceleration(DeltaTime);
		
		/**** Air Strafe calculations ****/
		ProjVelocity = Velocity.X * AccelDir.X + Velocity.Y * AccelDir.Y; // Strafing subtracts from this value, neutral defaults to the player speed, and negative values add to speed 
		AirSpeedCap = (GetMaxAcceleration() / 100) * AirStrafeSpeedGainMultiplier; // How much speed is gained during air strafing?
		AirAccelerationMultiplier = AirStrafeRotationRate; // Drag is added to the equation if the accelerationMultiplier / 10 isn't the same as the player's velocity 
		
		// Update the acceleration based on the character's air control
		Acceleration = GetFallingLateralAcceleration(DeltaTime);
		
		// Add strafing momentum to the character's velocity 
		AddSpeed = Acceleration.GetClampedToMaxSize2D(AirSpeedCap).Size2D() - ProjVelocity;
		if (AddSpeed > 0.0f)
		{
			AddedVelocity = Acceleration * AirAccelerationMultiplier * AirControl * DeltaTime;
			AddedVelocity = AddedVelocity.GetClampedToMaxSize2D(AddSpeed);
			AirStrafeVelocity += AddedVelocity;
		}

		
		/**** Strafe Lurch influence ****/
		if (AccelDir.IsNearlyZero()) AirStrafeLurchVelocity = Velocity;
		else AirStrafeLurchVelocity = AccelDir * BaseSpeed;
		
		// Apply friction
		FVector FrictionVector = OldVelocity - AirStrafeLurchVelocity;
		FVector Friction = FrictionVector * (StrafeLurchFriction * 10) * DeltaTime;
		AirStrafeLurchVelocity += Friction;
		
		// Apply input acceleration
		// if (!Acceleration.IsNearlyZero())
		// {
		// 	const float MaxInputSpeed = FMath::Max(GetMaxSpeed() * AnalogInputModifier, GetMinAnalogSpeed());
		// 	const float NewMaxInputSpeed = IsExceedingMaxSpeed(MaxInputSpeed) ? Velocity.Size() : MaxInputSpeed;
		// 	AirStrafeLurchVelocity += Acceleration * DeltaTime;
		// 	AirStrafeLurchVelocity = Velocity.GetClampedToMaxSize(NewMaxInputSpeed);
		// }
		

		/**** Velocity calculations ****/
		float LurchStrength;
		float Duration = StrafeLurchStartTime + StrafeLurchDuration - Time;
		if (StrafeLurchStartTime + StrafeLurchFullStrengthDuration > Time) LurchStrength = 1;
		else LurchStrength = UKismetMathLibrary::MapRangeClamped(Duration, 0, StrafeLurchDuration - StrafeLurchFullStrengthDuration, 0, 1);
		LurchStrength = FMath::Clamp(LurchStrength * StrafeLurchStrength, 0, 1);

		FVector AirStrafeInfluence = AirStrafeVelocity * (1 - LurchStrength);
		FVector AirStrafeLurchInfluence = AirStrafeLurchVelocity * LurchStrength;
		Velocity = AirStrafeInfluence + AirStrafeLurchInfluence;

		if (bDebugStrafeLurch)
		{
			float PrevSpeed = OldVelocity.Size2D();
			float Speed = Velocity.Size2D();
			// Velocity/AccelDir // Speed (+/- Adjusted) // Proj/Added Velocity / AccelMultiplier
			UE_LOGFMT(Movement, Log, "{0}::{1} ({2}) ->  ({3})({4}) Vel/AccelDir: ({5})({6}) Speed: ({7})({8}), AirStrafe/Lurch Vel: ({9})({10}), Proj/Add Vel: ({11})({12})",
				CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"), *FString("StrafeLurch"),
				*FString::SanitizeFloat(StrafeLurchStartTime + StrafeLurchDuration - Time),
				*GetMovementDirection(PlayerInput), Speed - PrevSpeed > 0 ? *FString("+") : *FString("-"),
				*Velocity.ToString(), *FVector2D(AccelDir.X, AccelDir.Y).ToString(),
				FMath::CeilToInt(Speed), FMath::CeilToInt(Speed - PrevSpeed),
				*AirStrafeVelocity.ToString(), *AirStrafeLurchVelocity.ToString(),
				FMath::CeilToInt(ProjVelocity), *AddedVelocity.ToString()
			);

			// UE_LOGFMT(Movement, Log, "StrafeLurchTime: {0}, Strength: {1}, AirStrafeInfluence: {2}, StrafeLurchInfluence: {3}",
			// 	*FString::SanitizeFloat(Duration), *FString::SanitizeFloat(LurchStrength),
			// 	*AirStrafeInfluence.ToString(), *AirStrafeLurchInfluence.ToString()
			// );
		}
	}
	
	//--------------------------------------------------------------------------------------------------------------//
	// Strafing / Bhopping physics																					//
	//--------------------------------------------------------------------------------------------------------------//
	else
	{
		// Find strafe
		ProjVelocity = Velocity.X * AccelDir.X + Velocity.Y * AccelDir.Y; // Strafing subtracts from this value, neutral defaults to the player speed, and negative values add to speed 
		AirSpeedCap = (GetMaxAcceleration() / 100) * AirStrafeSpeedGainMultiplier; // How much speed is gained during air strafing?
		AirAccelerationMultiplier = AirStrafeRotationRate; // Drag is added to the equation if the accelerationMultiplier / 10 isn't the same as the player's velocity 
		
		// Update the acceleration based on the character's air control
		Acceleration = GetFallingLateralAcceleration(DeltaTime);
		
		// Add strafing momentum to the character's velocity 
		AddSpeed = Acceleration.GetClampedToMaxSize2D(AirSpeedCap).Size2D() - ProjVelocity;
		if (AddSpeed > 0.0f)
		{
			AddedVelocity = Acceleration * AirAccelerationMultiplier * AirControl * DeltaTime;
			AddedVelocity = AddedVelocity.GetClampedToMaxSize2D(AddSpeed);
			Velocity += AddedVelocity;
		}
	}
	
	// MovementInput, Gain/Lose Speed, AddedVelocity, Velocity, AirSpeedCap, AirAccelMultiplier
	if (bDebugAirStrafe || bDebugStrafeSway)
	{
		if (IsStrafeSwaying() && !bDebugStrafeSway) return;
		else if (IsStrafeLurching()) return;
		else if (!bDebugAirStrafe) return;

		float PrevSpeed = OldVelocity.Size2D();
		float Speed = Velocity.Size2D();
		// Velocity/AccelDir // Speed (+/- Adjusted) // Proj/Added Velocity / AccelMultiplier
		UE_LOGFMT(Movement, Log, "{0}::{1} ({2}) ->  ({3})({4}) Vel/AccelDir: ({5})({6}) Speed: ({7})({8}), ProjVel: ({9}), Added Vel: ({10})",
			CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
			IsStrafeSwaying() ? *FString("StrafeSway") : *FString("AirStrafe"),
			IsStrafeSwaying() ? *FString::SanitizeFloat(StrafeSwayStartTime + StrafeSwayDuration - Time) : *FString(""),
			*GetMovementDirection(PlayerInput),
			Speed - PrevSpeed > 0 ? *FString("+") : *FString("-"),
			*Velocity.ToString(),
			*FVector2D(AccelDir.X, AccelDir.Y).ToString(),
			FMath::CeilToInt(Speed),
			FMath::CeilToInt(Speed - PrevSpeed),
			FMath::CeilToInt(ProjVelocity),
			*AddedVelocity.ToString()
		);
	}
}


void UAdvancedMovementComponent::StartNewPhysics(float deltaTime, int32 Iterations)
{
	Super::StartNewPhysics(deltaTime, Iterations);
}


void UAdvancedMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(CharPhysWalking);

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// null character and root motion checks
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	// null safety check
	if (!UpdatedComponent->IsQueryCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	GroundMovementPhysics(deltaTime, Iterations);
}


void UAdvancedMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// null character and root motion checks
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}
	
	// if the player stops crouching or presses away from the slide then stop sliding
	if (PlayerInput.X < -0.1 || !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	// check if they're still sliding
	if (!CanSlide())
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	GroundMovementPhysics(deltaTime, Iterations);
}


void UAdvancedMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	if (CustomMovementMode == MOVE_Custom_Slide) PhysSlide(deltaTime, Iterations);
	if (CustomMovementMode == MOVE_Custom_WallClimbing) PhysWallClimbing(deltaTime, Iterations);
	if (CustomMovementMode == MOVE_Custom_Mantling) PhysMantling(deltaTime, Iterations);
	if (CustomMovementMode == MOVE_Custom_LedgeClimbing) PhysLedgeClimbing(deltaTime, Iterations);
	if (CustomMovementMode == MOVE_Custom_WallRunning) PhysWallRunning(deltaTime, Iterations);
}


void UAdvancedMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	
	// Setup physics sub steps
	float remainingTime = deltaTime;
	while( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) )
	{
		Iterations++;
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// UE_LOGFMT(Movement, Log, "{0} Physics(Falling): {2}", CharacterOwner->HasAuthority() ? "Server" : "Client", Time);
		
		// save the current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		FVector Adjusted;
		bJustTeleported = false;

		const FVector OldVelocityWithRootMotion = Velocity;
		RestorePreAdditiveRootMotionVelocity();
		const FVector OldVelocity = Velocity;
		
		// Compute current gravity
		FVector Gravity = FVector(0.f, 0.f, GetGravityZ());
		float GravityTime = timeTick;
		
		HandleFallingFunctionality( deltaTime, timeTick, Iterations, remainingTime, OldVelocity, OldVelocityWithRootMotion, Adjusted, Gravity, GravityTime);
		FallingMovementPhysics( deltaTime, timeTick, Iterations, remainingTime, OldLocation, OldVelocity, PawnRotation, Adjusted, true /* bHasLimitedAirControl */, Gravity, GravityTime);
	}
}


void UAdvancedMovementComponent::PhysWallClimbing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// null character and root motion checks
	if (!CharacterOwner || !UpdatedComponent || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	// Wall climb duration
	if (WallClimbDuration != 0 && WallClimbStartTime + WallClimbDuration < Time)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	// movement physics
	float remainingTime = deltaTime;
	while( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) )
	{
		Iterations++; 
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		
		// save the current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		FVector Adjusted;
		bJustTeleported = false;

		RestorePreAdditiveRootMotionVelocity();
		FVector AccelDir = Acceleration.GetSafeNormal();
		const FVector OldVelocity = Velocity;
		
		// Check if the player is trying to wall jump
		FHitResult JumpHit;
		if (WallJumpValid(deltaTime, OldLocation, AccelDir, JumpHit, FHitResult()))
		{
			CalculateWallJumpTrajectory(timeTick, Iterations, JumpHit, WallJumpSpeed, WallJumpBoostDuringWallClimbs, FString("WallClimb"));
			return;
		}
		
		// if they stop climbing the wall transition to air
		if (PlayerInput.IsNearlyZero())
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(deltaTime, Iterations);
			return;
		}
		
		// Move forwards, up, and sideways if they're also moving to the side
		FVector WallClimbVector = FVector();
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			if (Velocity.Z < WallClimbGravityLimit)
			{
				ApplyVelocityBraking(timeTick, WallClimbFriction, GetMaxBrakingDeceleration());
				Adjusted = Velocity * timeTick;
			}
			if (Velocity.Z > WallClimbGravityLimit + WallClimbAddSpeedThreshold)
			{
				// if they aren't climbing don't add vertical speed, however add a multiplier for how much speed should be added, and limit it to their input
				WallClimbVector = FVector( AccelDir.X * WallClimbMultiplier.X, AccelDir.Y * WallClimbMultiplier.X, 1 * WallClimbMultiplier.Y);
				Velocity = FVector(Velocity + WallClimbVector * WallClimbAcceleration).GetClampedToSize(0, WallClimbSpeed);
				Adjusted = Velocity * timeTick;
			}
		}

		// check if they're wall climbing
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit); // Moves based on adjusted, updates velocity, and handles returning colliding information for handling the different movement scenarios
		if (Hit.IsValidBlockingHit())
		{
			float LastMoveTimeSlice = timeTick;
			float subTimeTickRemaining = timeTick * (1.f - Hit.Time);
			
			// if the character just landed on the ground
			if (Hit.bBlockingHit && IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += subTimeTickRemaining;
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			
			// if they're trying to transition to mantling/ledge climbing
			if (CheckIfSafeToMantleLedge())
			{
				FVector Location = UpdatedComponent->GetComponentLocation();
				if (Location.Z <= MantleLedgeLocation.Z) SetMovementMode(MOVE_Custom, MOVE_Custom_Mantling);
				else SetMovementMode(MOVE_Custom, MOVE_Custom_LedgeClimbing);
				StartNewPhysics(deltaTime, Iterations);
				return;
			}
			
			// transition out of wall climbing if they aren't trying to climb the wall
			const FVector ForwardVector = bOrientRotationToMovement ? AccelDir : UpdatedComponent->GetForwardVector();
			const float Angle = 180 - UKismetMathLibrary::DegAcos(Hit.Normal.Dot(ForwardVector));
			if (Angle > WallClimbAcceptableAngle)
			{
				SetMovementMode(MOVE_Falling);
				StartNewPhysics(timeTick, Iterations);
				return;
			}

			// Handle moving up and alongside the wall
			HandleImpact(Hit, LastMoveTimeSlice, Adjusted);
			FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, Hit.Normal, Hit);
			SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
			
			if (bDebugWallClimb)
			{
				UE_LOGFMT(Movement, Log, "{0}::WallClimb ({1}) ->  ({2})({3}) Adjusted: ({4}), Vel: ({5}), WallClimbVector: ({6}), PlayerInput: ({7}), Speed: ({8}), Acceleration: ({9}), Multiplier: ({10})",
					CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
					*FString::SanitizeFloat(WallClimbStartTime + WallClimbDuration - Time),
					*FString::SanitizeFloat(Angle),
					*FString::SanitizeFloat(Adjusted.Size2D()),
					*Adjusted.ToString(),
					*Velocity.ToString(),
					*WallClimbVector.ToString(),
					*PlayerInput.ToString(),
					*FString::SanitizeFloat(WallClimbSpeed),
					*FString::SanitizeFloat(WallClimbAcceleration),
					*WallClimbMultiplier.ToString()
				);
			}
		}
		else
		{
			SetMovementMode(MOVE_Falling);
		}
	}
}


void UAdvancedMovementComponent::PhysMantling(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// null character and root motion checks
	if (!CharacterOwner || !UpdatedComponent || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	// if the player presses away from the wall then return to the falling state
	const float PlayerAngle = -LedgeClimbNormal.Dot(Acceleration.GetSafeNormal());
	if (!PlayerInput.IsNearlyZero(0.1) && PlayerAngle <= 0.34)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	// Setup physics sub steps
	float remainingTime = deltaTime;
	while( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) )
	{
		Iterations++;
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save the current values
		FVector Adjusted;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();

		// The player is transitioning to the ledge
		if (!UpdatedComponent->GetComponentLocation().Equals(MantleLedgeLocation, 0.1))
		{
	
			// Find the interp rotation
			const FRotator MantleRotation = (-LedgeClimbNormal).Rotation();
			const FRotator PlayerRotation = UpdatedComponent->GetComponentRotation();
			FRotator TargetRotation = FRotator(0, MantleRotation.Yaw, 0);
			
			// Use speed adjustments to create your own ease in transitions
			const float CurrentPercent = (MantleLedgeLocation - OldLocation).Size() / (MantleLedgeLocation - MantleStartLocation).Size();; // 0-1
			const float InterpSpeedAdjustments = MantleRotationSpeedAdjustments ? FMath::Clamp(MantleRotationSpeedAdjustments->GetFloatValue(CurrentPercent * 10), 0.1, 10) : 1;
			
			// UKismetMathLibrary::RInterpTo();
			FRotator Delta = (TargetRotation - PlayerRotation).GetNormalized();
			FRotator AdjustedRotation = Delta * timeTick * MantleRotationSpeed * InterpSpeedAdjustments;
			
			// UE_LOGFMT(Movement, Log, "MantleRotation: {0}, PlayerRotation: {1}, TargetRotation: {2}, AdjustedRotation: {3}",
			// 	MantleRotation.Yaw, PlayerRotation.Yaw, TargetRotation.Yaw, AdjustedRotation.Yaw);
			
			// Interp the character to the target location
			Adjusted = MantleAndClimbInterp(timeTick, MantleStartLocation, MantleLedgeLocation, OldLocation, MantleSpeed, MantleSpeedAdjustments);
			FHitResult Hit;
			SafeMoveUpdatedComponent(Adjusted, (PlayerRotation + AdjustedRotation).GetNormalized(), false, Hit);

			// TODO: Error handling
			
			if (bDebugMantle)
			{
				UE_LOGFMT(Movement, Log, "{0}::Mantling ({1}) ->  ({2})({3}) Mantle/Location: ({4})({5}), Vector/Adjusted: ({6})({7}), Speed: ({8}), PlayerAngle: ({9})",
					CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
					*FString::SanitizeFloat(Time - MantleStartTime),
					*GetMovementDirection(PlayerInput),
					Adjusted.Z > 0 ? FString("+") : FString("-"),
					*MantleLedgeLocation.ToString(),
					*OldLocation.ToString(),
					*(MantleLedgeLocation - OldLocation).GetSafeNormal().ToString(),
					*Adjusted.ToString(),
					Adjusted.Size(),
					*FString::SanitizeFloat(PlayerAngle)
				);
			}
		}
		else
		{
			// if the player presses forward, climb up on the ledge.
			if (bUseLedgeClimbing && !PlayerInput.IsNearlyZero(0.1) && PlayerAngle > 0.64)
			{
				SetMovementMode(MOVE_Custom, MOVE_Custom_LedgeClimbing);
				StartNewPhysics(deltaTime, Iterations);
				return;
			}
		}
	}
}


void UAdvancedMovementComponent::PhysLedgeClimbing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// null character and root motion checks
	if (!CharacterOwner || !UpdatedComponent || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}
	
	// Setup physics sub steps
	float remainingTime = deltaTime;
	while( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) )
	{
		Iterations++;
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save the current values
		FVector Adjusted;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		
		// The player is transitioning to the ledge
		const float CharacterHeightOffset = CharacterOwner->GetCapsuleComponent() ? CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + LedgeClimbOffset : 90 + LedgeClimbOffset;
		if (!UpdatedComponent->GetComponentLocation().Equals(LedgeClimbLocation + FVector(0, 0, CharacterHeightOffset), 0.1))
		{
			// Interp the character to the target location
			Adjusted = MantleAndClimbInterp(timeTick, LedgeClimbStartLocation, LedgeClimbLocation + FVector(0, 0, CharacterHeightOffset), OldLocation, CurrentClimbSpeed, CurrentClimbSpeedAdjustments);
			FHitResult Hit;
			SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentRotation(), false, Hit);

			// TODO: Error handling
			
			if (bDebugLedgeClimb)
			{
				UE_LOGFMT(Movement, Log, "{0}::LedgeClimbing ({1}) ->  ({2})({3}) Ledge/Location: ({4})({5}), Vector/Adjusted: ({6})({7}), Speed: ({8})",
					CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
					*FString::SanitizeFloat(Time - LedgeClimbStartTime),
					*GetMovementDirection(PlayerInput),
					Adjusted.Z > 0 ? FString("+") : FString("-"),
					*LedgeClimbLocation.ToString(),
					*OldLocation.ToString(),
					*(LedgeClimbLocation - OldLocation).GetSafeNormal().ToString(),
					*Adjusted.ToString(),
					Adjusted.Size()
				);
			}
		}
		else
		{
			Velocity = FVector();
			MantleJumpStartTime = Time;
			MantleJumpLocation = UpdatedComponent->GetComponentLocation();
			
			SetMovementMode(MOVE_Walking);
			StartNewPhysics(deltaTime, Iterations);
			return;
		}
	}
}


void UAdvancedMovementComponent::PhysWallRunning(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	
	// null character and root motion checks
	if (!CharacterOwner || !UpdatedComponent || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	// Wall run duration
	if (WallRunStartTime + WallRunDuration < Time)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	// movement physics
	float remainingTime = deltaTime;
	while( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) )
	{
		Iterations++; 
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		
		// save the current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		FVector Adjusted;
		bJustTeleported = false;

		RestorePreAdditiveRootMotionVelocity();
		FVector AccelDir = Acceleration.GetSafeNormal();
		
		// Check if the player is trying to wall jump
		FHitResult JumpHit;
		if (WallJumpValid(deltaTime, OldLocation, AccelDir, JumpHit, FHitResult()))
		{
			CalculateWallJumpTrajectory(timeTick, Iterations, JumpHit, WallJumpSpeed, WallJumpBoostDuringWallRuns, FString("WallRun"));
			return;
		}

		// if they move away or let go of the wall wall transition to air
		if ((WallRunInputDirection < 0 && PlayerInput.Y > -0.1) || (WallRunInputDirection > 0 && PlayerInput.Y < 0.1))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(timeTick, Iterations);
			return;
		}
		
		// Move forwards and towards the wall to allow for traversing angled walls
		FVector WallRunVector = FVector();
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			// Calculate the forward input based on the wall
			const FRotator CharacterRotation(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
			FVector CharacterInput = (
				// FVector(UKismetMathLibrary::GetForwardVector(CharacterRotation) * PlayerInput.X) +
				FVector(UKismetMathLibrary::GetRightVector(CharacterRotation) * PlayerInput.Y)
			).GetSafeNormal();
			
			// Find which way the character should wall run
			float WallDirection = WallRunNormal.Dot(UpdatedComponent->GetForwardVector()); // The impact normal of the wall is based on the current surface
			float PlayerForwardsDirection = UpdatedComponent->GetForwardVector().Dot(WallRunWall->GetRightVector()); // The Get vectors of the wall are to help translate things in world space
			FVector WallRunDirection = PlayerForwardsDirection < 0 ? WallRunWall->GetRightVector() * -1 : WallRunWall->GetRightVector();
			if (bShouldRunBackwardsIfFacingAwayFromWall && WallDirection > 0) WallRunDirection *= -1;
			WallRunVector = FVector(
				FMath::Clamp(CharacterInput.X + WallRunDirection.X, -1, 1) * WallRunMultiplier.X,
				FMath::Clamp(CharacterInput.Y + WallRunDirection.Y, -1, 1) * WallRunMultiplier.X,
				WallRunMultiplier.Y
			);
			
			if (bDebugWallTraces)
			{
				DrawDebugLine(GetWorld(), OldLocation, OldLocation + WallRunNormal * 50, FColor::Emerald, false, TraceDuration);
				DrawDebugLine(GetWorld(), OldLocation, OldLocation + WallRunDirection * 100 + FVector(0, 0, 2), FColor::Cyan, false, TraceDuration);
			}

			// Don't exceed the max speed, but don't slow down current speeds
			Velocity.Z = 0;
			Velocity = FVector(Velocity + (WallRunVector * WallRunAcceleration)).GetClampedToSize(0, WallRunCurrentSpeed);
			Adjusted = Velocity * timeTick;
		}
		
		// check if there's a wall
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit); // Moves based on adjusted, updates velocity, and handles returning colliding information for handling the different movement scenarios
		if (Hit.IsValidBlockingHit())
		{
			float LastMoveTimeSlice = timeTick;
			float subTimeTickRemaining = timeTick * (1.f - Hit.Time);
			WallRunNormal = Hit.Normal;
			WallRunWall = Hit.GetComponent();
			
			// if the character just landed on the ground
			if (Hit.bBlockingHit && IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += subTimeTickRemaining;
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			
			// transition out of wall running if they aren't trying to wall run
			const float Angle = 180 - UKismetMathLibrary::DegAcos(Hit.Normal.Dot(UpdatedComponent->GetForwardVector()));
			if (Angle < 90 - WallRunAcceptableAngleRadius || Angle > 90 + WallRunAcceptableAngleRadius)
			{
				SetMovementMode(MOVE_Falling);
				StartNewPhysics(timeTick, Iterations);
				return;
			}
			
			// Handle velocity calculations running alongside the wall
			HandleImpact(Hit, LastMoveTimeSlice, Adjusted);
			FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, Hit.Normal, Hit);
			SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

			if (bDebugWallRunning)
			{
				float AddedSpeed = Adjusted.Size2D();
				UE_LOGFMT(Movement, Log, "{0}::WallRunning ({1}) ->  ({2})({3}) Adjusted: ({4}), Vel: ({5}), WallRunVector: ({6}), PlayerInput: ({7}), Angle: ({8}), SpeedCap: {9}, WallRunMultiplier: ({10})",
					CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"), *FString::SanitizeFloat(WallRunStartTime + WallRunDuration - Time),
					WallRunNormal.Dot(UpdatedComponent->GetRightVector()) < 0 ? *FString("L") : *FString("R"),
					*FString::SanitizeFloat(AddedSpeed),
					*Adjusted.ToString(),
					*Velocity.ToString(),
					*WallRunVector.ToString(),
					*PlayerInput.ToString(),
					*FString::SanitizeFloat(Angle),
					*FString::SanitizeFloat(WallRunSpeed),
					*WallRunMultiplier.ToString()
				);
			}
		}
		else
		{
			SetMovementMode(MOVE_Falling);
		}
	}
}


//------------------------------------------------------------------------------//
// Falling Movement Logic														//
//------------------------------------------------------------------------------//
#pragma region Falling Logic
void UAdvancedMovementComponent::HandleFallingFunctionality(float deltaTime, float& timeTick, int32& Iterations, float& remainingTime,
	const FVector& OldVelocity, const FVector& OldVelocityWithRootMotion, FVector& Adjusted,
	FVector& Gravity, float& GravityTime)
{
	// Apply input
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity.Z = 0.f;
		CalcVelocity(timeTick, 0, false, GetMaxBrakingDeceleration());
		Velocity.Z = OldVelocity.Z;
	}

	// // If jump is providing force, gravity may be affected.
	bool bEndingJumpForce = false;
	if (CharacterOwner->JumpForceTimeRemaining > 0.0f)
	{
		// Consume some of the force time. Only the remaining time (if any) is affected by gravity when bApplyGravityWhileJumping=false.
		const float JumpForceTime = FMath::Min(CharacterOwner->JumpForceTimeRemaining, timeTick);
		GravityTime = bApplyGravityWhileJumping ? timeTick : FMath::Max(0.0f, timeTick - JumpForceTime);
		
		// Update Character state
		CharacterOwner->JumpForceTimeRemaining -= JumpForceTime;
		if (CharacterOwner->JumpForceTimeRemaining <= 0.0f)
		{
			CharacterOwner->ResetJumpState();
			bEndingJumpForce = true;
		}
	}
	
	// Apply gravity
	Velocity = NewFallVelocity(Velocity, Gravity, GravityTime);
	// UE_LOG(Movement, Log, TEXT("dt=(%.6f) OldLocation=(%s) OldVelocity=(%s) OldVelocityWithRootMotion=(%s) NewVelocity=(%s)"), timeTick, *(UpdatedComponent->GetComponentLocation()).ToString(), *OldVelocity.ToString(), *OldVelocityWithRootMotion.ToString(), *Velocity.ToString());

	// Root motion and friction
	ApplyRootMotionToVelocity(timeTick);
	DecayFormerBaseVelocity(timeTick);
	
	// See if we need to sub-step to exactly reach the apex. This is important for avoiding "cutting off the top" of the trajectory as framerate varies.
	if (/* CharacterMovementCVars::ForceJumpPeakSubstep && */ OldVelocityWithRootMotion.Z > 0.f && Velocity.Z <= 0.f && NumJumpApexAttempts < MaxJumpApexAttemptsPerSimulation)
	{
		const FVector DerivedAccel = (Velocity - OldVelocityWithRootMotion) / timeTick;
		if (!FMath::IsNearlyZero(DerivedAccel.Z))
		{
			const float TimeToApex = -OldVelocityWithRootMotion.Z / DerivedAccel.Z;
			
			// The time-to-apex calculation should be precise, and we want to avoid adding a substep when we are basically already at the apex from the previous iteration's work.
			const float ApexTimeMinimum = 0.0001f;
			if (TimeToApex >= ApexTimeMinimum && TimeToApex < timeTick)
			{
				const FVector ApexVelocity = OldVelocityWithRootMotion + (DerivedAccel * TimeToApex);
				Velocity = ApexVelocity;
				Velocity.Z = 0.f; // Should be nearly zero anyway, but this makes apex notifications consistent.
	
				// We only want to move the amount of time it takes to reach the apex, and refund the unused time for next iteration.
				const float TimeToRefund = (timeTick - TimeToApex);
	
				remainingTime += TimeToRefund;
				timeTick = TimeToApex;
				Iterations--;
				NumJumpApexAttempts++;
	
				// Refund time to any active Root Motion Sources as well
				for (TSharedPtr<FRootMotionSource> RootMotionSource : CurrentRootMotion.RootMotionSources)
				{
					const float RewoundRMSTime = FMath::Max(0.0f, RootMotionSource->GetTime() - TimeToRefund);
					RootMotionSource->SetTime(RewoundRMSTime);
				}
			}
		}
	}

	if (bNotifyApex && (Velocity.Z < 0.f))
	{
		// Just passed jump apex since now going down
		bNotifyApex = false;
		NotifyJumpApex();
	}

	// Compute change in position (using midpoint integration method).
	Adjusted = 0.5f * (OldVelocityWithRootMotion + Velocity) * timeTick;
	
	// Special handling if ending the jump force where we didn't apply gravity during the jump.
	if (bEndingJumpForce && !bApplyGravityWhileJumping)
	{
		// We had a portion of the time at constant speed then a portion with acceleration due to gravity.
		// Account for that here with a more correct change in position.
		const float NonGravityTime = FMath::Max(0.f, timeTick - GravityTime);
		Adjusted = (OldVelocityWithRootMotion * NonGravityTime) + (0.5f*(OldVelocityWithRootMotion + Velocity) * GravityTime);
	}
}


void UAdvancedMovementComponent::FallingMovementPhysics(float deltaTime, float& timeTick, int32& Iterations, float& remainingTime,
	const FVector& OldLocation, const FVector& OldVelocity, const FQuat& PawnRotation, FVector& Adjusted,
	bool bHasLimitedAirControl, FVector& Gravity, float& GravityTime)
{
	// Move the character based on the updated velocity and acceleration
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent( Adjusted, PawnRotation, true, Hit);
	
	if (!HasValidData())
	{
		return;
	}
	
	float LastMoveTimeSlice = timeTick;
	float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

	// just entered water
	if ( IsSwimming() )
	{
		remainingTime += subTimeTickRemaining;
		StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
		return;
	}
	
	// if the character just landed on the ground
	if (Hit.bBlockingHit && IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
	{
		remainingTime += subTimeTickRemaining;
		ProcessLanded(Hit, remainingTime, Iterations);
		return;
	}

	// Wall jump and wall movement both need the player's input for calculations
	const FRotator CharacterRotation(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
	// FVector AccelDir = (FVector(UKismetMathLibrary::GetForwardVector(CharacterRotation) * PlayerInput.X) + FVector(UKismetMathLibrary::GetRightVector(CharacterRotation) * PlayerInput.Y)).GetSafeNormal(); // Walls adjust this
	FVector AccelDir = Acceleration.GetSafeNormal();

	
	// Check if the player is trying to wall jump
	FHitResult JumpHit;
	if (WallJumpValid(deltaTime, OldLocation, AccelDir, JumpHit, Hit))
	{
		CalculateWallJumpTrajectory(timeTick, Iterations, JumpHit, WallJumpSpeed, WallJumpBoost, FString("Air Strafe"));
		return;
	}
	
	// Handle impact deflections
	if ( Hit.bBlockingHit )
	{
		// Compute impact deflection based on final velocity, not integration step.
		// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
		Adjusted = Velocity * timeTick;

		
		// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
		if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
		{
			const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
			FFindFloorResult FloorResult;
			FindFloor(PawnLocation, FloorResult, false);
			if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
			{
				remainingTime += subTimeTickRemaining;
				ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
				return;
			}
		}
		
		// Wall Climb
		if (CanWallClimb() && TryingToClimbWall(Hit.Normal))
		{
			PrevWallClimbLocation = Hit.Location;
			PrevWallClimbNormal = Hit.ImpactNormal;
			SetMovementMode(MOVE_Custom, MOVE_Custom_WallClimbing);
			StartNewPhysics(deltaTime, Iterations);
		}
		
		// Wall Run
		const float Speed = Velocity.Size2D();
		const float Angle = 180 - UKismetMathLibrary::DegAcos(Hit.Normal.Dot(UpdatedComponent->GetForwardVector()));
		if (CanWallRun(Hit) && Speed > WallRunSpeedThreshold && Angle > 90 - WallRunAcceptableAngleRadius && Angle < 90 + WallRunAcceptableAngleRadius)
		{
			WallRunWall = Hit.GetComponent();
			WallRunNormal = Hit.Normal;
			WallRunLocation = Hit.ImpactNormal;
			SetMovementMode(MOVE_Custom, MOVE_Custom_WallRunning);
			StartNewPhysics(deltaTime, Iterations);
		}

		
		// If we've changed physics mode, abort.
		HandleImpact(Hit, LastMoveTimeSlice, Adjusted);
		if (!HasValidData() || !IsFalling())
		{
			return;
		}

		// Limit air control based on what we hit.
		// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
		FVector VelocityNoAirControl = OldVelocity;
		FVector AirControlAccel = Acceleration;
		if (bHasLimitedAirControl)
		{
			FVector AirControlDeltaV;
			AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;
			AirControlDeltaV = AirControlAccel;
			if (!IsValidLandingSpot(Hit.Location, Hit))
			{
				float WallAngle = AirControlAccel.GetSafeNormal().Dot(Hit.Normal);
				if (WallAngle < 0.f)
				{
					// Allow movement parallel to the wall based on the character's current movements and their input, but not into it because that may push us up.
					// TODO: this needs to factor in the character inputs to allow traversing slopes
					FVector AirAndMovementAccel = Velocity.GetSafeNormal() + AirControlAccel.GetSafeNormal();
					AirControlDeltaV = FVector::VectorPlaneProject(AirAndMovementAccel, Hit.Normal);
				}
			}

			AirControlDeltaV *= LastMoveTimeSlice;
			Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
			if (bDebugAirStrafeTrace)
			{
				DrawDebugLine(GetWorld(), OldLocation + FVector(0, 0, 64), (OldLocation + FVector(0, 0, 64)) + (Adjusted.GetSafeNormal() * 100), FColor::Emerald, false, TraceDuration);
				DrawDebugLine(GetWorld(), OldLocation + FVector(0, 0, 30), (OldLocation + FVector(0, 0, 30)) + (AirControlAccel.GetSafeNormal() * 100), FColor::Blue, false, TraceDuration);
				DrawDebugLine(GetWorld(), OldLocation + FVector(0, 0, 27), (OldLocation + FVector(0, 0, 27)) + (AirControlDeltaV.GetSafeNormal() * 100), FColor::Cyan, false, TraceDuration);
			}
		}
		
		const FVector OldHitNormal = Hit.Normal;
		const FVector OldHitImpactNormal = Hit.ImpactNormal;
		FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);
		
		// Compute velocity after deflection (only gravity component for RootMotion)
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (/* PlayerCharacterMovementCVars::UseTargetVelocityOnImpact && */ !Velocity.IsNearlyZero() && MovementBaseUtility::IsSimulatedBase(HitComponent))
		{
			const FVector ContactVelocity = MovementBaseUtility::GetMovementBaseVelocity(HitComponent, NAME_None) + MovementBaseUtility::GetMovementBaseTangentialVelocity(HitComponent, NAME_None, Hit.ImpactPoint);
			const FVector NewVelocity = Velocity - Hit.ImpactNormal * FVector::DotProduct(Velocity - ContactVelocity, Hit.ImpactNormal);
			Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
		}
		else if (subTimeTickRemaining > UE_KINDA_SMALL_NUMBER && !bJustTeleported)
		{
			const FVector NewVelocity = (Delta / subTimeTickRemaining);
			Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
		}

		float GravityFactor;
		if (subTimeTickRemaining > UE_KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
		{
			// The compute slide vector along surfaces doesn't calculate gravity, so we'll have to recalculate that here
			float MaxSpeed = GetMaxSpeed();
			GravityFactor = UKismetMathLibrary::MapRangeClamped(FVector2D(Velocity).Size(), MaxSpeed, MaxSpeed * 4, 1, .5);
			FVector VelocityWithGravity = NewFallVelocity(Velocity, Gravity* GravityFactor, timeTick);
			Velocity.Z = VelocityWithGravity.Z;

			// TODO: Factor gravity based on the floor angle to allow the player to control while wall sliding
			
			// Move in deflected direction.
			SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);
			
			if (Hit.bBlockingHit)
			{
				// hit second wall
				LastMoveTimeSlice = subTimeTickRemaining;
				subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);
			
				if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
				{
					remainingTime += subTimeTickRemaining;
					ProcessLanded(Hit, remainingTime, Iterations);
					return;
				}
			
				HandleImpact(Hit, LastMoveTimeSlice, Delta);
			
				// If we've changed physics mode, abort.
				if (!HasValidData() || !IsFalling())
				{
					return;
				}
			
				// Act as if there was no air control on the last move when computing new deflection.
				if (bHasLimitedAirControl && Hit.Normal.Z > CharacterMovementConstants::VERTICAL_SLOPE_NORMAL_Z)
				{
					const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
					Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
				}
			
				FVector PreTwoWallDelta = Delta;
				TwoWallAdjust(Delta, Hit, OldHitNormal);
			
				// Limit air control, but allow a slide along the second wall.
				if (bHasLimitedAirControl)
				{
					const bool bCheckLandingSpot = false; // we already checked above.
					const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;
			
					// Only allow if not back in to first wall
					if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
					{
						Delta += (AirControlDeltaV * subTimeTickRemaining);
					}
				}
			
				// Compute velocity after deflection (only gravity component for RootMotion)
				if (subTimeTickRemaining > UE_KINDA_SMALL_NUMBER && !bJustTeleported)
				{
					const FVector NewVelocity = (Delta / subTimeTickRemaining);
					Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
				}
			
				// bDitch=true means that pawn is straddling two slopes, neither of which it can stand on
				bool bDitch = ( (OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= UE_KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f) );
				SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);
				if ( Hit.Time == 0.f )
				{
					// if we are stuck then try to side step
					FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
					if ( SideDelta.IsNearlyZero() )
					{
						SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
					}
					SafeMoveUpdatedComponent( SideDelta, PawnRotation, true, Hit);
				}
					
				if ( bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f  )
				{
					remainingTime = 0.f;
					ProcessLanded(Hit, remainingTime, Iterations);
					return;
				}
				else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
				{
					// We might be in a virtual 'ditch' within our perch radius. This is rare.
					const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
					const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
					const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
					if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
					{
						Velocity.X += 0.25f * GetMaxSpeed() * (RandomStream.FRand() - 0.5f);
						Velocity.Y += 0.25f * GetMaxSpeed() * (RandomStream.FRand() - 0.5f);
						Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
						Delta = Velocity * timeTick;
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
					}
				}
			}
		}
	}

	if (Velocity.SizeSquared2D() <= UE_KINDA_SMALL_NUMBER * 10.f)
	{
		Velocity.X = 0.f;
		Velocity.Y = 0.f;
	}
}


bool UAdvancedMovementComponent::WallJumpValid(float deltaTime, const FVector& OldLocation, const FVector& InputVector, FHitResult& JumpHit, const FHitResult& Hit)
{
	// If wall jumping is enabled
	if (!bUseWallJumping) return false;
	
	// If the player isn't trying to wall jump, just return
	if (!WallJumpPressed) return false;

	// Don't allow any of those extra wall jumps
	if (WallJumpLimit != 0 && CurrentWallJumpCount >= WallJumpLimit) return false;

	// Prevent wall jumping immediately and wasting another jump (We use distance instead of height to account for ledges and all scenarios)
	if ((UpdatedComponent->GetComponentLocation() - PreviousGroundLocation).Size() < WallJumpHeightFromGroundThreshold ) return false;
	
	// If the movement component has captured a valid wall already, just use that
	if (Hit.bBlockingHit && !Hit.ImpactNormal.Equals(PrevWallJumpNormal, 0.1))
	{
		JumpHit = Hit;
		return true;
	}
	
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector InputDir = Start + InputVector * WallJumpValidDistance;
	const FVector Front = Start + UpdatedComponent->GetForwardVector() * WallJumpValidDistance;
	
	TArray<AActor*> CharacterActors;
	CharacterOwner->GetAllChildActors(CharacterActors); // TODO: Investigate if this includes player spawned actors
	CharacterActors.AddUnique(CharacterOwner);

	// Check whether there's a wall in front or behind the player
	UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		Start,
		InputDir,
		MovementChannel,
		false,
		CharacterActors,
		bDebugWallJumpTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		JumpHit,
		true,
		FColor::Emerald,
		FColor::Blue,
		TraceDuration
	);

	if (!JumpHit.bBlockingHit)
	{
		UKismetSystemLibrary::LineTraceSingle(
			GetWorld(),
			Start,
			Front,
			MovementChannel,
			false,
			CharacterActors,
			bDebugWallJumpTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
			JumpHit,
			true,
			FColor::Cyan,
			FColor::Blue,
			TraceDuration
		);

		if (!JumpHit.bBlockingHit)
		{
			return false;
		}
	}
	
	if (JumpHit.ImpactNormal.Equals(PrevWallJumpNormal, 0.1))
	{
		return false;
	}

	if (bDebugWallJumpTrace)
	{
		DrawDebugBox(
			GetWorld(),
			JumpHit.ImpactPoint,
			FVector(5),
			FColor::Emerald,
			false,
			TraceDuration
		);
	}
	
	return true;
}


bool UAdvancedMovementComponent::IsMantleJump()
{
	if (MantleJumpStartTime + MantleJumpDuration < Time)
	{
		return false;
	}

	return true;
}


void UAdvancedMovementComponent::CalculateMantleJumpTrajectory(const FVector2D SpeedBoost)
{
	const FVector AccelDir = Acceleration.GetSafeNormal();
	const FVector OldVelocity = Velocity;

	const FVector AddedVelocity = AccelDir * FVector(SpeedBoost.X, SpeedBoost.X, 0) + FVector(0, 0, SpeedBoost.Y);
	Velocity += AddedVelocity;

	if (bDebugMantleJump)
	{
		UE_LOGFMT(Movement, Warning, "{0}::MantleJump () ->  ({1})({2}) Initial/Vel: ({3})({4}), AdditionalVelocity: ({5})",
			CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
			*GetMovementDirection(PlayerInput),
			Velocity.Size(),
			*OldVelocity.ToString(),
			*Velocity.ToString(),
			*AddedVelocity.ToString()
		);
	}
}


void UAdvancedMovementComponent::CalculateWallJumpTrajectory(float DeltaTime, int32 Iterations, const FHitResult& Wall, float Speed, FVector2D Boost, FString PrevState)
{
	if (!UpdatedComponent) return;
	
	// Most velocity calculation happens when the character is already sliding alongside the wall, and we need a calculation that's net safe for handling every scenario safely
	CurrentWallJumpCount++;
	FVector WallLocation;
	FVector PrevLocation;
	FVector WallJump;
	if (IsCustomMovementMode(MOVE_Custom_WallClimbing))
	{
		WallJump = Wall.ImpactNormal;
		WallJump.Z = 1;
	}
	else if (IsCustomMovementMode(MOVE_Custom_WallRunning))
	{
		const float PlayerForwardsDirection = UpdatedComponent->GetForwardVector().Dot(WallRunWall->GetRightVector());
		const FVector WallJumpDirection = PlayerForwardsDirection < 0 ? WallRunWall->GetRightVector() * -1 : WallRunWall->GetRightVector();
		WallJump = FVector(
			FMath::Clamp((WallJumpDirection.X + WallRunNormal.X) / 2, -1, 1),
			FMath::Clamp((WallJumpDirection.Y + WallRunNormal.Y) / 2, -1, 1),
			1
		);
	}
	else
	{
		// Calculate the trajectory
		WallLocation = FVector(Wall.ImpactPoint.X, Wall.ImpactPoint.Y, 0);
		PrevLocation = FVector(PreviousGroundLocation.X, PreviousGroundLocation.Y, 0);
		if (PrevLocation.Equals(FVector(WallLocation.X, WallLocation.Y, 0), WallJumpSpacing)) PrevLocation += Wall.Normal * WallJumpSpacing;
		const FVector CharacterTrajectory = (WallLocation - PrevLocation).GetSafeNormal2D();
		
		const float WallAngle = Wall.ImpactNormal.Dot(CharacterTrajectory); // -0.5 or greater is 45^ or less
		WallJump = (CharacterTrajectory - 2 * (CharacterTrajectory | Wall.ImpactNormal) * Wall.ImpactNormal).GetSafeNormal();
		
		// Check if the previous ground location is behind the wall location using the normal, if so fix the trajectory so it doesn't create the wrong walljumps
		if (WallAngle > 0)
		{
			const FVector LocationAlignedToWall = (Wall.Normal.GetSafeNormal2D() * WallLocation) + ((FVector(1) - Wall.Normal.GetSafeNormal2D()) * PrevLocation);
			const float DistanceFromWall = LocationAlignedToWall.Size();
			const FVector WallJumpTrajectory = (WallLocation - (LocationAlignedToWall + (Wall.ImpactNormal * DistanceFromWall))).GetSafeNormal();
			WallJump = (WallJumpTrajectory - 2 * (WallJumpTrajectory | Wall.ImpactNormal) * Wall.ImpactNormal).GetSafeNormal();

			if (bDebugWallJumpTrace)
			{
				DrawDebugBox(GetWorld(), LocationAlignedToWall, FVector(10), FColor::Red, false, TraceDuration);
				DrawDebugBox(GetWorld(), LocationAlignedToWall + Wall.ImpactNormal * DistanceFromWall, FVector(10), FColor::Orange, false, TraceDuration);
			}
		}
		// Fix wall jumps when sliding alongside a wall (during Air Strafing)
		else if (WallAngle > -0.45)
		{
			WallJump = (Velocity.GetSafeNormal2D() - Wall.ImpactNormal) / 2 + Wall.ImpactNormal;
		}

		// Wall jump Z velocity is factored in during the boost, and not while redirecting the velocity
		WallJump = FVector(WallJump.X, WallJump.Y, 1);
		
		// Smooth out gravity for non positive wall jump values
		if (Velocity.GetSafeNormal().Z < 0)
		{
			WallJump = FVector(WallJump.X, WallJump.Y, 0.5);
		}
	}
	
	
	// Wall jump calculations. Their speeds should be static, and also save the player's current velocity
	const float CurrentSpeed = Velocity.Size2D();
	const FVector RedirectedVelocity = FVector(WallJump.X, WallJump.Y, 0) * FMath::Max(CurrentSpeed, Speed);
	Velocity = RedirectedVelocity + (WallJump * FVector(Boost.X, Boost.X, Boost.Y));
	PrevWallJumpNormal = Wall.ImpactNormal;
	PrevWallJumpLocation = Wall.Location;
	PrevWallJumpTime = Time;
	
	SetMovementMode(MOVE_Falling);
	EnableStrafeSwayPhysics();
	StartNewPhysics(DeltaTime, Iterations);

	
	if (bDebugWallJumpTrajectory)
	{
		FVector DebugWallLocation = WallLocation;
		FVector DebugPrevLocation = PrevLocation;
		if (IsCustomMovementMode(MOVE_Custom_WallClimbing))
		{
			DebugWallLocation = PrevWallClimbLocation;
			DebugPrevLocation = PreviousGroundLocation;
		}
		if (IsCustomMovementMode(MOVE_Custom_WallRunning))
		{
			DebugWallLocation = PrevWallClimbLocation;
			DebugPrevLocation = FVector();
		}
		
		UE_LOGFMT(Movement, Warning, "{0}::WallJump ({1}) ->  ({2})({3}) Initial/Vel: ({4})({5}), Boost: ({6}), CapturedSpeed: ({7}), Wall/Prev Location: ({8})({9})",
			CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
			*PrevState,
			*GetMovementDirection(PlayerInput),
			*FString::SanitizeFloat(Speed),
			*Velocity.ToString(),
			*RedirectedVelocity.ToString(),
			*Boost.ToString(),
			*FString::SanitizeFloat(CurrentSpeed),
			*DebugWallLocation.ToString(),
			*DebugPrevLocation.ToString()
		);
	}
	
	if (bDebugWallJumpTrace)
	{
		DrawDebugBox(GetWorld(), PreviousGroundLocation, FVector(10), FColor::Blue, false, TraceDuration);
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + (WallJump * 250), FColor::Red, false, TraceDuration);
	}
}


void UAdvancedMovementComponent::ResetWallJumpInformation(const EMovementMode PrevMode, uint8 PrevCustomMode)
{
	if (IsMovingOnGround())
	{
		CurrentWallJumpCount = 0;
	}

	if (PrevMode == MOVE_Falling)
	{
		PrevWallJumpNormal = FVector::ZeroVector;
		PrevWallJumpLocation = FVector::ZeroVector;
		PrevWallJumpTime = 0;
	}
	else if (PrevMode == MOVE_Walking)
	{
		PreviousGroundLocation = UpdatedComponent->GetComponentLocation();
	}
}
#pragma endregion 




//------------------------------------------------------------------------------//
// Ground Movement Logic														//
//------------------------------------------------------------------------------//
#pragma region Ground Movement
void UAdvancedMovementComponent::BaseWalkingFunctionality(float deltaTime, int32& Iterations, const float timeTick)
{
	// Ensure velocity is horizontal.
	MaintainHorizontalGroundVelocity();
	Acceleration.Z = 0.f;

	// Apply acceleration
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());
	}
}


void UAdvancedMovementComponent::BaseSlidingFunctionality(float deltaTime, int32& Iterations, const float timeTick)
{
	// Velocity calculation, and Ensure velocity is horizontal.
	MaintainHorizontalGroundVelocity();
	
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		float SlopeAngle = CurrentFloor.HitResult.ImpactNormal.Dot(UpdatedComponent->GetForwardVector()) * SlideAngleFrictionMultiplier;
		const float SlidingFrictionCalc = FMath::Clamp(SlidingFriction - SlopeAngle, 0, SlidingFriction * 3);
		CalcVelocity(timeTick, SlidingFrictionCalc, false, GetMaxBrakingDeceleration());
	}
}


void UAdvancedMovementComponent::GroundMovementPhysics(float deltaTime, int32 Iterations)
{
	// Initial setup
	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while (
		(remainingTime >= MIN_TICK_TIME) &&
		(Iterations < MaxSimulationIterations) &&
		CharacterOwner &&
		(CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() ||
			(CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		// Setup physics sub steps
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// UE_LOGFMT(Movement, Log, "{0} Physics(Walking): {2}", CharacterOwner->HasAuthority() ? "Server" : "Client", Time);
		
		// Save current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		RestorePreAdditiveRootMotionVelocity();
		const FVector OldVelocity = Velocity;

		//// This is the actual velocity and acceleration calculation ////
		if (MovementMode == MOVE_Walking) BaseWalkingFunctionality(deltaTime, Iterations, timeTick);
		else BaseSlidingFunctionality(deltaTime, Iterations, timeTick);

		
		ApplyRootMotionToVelocity(timeTick);
		if (IsFalling())
		{
			// Root motion could have put us into Falling.
			// No movement has taken place this movement tick so we pass on full time/past iteration count
			StartNewPhysics(remainingTime + timeTick, Iterations - 1);
			return;
		}

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}

				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}


		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}


		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on - @5.3
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				DebugGroundMovement(FString("This floor is valid to walk on"), FColor::Blue);
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					HandleWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
					}

					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				DebugGroundMovement(FString("This floor check failed"), FColor::Yellow);

				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}

			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				DebugGroundMovement(FString("The character started falling"), FColor::Red);
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}


		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				MaintainHorizontalGroundVelocity();
			}
		}

		// If we didn't move at all this iteration then stop (since other iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}
}
#pragma endregion 




//------------------------------------------------------------------------------//
// Slide Logic																	//
//------------------------------------------------------------------------------//
#pragma region Sliding
bool UAdvancedMovementComponent::CanSlide() const
{
	if (!bUseSliding) return false;
	if (!CharacterOwner->GetCapsuleComponent() || IsFalling()) return false;
	if (PrevSlideTime + SlideDelay > Time) return false;
	if (!IsCrouching()) return false;
	return true;
}


void UAdvancedMovementComponent::EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	Velocity += Velocity.GetSafeNormal2D() * SlideEnterBoost;
	SlideStartTime = Time;
}


void UAdvancedMovementComponent::ExitSlide()
{
	PrevSlideTime = Time;
}
#pragma endregion




//------------------------------------------------------------------------------//
// WallClimb Logic																//
//------------------------------------------------------------------------------//
#pragma region Wall Climbing
bool UAdvancedMovementComponent::CanWallClimb() const
{
	if (!bUseWallClimbing) return false;
	if (WallClimbInterval + PrevWallClimbTime > Time) return false;
	if (PlayerInput.IsNearlyZero(0.1)) return false;
	return true;
}

bool UAdvancedMovementComponent::TryingToClimbWall(FVector WallNormal) const
{
	const float PlayerAngle = 180 - UKismetMathLibrary::DegAcos(WallNormal.Dot(UpdatedComponent->GetForwardVector()));
	if (bOrientRotationToMovement) return WallClimbAcceptableAngle * 0.64 > PlayerAngle;
	else return WallClimbAcceptableAngle > PlayerAngle;
}


void UAdvancedMovementComponent::EnterWallClimb(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	WallClimbStartTime = Time;
}


void UAdvancedMovementComponent::ExitWallClimb()
{
	PrevWallClimbTime = Time;
	
	// This prevents the wall jump from being redirected if they wall jump out of a wall climb or a mantle 
	PreviousGroundLocation = UpdatedComponent->GetComponentLocation() + PrevWallClimbNormal * 100;
}


void UAdvancedMovementComponent::ResetWallClimbInformation(const EMovementMode PrevMode, uint8 PrevCustomMode)
{
	if (PrevMode == MOVE_Walking)
	{
		WallClimbStartTime = 0;
		PrevWallClimbTime = 0;
		PrevWallClimbLocation = FVector();
	}
}
#pragma endregion 



//------------------------------------------------------------------------------//
// Mantle and Ledge Climb Logic													//
//------------------------------------------------------------------------------//
#pragma region Mantling
bool UAdvancedMovementComponent::CheckIfSafeToMantleLedge()
{
	if (!bUseMantling) return false;
	if (!UpdatedComponent || !GetWorld() || !CharacterOwner || !CharacterOwner->GetCapsuleComponent()) return false;

	float CharacterRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CharacterHalfHeightNoHemisphere = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight_WithoutHemisphere();
	float CharacterHemisphereHeight = CharacterHalfHeight - CharacterHalfHeightNoHemisphere;
	
	TArray<AActor*> CharacterActors; 
	CharacterOwner->GetAllChildActors(CharacterActors); // TODO: Investigate if this includes player spawned actors
	CharacterActors.AddUnique(CharacterOwner);
	
	// Search for a wall
	FVector InitialTraceStart = UpdatedComponent->GetComponentLocation() - FVector(0, 0, MantleTraceHeightOffset);
	FVector InitialTraceEnd = InitialTraceStart + UpdatedComponent->GetForwardVector() * MantleTraceDistance;
	// if (MantleWallNormal.IsNearlyZero()) InitialTraceEnd = InitialTraceStart + UpdatedComponent->GetForwardVector() * MantleTraceDistance;
	// else InitialTraceEnd = InitialTraceStart + (-MantleWallNormal * MantleTraceDistance);
	
	FHitResult Wall;
	UKismetSystemLibrary::LineTraceSingleForObjects(
		GetWorld(), InitialTraceStart, InitialTraceEnd,
		MantleObjects,false, CharacterActors,
		bDebugMantleAndClimbTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		Wall,true, FColor::Emerald, FColor::Red, TraceDuration
	);
	if (!Wall.IsValidBlockingHit())
	{
		return false;
	}

	// Search for a valid ledge
	const FVector LedgeSurfaceEnd = Wall.Location + (-Wall.Normal * MantleSurfaceTraceFromLedgeOffset);
	const FVector LedgeSurfaceStart = LedgeSurfaceEnd + FVector(0, 0, MantleSecondTraceDistance);
	FHitResult Ledge;
	UKismetSystemLibrary::LineTraceSingleForObjects(
		GetWorld(), LedgeSurfaceStart, LedgeSurfaceEnd,
		MantleObjects, false, CharacterActors,
		bDebugMantleAndClimbTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		Ledge, true, FColor::Emerald, FColor::Red, TraceDuration
	);
	if (!Ledge.IsValidBlockingHit() || LedgeSurfaceStart.Equals(Ledge.ImpactPoint, 1))
	{
		return false;
	}
	
	// Check if the player is able to climb to it, and if they have to crouch
	FVector FrontOfLedgeMidpoint = Ledge.Location - (-Wall.Normal * (MantleSurfaceTraceFromLedgeOffset + CharacterRadius + ClimbLocationSpaceOffset)) + FVector(0, 0, MantleTraceHeightOffset + CharacterHemisphereHeight);
	FVector ClimbStart = FrontOfLedgeMidpoint + (FVector(0, 0, CharacterHalfHeightNoHemisphere * 2)) - FVector(0, 0, MantleTraceHeightOffset);
	FVector ClimbEnd = FVector(ClimbStart.X, ClimbStart.Y, UpdatedComponent->GetComponentLocation().Z + MantleTraceHeightOffset - CharacterHalfHeightNoHemisphere);
	FHitResult ClimbSpace;
	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(), ClimbStart, ClimbEnd, CharacterRadius,
		MantleObjects, false, CharacterActors,
		bDebugMantleAndClimbTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		ClimbSpace, true, FColor::Turquoise, FColor::Red, TraceDuration
	);
	if (ClimbSpace.IsValidBlockingHit())
	{
		return false;
	}
	
	// Check if they're able to walk on the ledge
	FVector LedgeWalkStart = Ledge.Location + FVector(0, 0, MantleTraceHeightOffset + CharacterHemisphereHeight);
	FVector LedgeWalkEnd = LedgeWalkStart + FVector(0, 0, CharacterHalfHeightNoHemisphere * 2) - FVector(0, 0, MantleTraceHeightOffset);
	FHitResult LedgeRoom;
	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(), LedgeWalkStart, LedgeWalkEnd, CharacterRadius,
		MantleObjects, false, CharacterActors,
		bDebugMantleAndClimbTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		LedgeRoom, true, FColor::Emerald, FColor::Red,TraceDuration
	);
	if (LedgeRoom.IsValidBlockingHit())
	{
		return false;
	}

	// Calculate the mantle location
	LedgeClimbLocation = Ledge.Location;
	LedgeClimbNormal = Wall.Normal;
	MantleLedgeLocation = LedgeClimbLocation
		- -Wall.Normal * (MantleSurfaceTraceFromLedgeOffset + CharacterRadius + ClimbLocationSpaceOffset + MantleLocationSpaceOffset)
		+ FVector(0, 0, MantleLedgeLocationOffset);
	if (bDebugMantleAndClimbTrace)
	{
		DrawDebugCapsule(
			GetWorld(),
			MantleLedgeLocation,
			CharacterHalfHeight,
			CharacterRadius,
			UpdatedComponent->GetComponentQuat(),
			FColor::Cyan,
			false,
			TraceDuration
		);
	}
	
	// Determine the mantle type
	// If in walking or in air, just use a normal ledge climb
	// If falling quickly through the air, use a slow ledge climb type
	// If walking and it's an object close to the ground, use a quick ledge climb type
	if (UpdatedComponent->GetComponentLocation().Z > MantleLedgeLocation.Z) ClimbType = EClimbType::Fast;
	else ClimbType = EClimbType::Normal;
	
	return true;
}


FVector UAdvancedMovementComponent::MantleAndClimbInterp(const float DeltaTime, const FVector StartLocation, const FVector TargetLocation, const FVector CurrentLocation, const float Speed, UCurveFloat* SpeedAdjustments) const
{
	// Find the interp direction
	const FVector MovementVector = TargetLocation - CurrentLocation;
	const FVector MovementDirection = MovementVector.GetSafeNormal();

	// Use speed adjustments to create your own ease in transitions
	const float CurrentPercent = MovementVector.Size() / (TargetLocation - StartLocation).Size(); // 0-1
	const float InterpSpeedAdjustments = SpeedAdjustments ? FMath::Clamp(SpeedAdjustments->GetFloatValue(CurrentPercent * 10), 0.1, 10) : 1;

	const FVector Adjusted = MovementDirection * Speed * InterpSpeedAdjustments * DeltaTime;
	if (Adjusted.Size() > MovementVector.Size()) return MovementVector;
	return Adjusted;
}


void UAdvancedMovementComponent::EnterMantle(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	MantleStartTime = Time;
	MantleStartLocation = UpdatedComponent->GetComponentLocation();
}


void UAdvancedMovementComponent::ExitMantle()
{
	MantleStartTime = 0;
	MantleStartLocation = FVector();
	MantleLedgeLocation = FVector();
	
	// This prevents the wall jump from being redirected if they wall jump out of a wall climb or a mantle 
	PreviousGroundLocation = UpdatedComponent->GetComponentLocation() + PrevWallClimbNormal * 100;
}


void UAdvancedMovementComponent::EnterLedgeClimb(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	LedgeClimbStartTime = Time;
	LedgeClimbStartLocation = UpdatedComponent->GetComponentLocation();

	// Adjust the collision during ledge climbs
	if (CharacterOwner->GetCapsuleComponent())
	{
		CapturedCollisionResponsesOutsideOfLedgeClimbing = CharacterOwner->GetCapsuleComponent()->GetCollisionResponseToChannels();
		CharacterOwner->GetCapsuleComponent()->SetCollisionResponseToChannels(CollisionResponsesDuringLedgeClimbing);
	}

	// Find the speed and easing adjustments from the list of ledge climb variations
	if (LedgeClimbVariations.Contains(ClimbType))
	{
		CurrentClimbSpeed = LedgeClimbVariations[ClimbType].InterpSpeed;
		CurrentClimbSpeedAdjustments = LedgeClimbVariations[ClimbType].SpeedAdjustments;
	}
}


void UAdvancedMovementComponent::ExitLedgeClimb()
{
	LedgeClimbStartTime = 0;
	LedgeClimbStartLocation = FVector();
	LedgeClimbLocation = FVector();
	LedgeClimbNormal = FVector();
	
	// Revert the collisions for traditional movement
	if (CharacterOwner->GetCapsuleComponent())
	{
		CharacterOwner->GetCapsuleComponent()->SetCollisionResponseToChannels(CapturedCollisionResponsesOutsideOfLedgeClimbing);
	}
	
	// Default speed for safety precautions
	CurrentClimbSpeed = 340;
	CurrentClimbSpeedAdjustments = nullptr;
	ClimbType = EClimbType::None;
}
#pragma endregion 




//------------------------------------------------------------------------------//
// WallRun Logic																//
//------------------------------------------------------------------------------//
#pragma region Wall Running
bool UAdvancedMovementComponent::CanWallRun(const FHitResult& Wall) const
{
	if (!bUseWallRunning) return false;
	if (PlayerInput.Y < 0.1 && PlayerInput.Y > -0.1) return false;

	// If it's the same wall, it needs to be at a lower height
	if (WallRunLocation.Z - Wall.Location.Z < WallRunHeightThreshold)
	{
		if (WallRunWall && WallRunWall == Wall.GetComponent())
		{
			return false;
		}
	}
	
	return true;
}

void UAdvancedMovementComponent::EnterWallRun(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	WallRunCurrentSpeed = WallRunSpeed > Velocity.Size2D() ? WallRunSpeed : Velocity.Size2D();
	WallRunInputDirection = PlayerInput.Y;
	WallRunStartTime = Time;
}

void UAdvancedMovementComponent::ExitWallRun()
{
}

void UAdvancedMovementComponent::ResetWallRunInformation(EMovementMode PrevMode, uint8 PrevCustomMode)
{
	if (PrevMode == MOVE_Walking)
	{
		WallRunWall = nullptr;
		WallRunStartTime = 0;
		WallRunLocation = FVector();
		WallRunNormal = FVector();
	}
}
#pragma endregion 




//------------------------------------------------------------------------------//
// Multiplayer input replication logic											//
//------------------------------------------------------------------------------//
#pragma region Multiplayer Input Replication
void UAdvancedMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	// Save additional movement information 
	FMCharacterNetworkMoveData* MoveData = static_cast<FMCharacterNetworkMoveData*>(GetCurrentNetworkMoveData());
	if (MoveData)
	{
		PlayerInput = FVector2D(MoveData->MoveData_InputAndTime.X, MoveData->MoveData_InputAndTime.Y);
		Time = MoveData->MoveData_InputAndTime.Z;
	}
	
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}


void UAdvancedMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	WallJumpPressed = (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
	AimPressed = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
	Mantling = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	SprintPressed = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}


//------------------------------------------------------------------------------//
// Bhop FMCharacterNetworkMoveData												//
//------------------------------------------------------------------------------//
void UAdvancedMovementComponent::FMCharacterNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	const FMSavedMove& SavedMove = static_cast<const FMSavedMove&>(ClientMove);
	MoveData_InputAndTime = FVector(SavedMove.PlayerInput.X, SavedMove.PlayerInput.Y, SavedMove.Time);
}


uint8 UAdvancedMovementComponent::FMSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (SavedRequestToStartWallJumping) Result |= FLAG_Custom_3;
	if (SavedRequestToStartAiming) Result |= FLAG_Custom_2;
	if (SavedRequestToStartMantling) Result |= FLAG_Custom_1;
	if (SavedRequestToStartSprinting) Result |= FLAG_Custom_0;
	return Result;
}


void UAdvancedMovementComponent::FMSavedMove::Clear()
{
	Super::Clear();
	SavedRequestToStartWallJumping = 0;
	SavedRequestToStartAiming = 0;
	SavedRequestToStartMantling = 0;
	SavedRequestToStartSprinting = 0;
	PlayerInput = FVector2D::ZeroVector;
}


bool UAdvancedMovementComponent::FMSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	// Set which moves can be combined together. This will depend on the bit flags that are used.
	const FMSavedMove* NewSavedMove = static_cast<FMSavedMove*>(NewMove.Get());
	if (!PlayerInput.Equals(NewSavedMove->PlayerInput, 0.1)) return false;
	if (SavedRequestToStartWallJumping != NewSavedMove->SavedRequestToStartWallJumping) return false;
	if (SavedRequestToStartAiming != NewSavedMove->SavedRequestToStartAiming) return false;
	if (SavedRequestToStartMantling != NewSavedMove->SavedRequestToStartMantling) return false;
	if (SavedRequestToStartSprinting != NewSavedMove->SavedRequestToStartSprinting) return false;
	// TODO: Investigate Combining moves with acceptable times
	
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}


bool UAdvancedMovementComponent::FMCharacterNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	const bool bIsSaving = Ar.IsSaving();
	bool bLocalSuccess = true;

	// Save move values
	MoveData_InputAndTime.NetSerialize(Ar, PackageMap, bLocalSuccess); // TODO: Learn how to serialize things
	
	return !Ar.IsError();
}




//------------------------------------------------------------------------------//
// Bhop FMCharacterNetworkMoveDataContainer										//
//------------------------------------------------------------------------------//
UAdvancedMovementComponent::FMCharacterNetworkMoveDataContainer::FMCharacterNetworkMoveDataContainer()
{
	NewMoveData = &CustomDefaultMoveData[0];
	PendingMoveData = &CustomDefaultMoveData[1];
	OldMoveData = &CustomDefaultMoveData[2];
}


void UAdvancedMovementComponent::FMSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
	
	// Set our saved cmc values to the current(safe) values of the cmc
	UAdvancedMovementComponent* CharacterMovement = Cast<UAdvancedMovementComponent>(Character->GetCharacterMovement());
	PlayerInput = CharacterMovement->PlayerInput;
	Time = CharacterMovement->Time;
	SavedRequestToStartWallJumping = CharacterMovement->WallJumpPressed;
	SavedRequestToStartAiming = CharacterMovement->AimPressed;
	SavedRequestToStartMantling = CharacterMovement->Mantling;
	SavedRequestToStartSprinting = CharacterMovement->SprintPressed;
}


void UAdvancedMovementComponent::FMSavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);
	UAdvancedMovementComponent* CharacterMovement = Cast<UAdvancedMovementComponent>(Character->GetCharacterMovement());
	CharacterMovement->PlayerInput = PlayerInput; 
	CharacterMovement->WallJumpPressed = SavedRequestToStartWallJumping;
	CharacterMovement->AimPressed = SavedRequestToStartAiming;
	CharacterMovement->Mantling = SavedRequestToStartMantling;
	CharacterMovement->SprintPressed = SavedRequestToStartSprinting;
	CharacterMovement->Time = Time;
}




//------------------------------------------------------------------------------//
// Bhop FNetworkPredicitonData_Client Implementation							//
//------------------------------------------------------------------------------//
UAdvancedMovementComponent::FMNetworkPredictionData_Client::FMNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {}
FSavedMovePtr UAdvancedMovementComponent::FMNetworkPredictionData_Client::AllocateNewMove()
{
	return FSavedMovePtr(new FMSavedMove());
}


FNetworkPredictionData_Client* UAdvancedMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);
	if (!ClientPredictionData)
	{
		UAdvancedMovementComponent* MutableThis = const_cast<UAdvancedMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FMNetworkPredictionData_Client(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}
#pragma endregion 




//------------------------------------------------------------------------------//
// Input Functions																//
//------------------------------------------------------------------------------//
#pragma region Input functions
void UAdvancedMovementComponent::StartMantling() { Mantling = true; }
void UAdvancedMovementComponent::StopMantling() { Mantling = false; }
void UAdvancedMovementComponent::StartSprinting() { SprintPressed = true; }
void UAdvancedMovementComponent::StopSprinting() { SprintPressed = false; }
void UAdvancedMovementComponent::StartAiming() { AimPressed = true; }
void UAdvancedMovementComponent::StopAiming() { AimPressed = false; }
void UAdvancedMovementComponent::UpdatePlayerInput(const FVector2D& InputVector) { PlayerInput = InputVector; }
void UAdvancedMovementComponent::StartWallJump() { WallJumpPressed = true; }
void UAdvancedMovementComponent::StopWallJump() { WallJumpPressed = false; }
void UAdvancedMovementComponent::DisableStrafeSwayPhysics() { AirStrafeSwayPhysics = false; }
void UAdvancedMovementComponent::EnableStrafeSwayPhysics()
{
	StrafeSwayStartTime = Time;
	AirStrafeSwayPhysics = true;
}
void UAdvancedMovementComponent::DisableStrafeLurchPhysics() { AirStrafeLurchPhysics = false; }
void UAdvancedMovementComponent::EnableStrafeLurchPhysics()
{
	StrafeLurchStartTime = Time;
	AirStrafeLurchPhysics = true;
}
#pragma endregion




//------------------------------------------------------------------------------//
// Jump Logic																	//
//------------------------------------------------------------------------------//
#pragma region Jumping
bool UAdvancedMovementComponent::DoJump(bool bReplayingMoves)
{
	if (CharacterOwner && CharacterOwner->CanJump())
	{
		FVector AccelDir = Acceleration.GetSafeNormal();
		FVector OldVelocity = Velocity;
		
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			// Unreal handles movement logic with client information, and we need to find a way to either make this frame dependent or challenging so it isn't easy to accomplish

			// Jump forward boost for jumps while sliding
			if (IsSliding())
			{
				const FVector ForwardVector = Velocity.GetSafeNormal() * SlideJumpBoost;
				Velocity += ForwardVector;
			}
			
			// Mantle Jumping
			if (IsMantleJump())
			{
				CalculateMantleJumpTrajectory(MantleJumpBoost);
				EnableStrafeLurchPhysics();

				// Super glide input (in order for jumps to work while crouching, you need to adjust the CanJump() function to allow jumping while crouching)
				if (bWantsToCrouch) // CanSlide()
				{
						Velocity += AccelDir * FVector(SuperGlideBoost.X, SuperGlideBoost.X, 0) + FVector(0, 0, SuperGlideBoost.Y);
				}
			}
			else
			{
				Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z + 1.f, JumpZVelocity);
			}
			
			if (bDebugWallJump)
			{
				//UE_LOG(Movement, Warning, TEXT("%s() %s: Go, do a crime. new velocity: %s"), *FString(__FUNCTION__), *GetNameSafe(CharacterOwner), *Velocity.ToCompactString());
				const FString JumpVariation = IsMantleJump() ? FString("MantleJump") : IsSliding() ? FString("SlideJump") : FString("");
				UE_LOGFMT(Movement, Warning, "{0}::Jumped ({1}) ->  ({2})({3}) Initial/Vel: ({4})({5}), JumpBoost: ({6}), CapturedSpeed: ({7})",
					CharacterOwner->HasAuthority() ? *FString("Server") : *FString("Client"),
					*JumpVariation,
					*GetMovementDirection(PlayerInput),
					*FString::SanitizeFloat(JumpZVelocity),
					*OldVelocity.ToString(),
					*Velocity.ToString(),
					IsMantleJump() ? *WallJumpBoost.ToString() :
					IsSliding() ? *FVector2D(SlideJumpBoost, 0).ToString() :
					*FVector2D(JumpZVelocity, 0).ToString(),
					*FString::SanitizeFloat(Velocity.Size2D())
				);
			}
			
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}

	return false;
}


bool UAdvancedMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling()); // Falling included for double-jump and non-zero jump hold time, but validated by character.
	//!bWantsToCrouch &&
}
#pragma endregion 




//------------------------------------------------------------------------------//
// Crouch Logic																	//
//------------------------------------------------------------------------------//
#pragma region Crouching
void UAdvancedMovementComponent::Crouch(bool bClientSimulation)
{
	// These are just overridden to add client side predicted replicated tags in safe to add areas
	if (!HasValidData())
	{
		return;
	}

	if (!bClientSimulation && !CanCrouchInCurrentState())
	{
		return;
	}

	// See if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == GetCrouchedHalfHeight())
	{
		if (!bClientSimulation)
		{
			CharacterOwner->bIsCrouched = true;
		}
		CharacterOwner->OnStartCrouch( 0.f, 0.f );
		HandleCrouchLogic();
		return;
	}

	if (bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// restore collision size before crouching
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
		bShrinkProxyCapsule = true;
	}

	// Change collision size to crouching dimensions
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	// Height is not allowed to be smaller than radius.
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, GetCrouchedHalfHeight());
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedCrouchedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedCrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if( !bClientSimulation )
	{
		// Crouching to a larger height? (this is rare)
		if (ClampedCrouchedHalfHeight > OldUnscaledHalfHeight)
		{
			FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
			FCollisionResponseParams ResponseParam;
			InitCollisionParams(CapsuleParams, ResponseParam);
			const bool bEnCrouched = GetWorld()->OverlapBlockingTestByChannel(UpdatedComponent->GetComponentLocation() - FVector(0.f,0.f,ScaledHalfHeightAdjust), FQuat::Identity,
				UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

			// If encroached, cancel
			if( bEnCrouched )
			{
				CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, OldUnscaledHalfHeight);
				return;
			}
		}

		if (bCrouchMaintainsBaseLocation)
		{
			// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
			UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
		}

		CharacterOwner->bIsCrouched = true;
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	const float MeshAdjust = ScaledHalfHeightAdjust;
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ClampedCrouchedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );
	HandleCrouchLogic();

	// Don't smooth this change in mesh position
	if ((bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) || (IsNetMode(NM_ListenServer) && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy))
	{
		FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
		if (ClientData)
		{
			ClientData->MeshTranslationOffset -= FVector(0.f, 0.f, MeshAdjust);
			ClientData->OriginalMeshTranslationOffset = ClientData->MeshTranslationOffset;
		}
	}
}


void UAdvancedMovementComponent::UnCrouch(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	// See if collision is already at desired size.
	if( CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() )
	{
		if (!bClientSimulation)
		{
			CharacterOwner->bIsCrouched = false;
		}
		CharacterOwner->OnEndCrouch( 0.f, 0.f );
		HandleCrouchLogic();
		return;
	}

	const float CurrentCrouchedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

	// Grow to uncrouched size.
	check(CharacterOwner->GetCapsuleComponent());

	if( !bClientSimulation )
	{
		// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		const UWorld* MyWorld = GetWorld();
		const float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
		FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);

		// Compensate for the difference between current capsule size and standing size
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust); // Shrink by negative amount, so actually grow it.
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		bool bEncroached = true;

		if (!bCrouchMaintainsBaseLocation)
		{
			// Expand in place
			bEncroached = MyWorld->OverlapBlockingTestByChannel(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
		
			if (bEncroached)
			{
				// Try adjusting capsule position to see if we can avoid encroachment.
				if (ScaledHalfHeightAdjust > 0.f)
				{
					// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
					float PawnRadius, PawnHalfHeight;
					CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
					const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
					const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
					const FVector Down = FVector(0.f, 0.f, -TraceDist);

					FHitResult Hit(1.f);
					const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
					const bool bBlockingHit = MyWorld->SweepSingleByChannel(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
					if (Hit.bStartPenetrating)
					{
						bEncroached = true;
					}
					else
					{
						// Compute where the base of the sweep ended up, and see if we can stand there
						const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
						const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + StandingCapsuleShape.Capsule.HalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
						bEncroached = MyWorld->OverlapBlockingTestByChannel(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
						if (!bEncroached)
						{
							// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
							UpdatedComponent->MoveComponent(NewLoc - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
						}
					}
				}
			}
		}
		else
		{
			// Expand while keeping base location the same.
			FVector StandingLocation = PawnLocation + FVector(0.f, 0.f, StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight);
			bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				if (IsMovingOnGround())
				{
					// Something might be just barely overhead, try moving down closer to the floor to avoid it.
					const float MinFloorDist = UE_KINDA_SMALL_NUMBER * 10.f;
					if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
					{
						StandingLocation.Z -= CurrentFloor.FloorDist - MinFloorDist;
						bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
					}
				}				
			}

			if (!bEncroached)
			{
				// Commit the change in location.
				UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
				bForceNextFloorCheck = true;
			}
		}

		// If still encroached then abort.
		if (bEncroached)
		{
			return;
		}

		CharacterOwner->bIsCrouched = false;
	}	
	else
	{
		bShrinkProxyCapsule = true;
	}

	// Now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);

	const float MeshAdjust = ScaledHalfHeightAdjust;
	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );
	HandleCrouchLogic();

	// Don't smooth this change in mesh position
	if ((bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) || (IsNetMode(NM_ListenServer) && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy))
	{
		FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
		if (ClientData)
		{
			ClientData->MeshTranslationOffset += FVector(0.f, 0.f, MeshAdjust);
			ClientData->OriginalMeshTranslationOffset = ClientData->MeshTranslationOffset;
		}
	}
}


void UAdvancedMovementComponent::HandleCrouchLogic()
{
	// BaseAbilitySystem = BaseAbilitySystem ? BaseAbilitySystem : GetAbilitySystem();
	// if (!BaseAbilitySystem) return;
	//
	// if (IsCrouching())
	// {
	// 	if (!BaseAbilitySystem->HasMatchingGameplayTag(CrouchTag))
	// 	{
	// 		BaseAbilitySystem->AddLooseGameplayTag(CrouchTag);
	// 		BaseAbilitySystem->AddReplicatedLooseGameplayTag(CrouchTag);
	// 	}
	// }
	// else
	// {
	// 	if (BaseAbilitySystem->HasMatchingGameplayTag(CrouchTag))
	// 	{
	// 		BaseAbilitySystem->RemoveLooseGameplayTag(CrouchTag);
	// 		BaseAbilitySystem->RemoveReplicatedLooseGameplayTag(CrouchTag);
	// 	}
	// }
}
#pragma endregion 


//------------------------------------------------------------------------------//
// Get And Set functions														//
//------------------------------------------------------------------------------//
#pragma region Get and set functions
float UAdvancedMovementComponent::GetMaxSpeed() const
{
	// Player input based movement logic
	if (IsMovingOnGround())
	{
		if (SprintPressed) return MaxWalkSpeed * SprintSpeedMultiplier;
		if (AimPressed) return MaxWalkSpeed * AimSpeedMultiplier;
		// if (Character->bWalking) return MaxWalkSpeed * WalkSpeedMultiplier;
		if (IsSliding()) return SlideSpeedLimit; // TODO: Investigate crouching logic and it's behavior during air movement, and perhaps refactor this into the normal walking movement logic with physics adjustments similar to air strafe lurches
		if (IsWallRunning()) return WallRunSpeed;
		if (IsCrouching())
		{
			if (SprintPressed) return MaxWalkSpeedCrouched * CrouchSprintSpeedMultiplier;
			else return MaxWalkSpeedCrouched;
		}
	}
	if (IsFalling())
	{
		// air strafing movement technically doesn't have a limit. This is for handling third person speeds that inhibit other logic
	}
	
	return Super::GetMaxSpeed();
}


float UAdvancedMovementComponent::GetMaxAcceleration() const
{
	if (IsFalling()) return StrafingMaxAcceleration;
	return Super::GetMaxAcceleration();
}


float UAdvancedMovementComponent::GetMaxBrakingDeceleration() const
{
	if (MOVE_Walking == MovementMode) return BrakingDecelerationWalking;
	if (MOVE_NavWalking == MovementMode) return BrakingDecelerationWalking;
	if (MOVE_Swimming == MovementMode) return BrakingDecelerationSwimming;
	if (MOVE_Falling == MovementMode) return BrakingDecelerationFalling;
	if (MOVE_Flying == MovementMode) return BrakingDecelerationFlying;
	if (MOVE_Custom == MovementMode)
	{
		if (MOVE_Custom_WallClimbing == CustomMovementMode) return BrakingDecelerationFalling;
	}
	
	return 0.f;
}


void UAdvancedMovementComponent::UpdateExternalMovementModeInformation(EMovementMode& MovementModeRef, uint8& CustomMovementModeRef)
{
	MovementModeRef = MovementMode;
	CustomMovementModeRef = CustomMovementMode;
}
#pragma endregion 


//------------------------------------------------------------------------------//
// Utility																		//
//------------------------------------------------------------------------------//
#pragma region Utility
FVector2D UAdvancedMovementComponent::GetPlayerInput() const
{
	return PlayerInput;
}


EMovementMode UAdvancedMovementComponent::GetMovementMode() const
{
	return MovementMode;
}


ECustomMovementMode UAdvancedMovementComponent::GetCustomMovementMode() const
{
	return static_cast<ECustomMovementMode>(CustomMovementMode);
}


FVector UAdvancedMovementComponent::GetMantleLedgeLocation() const
{
	return MantleLedgeLocation;
}


FVector UAdvancedMovementComponent::GetLedgeClimbLocation() const
{
	return LedgeClimbLocation;
}


FVector UAdvancedMovementComponent::GetLedgeClimbNormal() const
{
	return LedgeClimbNormal;
}

FVector UAdvancedMovementComponent::GetWallJumpLocation() const
{
	return PrevWallJumpLocation;
}

FVector UAdvancedMovementComponent::GetWallJumpNormal() const
{
	return PrevWallJumpNormal;
}

float UAdvancedMovementComponent::GetPreviousWallJumpTime() const
{
	return PrevWallJumpTime;
}


FVector UAdvancedMovementComponent::ComputeSlideVector(const FVector& Delta, const float HitTime, const FVector& Normal, const FHitResult& Hit) const
{
	FVector Result = Super::ComputeSlideVector(Delta, HitTime, Normal, Hit);

	// prevent boosting up slopes
	if (IsWallClimbing() || IsFalling())
	{
		Result = HandleSlopeBoosting(Result, Delta, HitTime, Normal, Hit);
	}

	return Result;
}


void UAdvancedMovementComponent::ResetFallingStateInformation(EMovementMode PrevMode, uint8 PrevCustomMode)
{
// 	BaseAbilitySystem = BaseAbilitySystem ? BaseAbilitySystem : GetAbilitySystem();
// 	if (!BaseAbilitySystem) return;
//
// 	if (IsFalling())
// 	{
// 		if (!BaseAbilitySystem->HasMatchingGameplayTag(InAirTag))
// 		{
// 			BaseAbilitySystem->AddLooseGameplayTag(InAirTag);
// 			BaseAbilitySystem->AddReplicatedLooseGameplayTag(InAirTag);
// 		}
// 	}
// 	else
// 	{
// 		if (BaseAbilitySystem->HasMatchingGameplayTag(InAirTag))
// 		{
// 			BaseAbilitySystem->RemoveLooseGameplayTag(InAirTag);
// 			BaseAbilitySystem->RemoveReplicatedLooseGameplayTag(InAirTag);
// 		}
// 	}
}


void UAdvancedMovementComponent::ResetGroundStateInformation(EMovementMode PrevMode, uint8 PrevCustomMode)
{
	if (IsMovingOnGround())
	{
		WalkingStartTime = Time;
		
		if (IsStrafeLurching())
		{
			DisableStrafeLurchPhysics();
		}
	}

}


// UBaseAbilitySystem* UAdvancedMovementComponent::GetAbilitySystem() const
// {
// 	if (!BaseCharacter) return nullptr;
// 	return BaseCharacter->GetBaseAbilitySystem();
// }


void UAdvancedMovementComponent::DebugGroundMovement(FString Message, FColor Color, bool DrawSphere)
{
	if (!bDebugGroundMovement) return;
	GEngine->AddOnScreenDebugMessage(2, 10.f, Color, Message);
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.5f * FVector::DownVector;
	DrawDebugLine(GetWorld(), Start, End, Color, false, TraceDuration);
	if (DrawSphere) DrawDebugSphere(GetWorld(), Start, 10.f, 12, Color, false, TraceDuration);
}


FString UAdvancedMovementComponent::GetMovementDirection(const FVector2D& InputVector) const
{
	const float Forwards = InputVector.X;
	const float Sideways = InputVector.Y;

	if (Sideways != 0 && Forwards != 0)
	{
		if (Forwards > 0) return Sideways > 0 ? FString("FR") : FString("FL");
		else return Sideways > 0 ? FString("BR") : FString("BL");
	}

	if (Sideways == 0 && Forwards == 0) return FString("N");
	if (Sideways == 0) return Forwards > 0 ? FString("F") : FString("B");
	return Sideways > 0 ? FString("R") : FString("L");
}
#pragma endregion
