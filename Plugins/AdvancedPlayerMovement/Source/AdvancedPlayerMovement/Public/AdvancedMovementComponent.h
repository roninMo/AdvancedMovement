#pragma once


#include "CoreMinimal.h"
#include "MovementInformation.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AdvancedMovementComponent.generated.h"

// CMC network breakdown
// First on tick the perform move function is called, which executes all the movement logic
// Then it creates a saved move, and uses SetMoveFor to read the safe values and store them in the saved values
// Calls canCombineMoveWith to check if there are any identical moves that can be combined to save bandwidth if necessary
// Then Calls getCompressed flags to reduce the saved move into the small networkable packet that it sends to the server
// Then when the server receives this move it will call updateFromCompressedFlags, and update the state variables with the values sent to it
// Then everything should be replicated and there shouldn't be any weird rubber banding conflicts with the code

// NOTE:
// You can only call moves that alter safe variables on the client
// You can alter movement safe variables in non safe momement functions on the client
// You should never utilize non movement safe variables in a movement safe function so don't call non movement safe functions that alter movement safe variables on the server




/*
*@Documentation Extending Saved Move Data ->  https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine

To add new data, first extend FSavedMove_Character to include whatever information your Character Movement Component needs.
Next, extend FCharacterNetworkMoveData and add the custom data you want to send across the network; in most cases, this mirrors the data added to FSavedMove_Character.
You will also need to extend FCharacterNetworkMoveDataContainer so that it can serialize your FCharacterNetworkMoveData for network transmission, and deserialize it upon receipt. When this setup is finised, configure the system as follows:

	- Modify your Character Movement Component to use the FCharacterNetworkMoveDataContainer subclass you created with the SetNetworkMoveDataContainer function.
		The simplest way to accomplish this is to add an instance of your FCharacterNetworkMoveDataContainer to your Character Movement Component child class, and call SetNetworkMoveDataContainer from the constructor.

	- Since your FCharacterNetworkMoveDataContainer needs its own instances of FCharacterNetworkMoveData, point it (typically in the constructor) to instances of your FCharacterNetworkMoveData subclass.
		See the base constructor for more details and an example.

	- In your extended version of FCharacterNetworkMoveData, override the ClientFillNetworkMoveData function to copy or compute data from the saved move.
		Override the Serialize function to read and write your data using an FArchive; this is the bit stream that RPCs require.

	- To extend the server response to clients, which can acknowledges a good move or send correction data, extend FCharacterMoveResponseData, FCharacterMoveResponseDataContainer,
		and override your Character Movement Component's version of the SetMoveResponseDataContainer.
*/


/*
* Character Movement Component In Depth

* Start at the StartNewPhysics function, this is the function that actually moves the character through the world. It checks your movement mode, and then it runs a specific function based on that
	* PhysWalking
	* PhysNavWalking
	* PhysFlying
	* PhysSwimming
	* PhysCustom
	* HandleSwimmingWallHit
	

/// Tick Component ///
* if you move the character by pressing a key, what happens (start at the tick component)
	* It gets the input vectors to calculate the character's movement
	* It checks whether your the autonomous or simulated proxy, if it's the authoritative actor then check if there's prediction data from the server
		* Then, if it's an autonomous proxy, call PerformMovement, and send the saved move to the client.
		* If it's a simulated proxy, call ReplicateMoveToServer (this takes in deltaTime and acceleration)

		// PerformMovement()

		// ReplicateMoveToServer()






// FSavedMove_Character
* Stores a bunch of information about where you were when you started and ended moving, and the physics and acceleration information
* The breakdown of how this is used is this:
	* the ClienData creates a new saved move
	* It records the movement
	* If multiple movements are close to the same then it combines multiple of these movements before sending it to the server
	* It then calls PerformMovement, which is what the server does immediately
	* After that, it calls PostUpdate(), which records the movement information after running the PerformMovement function
	* So every saved move is a record of the information pertaining to the initial location and movement information and the calculated information after you perform a move
	* It saves this information in an array of moves for the server to check against it's own recorded movements, and acknowledges them as valid or sends back a correction to the client 
	* It then runs CallServerMove(), and the server runs it's logic to check whether this is a valid move 
	* There's multiple serverMove functions, and the main one that's called is ServerMove(), and this is where's it's capturing the main movement information to send across the server
	* So in this case, this is where we'd send specific information and extra if necessary for handling specific movement functionality
		* This is where it's grabbing the acceleration, the compressedFlags which is how we're calculating specific input presses like jumping, crouching, sprinting, and so on. 
	
	* ServerMove is a net multicast rpc that runs on both the server and client
		* This unpacks the movement information, runs logic that prevents hacking, and then runs MoveAutoonomous
		* MoveAutonomous replays the move that you saved and sent to the server
		* After MoveAutonomous, the server checks if this is a valid client move by testing it against it's own movement steps
		* It does this through ServerMoveHandleClientError, and this runs ServerCheckClientError. 
			* If the move is invalid, it sets up movement information for correction, and sends that information to the client.
		* Otherwise, it sets the pendingAdjustments timestamp and acknowledges it as a good move,  

		// In the case of an invalid move
		* The invalidMove logic does not set whether it acknowledged it as a bad move in the characterMovementComponent, it actually also locks it down in the UNetDriver.ServerReplicateActors function
		* This is within the NetworkDriver class and sorry this is kinda confusing, but it implements the INetworkPRedictionInterface, and that's what runs the SendClientAdjustment function
		* Then the Character movement component runs SendClientAdjustment
		* This is what runs the ClientAdjustPosition function with the pendingAdjustment information

		* After a good move, it then runs ClientAckGoodMove which acknowledges the good move and this deletes the move from the list
		* After a bad move, ClientAdjustPositon rpc has been called and handled, set it's location and movement information, it sets it's ClientData->bUpdatePosition to true, and in the PostPhysicsTickComponent, and then it handles the correction
			* It then runs back through it's saved moves, and repeats these moves that have not been acknowledged yet from the corrected postion


	* Simulated proxies runs the simulatedTick function to roughly replicate the movement functions


	Adding input replication for the character movement component is trivial and not complicated but you need to be careful about how much information you send across the server
	If everything's already in place, and you're just trying to recouple the client logic for things like sliding and wall jumping that's easy to handle with custom replication flags
	
	If you need to handle anything that's time based, it get's a little tricky because the client and the server don't run on the same tick and you have to account for both the delta time and the function calls to vary between them
		- I thought of a way around this (which is similar to what I was doing with mantling, but I think I just need to catch up with all of the logic anyways)
			- The saved moves captures logic and information within the components, but the movement component is a little possessive and obsessive with what it captures and renders every frame (just let it do it's thing)
			- Regardless, the client and the server are in sync with every calculation that's done with these, even if the server handles these at a later time (and replicates this to other clients while smoothing out the edges)
			- So for things like mantling where you need a smooth interp over an animation (root motion source) (I think) you should actually handle the time based calculations on the client and rep them to the server
			- I just do it with a boolean flag because the simulated proxies also don't register or truly closely couple their logic or config with the server (but this comes back to bite you later (I think it's a good learning lesson tho)

			- But for things like, after you land a wall jump where you need physics tick frames of some other conditional logic, you should pass this information to the savedMoves for replication also
				- The only downside to this is hacking, but (I don't know (potentially (I always think I'm lying or doing something bad)) another way of handling this)
			- Even if they were hacking to exploit things like this, I don't think they'd gain much from doing something like this (other than be incomphrehensibly rude because they don't get the gravity of things or they're just like thirsty or something hoeish)
			- Either way, the client and the server need to be in sync with what moves they calculate to help facilitate building up the character movement, and everything else is just what you want to focus on
			
				


*/




////////////////////////////////////////////////// 
// Error handling								//
////////////////////////////////////////////////// 
///// Network /////
// "p.NetShowCorrections 1"
// "emulationPkLag 500"

///// Ability System /////
// "showdebug abilitysystem"
// "AbilitySystem.Debug.NextCategory"
// NextDebugTarget or PgUpKey 

///// Movement /////
// "p.VisualizeMovement 1"



/*
* For handling state use the handleInAirState and handleCrouchState,
*/
UCLASS()
class UAdvancedMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
//----------------------------------------------------------------------------------------------------------------------------------//
// Movement																															//
//----------------------------------------------------------------------------------------------------------------------------------//
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|General") float SprintSpeedMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|General") float CrouchSprintSpeedMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|General") float AimSpeedMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|General") float WalkSpeedMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|General") float SlideSpeedLimit;

	
//----------------------------------------------------------------------------------------------------------------------------------//
// Bhop																																//
//----------------------------------------------------------------------------------------------------------------------------------//
	// AirStrafeSpeedGainMultiplier just determines how much velocity is added during strafing
	// AirStrafeRotationRate Calculations (How much speed you're able to attain before drag starts to be added during normal (90-180) strafing rotations)
	// The default values for decent bhop movement is 0.064 gain and 3.4 strafe
	// decreasing the strafe at higher speeds should help with preventing misuse, right now between 2000-3000 you'll get drag if you don't slow down your turns
	// Adding more strafe and less gain at beginning speeds should make it easier
	// Speed and acceleration affect these values. For strafing the acceleration is set to 6400
	
	
/*

	- Movement
	- Bhopping
	- Wall Jumping
	- Wall Running
	- Wall Climbing
	- Mantling
	- Sliding
	- Ledge Climbing
	- Ledge Jumping


	- Frame combinations for things like slide boosting out of ledge jumps, and other scenarios, and check that this is safe with different frame rates


	- Things that don't need specific inputs to be achieved

		- Wall Jumping
		- Wall Running
		- Wall Climbing
		- Mantling
		- Sliding
		- Ledge Climbing

	- Wall jumps need AirStrafeSwayPhysics, and add StrafeLurching during ledge jumps
	- Why don't we allow extra strafing if they press the jump input? Or figure out a good way of handling strafing during different states
	


	// FLAG_Reserved_1 = 0x04, // Reserved for future use (be a rebel and use these anyways)
	// FLAG_Reserved_2 = 0x08, // Reserved for future use (be a rebel and use these anyways)
	// FLAG_Custom_3 = 0x80, // WallJumping
	// FLAG_Custom_2 = 0x40, // Aiming
	// FLAG_Custom_1 = 0x20, // Mantling
	// FLAG_Custom_0 = 0x10, // Sprinting



*/
	
	
protected:
	/** Uses bunny hopping and air strafing physics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)")
	bool bUseBhopping;
	
	/** Max Strafing Acceleration (how fast you speed up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|Air Strafe", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "10000", EditCondition = "bUseBhopping", EditConditionHides))
	float StrafingMaxAcceleration;

	/** How much speed should be gained during air strafing? This is a multiplier based on the character's acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "5", EditCondition = "bUseBhopping", EditConditionHides))
	float AirStrafeSpeedGainMultiplier;

	/** This influences the rotation rate during air strafing while you're gaining speeds. The higher the value, the more you'll be able to gain speed while turning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "10", EditCondition = "bUseBhopping", EditConditionHides))
	float AirStrafeRotationRate;

	/** The raw strafe sway duration of inhibited movement after performing a wall jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe|Air Strafe Sway", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "1", EditCondition = "bUseBhopping", EditConditionHides))
	float StrafeSwayDuration;
	
	/** Speed gained during strafe is tied to the character's acceleration, and this multiplies how much is gained during a strafe */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe|Air Strafe Sway", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "5", EditCondition = "bUseBhopping", EditConditionHides))
	float StrafeSwaySpeedGainMultiplier;

	/** The rotation rate of the character that allows preserving the player's speed while turning. This is influenced by the character's current speed, so at higher speeds if they try strafing it'll slow them down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe|Air Strafe Sway", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "10", EditCondition = "bUseBhopping", EditConditionHides))
	float StrafeSwayRotationRate;

	/** air strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Movement (General Settings)|Air Strafe|Debug", meta=(EditCondition = "bUseBhopping", EditConditionHides))
	bool bDebugAirStrafe;
	
	/** strafe sway physics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Movement (General Settings)|Air Strafe|Debug", meta=(EditCondition = "bUseBhopping", EditConditionHides))
	bool bDebugStrafeSway;

	
protected:
	/** Whether Air Strafe Sway physics are enabled. If this is true, the player doesn't slow down if they press inputs in the opposite direction of the player's current movement. */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe") bool AirStrafeSwayPhysics;
	
	/** The time strafe sway was previously activated during different physics logic. */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Air Strafe") float StrafeSwayStartTime;

	
//----------------------------------------------------------------------------------------------------------------------------------//
// Wall Jumping																														//
//----------------------------------------------------------------------------------------------------------------------------------//
protected:
	/** Uses wall jumping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)")
	bool bUseWallJumping;
	
	/** The speed of wall jumps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump", meta=(UIMin = "0", UIMax = "1000", EditCondition = "bUseWallJumping", EditConditionHides))
	float WallJumpSpeed;
	
	/** An additional velocity multiplier to adjust the wall jump's velocity gains */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump", meta=(EditCondition = "bUseWallJumping", EditConditionHides))
	FVector WallJumpBoost;

	/** An additional velocity multiplier during wall climbs to adjust the wall jump's velocity gains */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump", meta=(EditCondition = "bUseWallJumping", EditConditionHides))
	FVector WallJumpBoostDuringWallClimbs;
	
	/** An additional velocity multiplier during wall runs to adjust the wall jump's velocity gains */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump", meta=(EditCondition = "bUseWallJumping", EditConditionHides))
	FVector WallJumpBoostDuringWallRuns;
	
	/** How far away is a wall allowed to for the player to be able to wall jump against */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump", meta=(UIMin = "0", UIMax = "343", EditCondition = "bUseWallJumping", EditConditionHides))
	float WallJumpValidDistance;

	/** How high the character should be from the ground before wall jumping is valid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump", meta=(UIMin = "30", UIMax = "100", EditCondition = "bUseWallJumping", EditConditionHides))
	float WallJumpHeightFromGroundThreshold;

	/** wall jump checks/traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Movement (General Settings)|Wall Jump|Debug", meta=(EditCondition = "bUseWallJumping", EditConditionHides))
	bool bDebugWallJumpTrace;

	/** wall jump trajectory logic and information */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Movement (General Settings)|Wall Jump|Debug", meta=(EditCondition = "bUseWallJumping", EditConditionHides))
	bool bDebugWallJumpTrajectory;

	
protected:
	/** Used to determine whether they're trying to perform multiple wall jumps before landing on the ground  */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump") FVector PrevWallJumpNormal;

	/** The location of the player the last time they were on the ground */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Jump") FVector PreviousGroundLocation;


//----------------------------------------------------------------------------------------------------------------------------------//
// Wall Climbing																													//
//----------------------------------------------------------------------------------------------------------------------------------//
protected:
	/** Uses wall climbing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)")
	bool bUseWallClimbing;
	
	/** The duration the player is able to climb a wall. Setting this to zero means there is no duration */ // TODO: add this code
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMin = "0", UIMax = "10", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbDuration;

	/** The interval between when a player is allowed to wall climb if they've already completed a climb */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMin = "0", UIMax = "10", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbInterval;
	
	/** The speed the player climbs the wall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMin = "0", UIMax = "1000", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbSpeed;

	/** The player's climb speed acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMin = "0", UIMax = "1000", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbAcceleration;
	
	/** How much speed should factor into wall climbing forwards/sideways/up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(EditCondition = "bUseWallClimbing", EditConditionHides))
	FVector WallClimbMultiplier;

	/** Up to what angle is wall climbing allowed? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMin = "0", UIMax = "90", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbAcceptableAngle;
	
	/** The friction climbing a wall when a player comes from a falling state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMin = "0", UIMax = "10", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbFriction;

	/** If the player was previously falling, what speed do we start adding velocity from? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing", meta=(UIMax = "0", ClampMax = "0", EditCondition = "bUseWallClimbing", EditConditionHides))
	float WallClimbAddSpeedThreshold;

	/** Wall climbing information */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|Wall Climbing|Debug", meta=(EditCondition = "bUseWallClimbing", EditConditionHides))
	bool bDebugWallClimb;

	
protected:
	/** The previous location the player had started wall climbing */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing") FVector PrevWallClimbLocation;

	/** When the player had began climbing */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing") float WallClimbStartTime;
	
	/** The previous time the player had completed a wall climb */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Climbing") float PrevWallClimbTime;
	
	
//----------------------------------------------------------------------------------------------------------------------------------//
// Wall Running																														//
//----------------------------------------------------------------------------------------------------------------------------------//
protected:
	/** Uses wall climbing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)")
	bool bUseWallRunning;
	
	/** The duration the player is able to run alongside a wall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(UIMin = "0", UIMax = "10", EditCondition = "bUseWallRunning", EditConditionHides))
	float WallRunDuration;

	/** The speed the player runs alongside the wall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(UIMin = "0", UIMax = "1000")) float WallRunSpeed;

	/** The player's wall run acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(UIMin = "0", UIMax = "1000", EditCondition = "bUseWallRunning", EditConditionHides))
	float WallRunAcceleration;
	
	/** How much speed should factor into wall running forwards/sideways/up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(EditCondition = "bUseWallRunning", EditConditionHides))
	FVector WallRunMultiplier;
	
	/** How quickly the character should be moving before they're allowed to wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(UIMin = "0", UIMax = "1000", EditCondition = "bUseWallRunning", EditConditionHides))
	float WallRunSpeedThreshold;

	/** The acceptable angle the character can be facing while running alongside a wall. ex: 30 -> 90-30 to 90+30  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(UIMin = "0", UIMax = "90", EditCondition = "bUseWallRunning", EditConditionHides))
	float WallRunAcceptableAngleRadius;

	/** If a player is wall running on the same wall, what is the acceptable height difference to wall jump an additional time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(UIMin = "0", UIMax = "500", EditCondition = "bUseWallRunning", EditConditionHides))
	float WallRunHeightThreshold;

	/** Whether the player should run backwards when facing away from the wall during a wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running", meta=(EditCondition = "bUseWallRunning", EditConditionHides))
	bool bShouldRunBackwardsIfFacingAwayFromWall;

	/** Wall run information */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|Wall Running|Debug", meta=(EditCondition = "bUseWallRunning", EditConditionHides))
	bool bDebugWallRunning;

	
protected:
	/** When did they begin wall running? */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running") float WallRunStartTime;

	/** The input direction the player is pressing to run alongside a wall. This helps with knowing when to transition and if they're running left or right */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running") float WallRunInputDirection;

	/** The speed of the wall run. This is either capped to the wall run speed, or the speed they were going before they were wall running if they were going faster */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running") float WallRunCurrentSpeed;
	
	/** The wall run location, we don't want them running on the same wall multiple times, unless it's at a different height */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running") FVector WallRunLocation;

	/** The wall the player is running on */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running") UPrimitiveComponent* WallRunWall;
	
	/** The wall run's normal. This helps with knowing which way to add velocity, and if they run on the same wall at the same height, it needs to be at a different acceptable angle */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Character Movement (General Settings)|Wall Running") FVector WallRunNormal;
	
	
//----------------------------------------------------------------------------------------------------------------------------------//
// Sliding																															//
//----------------------------------------------------------------------------------------------------------------------------------//
protected:
	/** Uses sliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)")
	bool bUseSliding;
	
	/** The required movement speed to be allowed to slide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0.0", UIMax = "1000", EditCondition = "bUseSliding", EditConditionHides))
	float SlideEnterThreshold;
	
	/** The initial boost once you enter the slide movement mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0.0", UIMax = "1000", EditCondition = "bUseSliding", EditConditionHides))
	float SlideEnterImpulse;

	/** How much the character's able to rotate while sliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0", UIMax = "1", EditCondition = "bUseSliding", EditConditionHides))
	float SlidingRotationRate;

	/** This is a raw value added to slope for adding a base friction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0", UIMax = "1", EditCondition = "bUseSliding", EditConditionHides))
	float SlidingFriction;

	/** This multiplies the angle of the slope the character is currently on and this is added to the sliding friction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0", UIMax = "5", EditCondition = "bUseSliding", EditConditionHides))
	float SlideAngleFrictionMultiplier;

	/** This is braking friction of sliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0", UIMax = "5", EditCondition = "bUseSliding", EditConditionHides))
	float SlideBrakingFriction;
	
	/** The multiplier added to the slide jump for forward movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Movement (General Settings)|Sliding", meta=(UIMin = "0", UIMax = "1000", EditCondition = "bUseSliding", EditConditionHides))
	float SlideJumpSpeed;
	
	/** sliding information */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)|Sliding|Debug", meta=(EditCondition = "bUseSliding", EditConditionHides))
	bool bDebugSlide;
	
	
//----------------------------------------------------------------------------------------------------------------------------------//
// Other																															//
//----------------------------------------------------------------------------------------------------------------------------------//
protected:
	/** land movement and information */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Debugging") bool bDebugGroundMovement;

	/** Debug movement mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Debugging") bool bDebugMovementMode;
	
	/** Debug network replication */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Debugging") bool bDebugNetworkReplication;
	
	/** Debug mantling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Debugging") bool bDebugVault;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Debugging|Mantling") float MantleTraceDuration = 5;

	/** The physics channel for tracing against objects in the world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement (General Settings)") TEnumAsByte<ETraceTypeQuery> MovementChannel;
	

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// Bhop Character Movement Component																																 						 //
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
protected:
	/**
	 * Initializes the component.  Occurs at level startup or actor spawn. This is before BeginPlay (Actor or Component).
	 * All Components in the level will be Initialized on load before any Actor/Component gets BeginPlay
	 */
	virtual void InitializeComponent() override;

	
//------------------------------------------------------------------------------//
// General Movement Logic														//
//------------------------------------------------------------------------------//
public:
	/** Returns maximum speed of component in current movement mode. */
	virtual float GetMaxSpeed() const override;

	/** Checks if the MovementMode is Custom and the specific custom submode passed in the the same as the current movement mode. */
	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;

	/** If the player is walking, nav walking, or sliding */
	virtual bool IsMovingOnGround() const override;
	
	/** If the player is sliding */
	UFUNCTION(BlueprintCallable) virtual bool IsSliding() const;

	/** If the player is running */
	UFUNCTION(BlueprintCallable) virtual bool IsRunning() const;

	/** Is the player is wall climbing */
	UFUNCTION(BlueprintCallable) virtual bool IsWallClimbing() const;
	
	/** Is the player is wall running */
	UFUNCTION(BlueprintCallable) virtual bool IsWallRunning() const;
	
	/** If the player is mantling */
	UFUNCTION(BlueprintCallable) virtual bool IsMantling() const;

	/** If the player is aiming */
	UFUNCTION(BlueprintCallable) virtual bool IsAiming() const;

	/** If the player is strafe swaying */
	UFUNCTION(BlueprintCallable) virtual bool IsStrafeSwaying();

	
//------------------------------------------------------------------------------//
// Update Movement Mode Logic													//
//------------------------------------------------------------------------------//
protected:
	/**
	 * Updates the character state in PerformMovement right before doing the actual position change
	 * This handles updating the movement mode updates from player inputs
	 */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	/**
	 * Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner.
	 * This updates the character's state information, and handles the enter and exit logic for different movement modes
	 */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	/**
	 * Event activated at the end of a movement update. If scoped movement updates are enabled (bUseScopedMovementUpdates), this is within such a scope.
	 * This handles updating this component's movement values for a specific movement mode once it's been updated
	 */
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	
//------------------------------------------------------------------------------//
// Physics Functions															//
//------------------------------------------------------------------------------//
protected:
	/** changes physics based on MovementMode */
	virtual void StartNewPhysics(float deltaTime, int32 Iterations) override;
	
	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysWalking(float deltaTime, int32 Iterations) override;

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysSlide(float deltaTime, int32 Iterations);

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysWallClimbing(float deltaTime, int32 Iterations);

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysMantling(float deltaTime, int32 Iterations);
	
	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysWallRunning(float deltaTime, int32 Iterations);
	
	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	/** 
	 * Updates Velocity and Acceleration based on the current state, applying the effects of friction and acceleration or deceleration. Does not apply gravity.
	 * This is used internally during movement updates. Normally you don't need to call this from outside code, but you might want to use it for custom movement modes.
	 *
	 * @param	DeltaTime						time elapsed since last frame.
	 * @param	Friction						coefficient of friction when not accelerating, or in the direction opposite acceleration.
	 * @param	bFluid							true if moving through a fluid, causing Friction to always be applied regardless of acceleration.
	 * @param	BrakingDeceleration				deceleration applied when not accelerating, or when exceeding max velocity.
	 */
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	
//------------------------------------------------------------------------------//
// Falling Movement Logic														//
//------------------------------------------------------------------------------//
protected:
	/** This applies the velocity and acceleration based on the input acceleration, air control, gravity, and jump force of the character before calculating collisions and updating the character's movement
	 * @note This is Movement update function logic, and should only be called through Physics functions
	 */
	virtual void HandleFallingFunctionality(float deltaTime, float& timeTick, int32& Iterations, float& remainingTime,
		const FVector& OldVelocity, const FVector& OldVelocityWithRootMotion, FVector& Adjusted, FVector& Gravity, float& GravityTime);

	/** This handles moving the player and any air adjustments with other objects
	 * @note This is Movement update function logic, and should only be called through Physics functions
	 * */
	virtual void FallingMovementPhysics(float deltaTime, float& timeTick, int32& Iterations, float& remainingTime,
		const FVector& OldLocation, const FVector& OldVelocity, const FQuat& PawnRotation, FVector& Adjusted,
		bool bHasLimitedAirControl, FVector& Gravity, float& GravityTime); 

	/** Checks if there's a valid wall to wall jump from either the safeMoveUpdatedComponent and additional logic. Also captures the wall jump angle for necessary calculations
	 * @returns true if the player is trying to jump and there's a valid wall to wall jump from
	 * @note SafeMoveUpdatedComponent adjusts the velocity, but it's kind of buggy when you're handling multiple objects in the way of the character's movements, so this is handled on it's own
	 */
	virtual bool WallJumpValid(float deltaTime, const FVector& OldLocation, const FVector& InputVector, FHitResult& JumpHit, const FHitResult& Hit); 
	
	/**
	 * Calculates the wall jump trajectory and performs the wall jump. Always invoke WallJumpValid() before calculating the trajectory
	 *
	 * @param Wall				The wall they're jumping off of
	 * @param TimeTick			The current time tick
	 * @param Iterations		The current physics step iteration
	 * @param Speed				The speed of the wall jump
	 * @param Boost				The speed multiplier for the wall jump
	 * @param PrevState			This is used for debugging to determine the logic before the wall jump
	 */
	virtual void CalculateWallJumpTrajectory(const FHitResult& Wall, float TimeTick, int32 Iterations, float Speed, FVector Boost, FString PrevState = FString("Falling"));

	/** Captures information for wall jumping during different movement modes when the movement mode has been updated */
	virtual void ResetWallJumpInformation(EMovementMode PrevMode, uint8 PrevCustomMode);

	
//------------------------------------------------------------------------------//
// Ground Movement Logic														//
//------------------------------------------------------------------------------//
protected:
	/** This applies the velocity and acceleration based on the inputs of the character.
	 * @note This is Movement update function logic, and should only be called through Physics functions
	 */
	virtual void BaseWalkingFunctionality(float deltaTime, int32& Iterations, const float timeTick);

	/** This applies the velocity and acceleration based on the character inputs and slope that they're on.
	 * @note This is Movement update function logic, and should only be called through Physics functions
	 */
	virtual void BaseSlidingFunctionality(float deltaTime, int32& Iterations, const float timeTick);
	
	/** This is the base physics for ground movement. It handles moving the character forward, checking for falling and ledges, transitions to other movement modes, and calculating the movement.
	 * @note This is Movement update function logic, and should only be called through Physics functions
	 */
	virtual void GroundMovementPhysics(float deltaTime, int32 Iterations);

	
//------------------------------------------------------------------------------//
// Slide Logic																	//
//------------------------------------------------------------------------------//
public:
	/** Returns true if current movement state and surface is valid for sliding, and if the player meets the minimum speed requirements for sliding */
	UFUNCTION(BlueprintCallable) virtual bool CanSlide() const;


protected:
	/** Adds a slide impulse to boost the character forwards (this is also a fix for when character's potentially get stuck on ledges */
	virtual void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);

	/** Exit slide logic */
	virtual void ExitSlide();
	
	
//------------------------------------------------------------------------------//
// WallClimb Logic																//
//------------------------------------------------------------------------------//
public:
	/** Returns true if current movement state and wall is valid for climbing, and if the player trying to climb the wall */
	UFUNCTION(BlueprintCallable) virtual bool CanWallClimb() const;

	
protected:
	/** Enter wall climb logic */
	virtual void EnterWallClimb(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);

	/** Exit wall climb logic */
	virtual void ExitWallClimb();

	/** Reset wall climb information based on specific states and movement modes */
	virtual void ResetWallClimbInformation(EMovementMode PrevMode, uint8 PrevCustomMode);
	
	
//------------------------------------------------------------------------------//
// WallClimb Logic																//
//------------------------------------------------------------------------------//
public:
	/** Returns true if current movement state and wall is valid for running, and if the player trying to wall run */
	UFUNCTION(BlueprintCallable) virtual bool CanWallRun(const FHitResult& Wall) const;

	
protected:
	/** Enter wall run logic */
	virtual void EnterWallRun(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);

	/** Exit wall run logic */
	virtual void ExitWallRun();

	/** Reset wall run information based on specific states and movement modes */
	virtual void ResetWallRunInformation(EMovementMode PrevMode, uint8 PrevCustomMode);
	
	
//------------------------------------------------------------------------------//
// Custom FSavedMove related function											//
//------------------------------------------------------------------------------//
public:
	/** Unpack compressed flags from a saved move and set state accordingly. See FSavedMove_Character. */
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	/** Get prediction data for a client game. Should not be used if not running as a client. Allocates the data on demand and can be overridden to allocate a custom override if desired. Result must be a FNetworkPredictionData_Client_Character. */
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/* Process a move at the given time stamp, given the compressed flags representing various events that occurred (ie jump). */
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	
	
	//////////////////////////////////////////////////////////////////
	// Custom FCharacterNetworkMoveData								//
	//////////////////////////////////////////////////////////////////
	class FMCharacterNetworkMoveData : public FCharacterNetworkMoveData
	{
	public:
		typedef FCharacterNetworkMoveData Super;
		FVector MoveData_Time;
		FVector_NetQuantize10 MoveData_Input;
		
		virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
		virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
	
		
	};
	
	
	//////////////////////////////////////////////////////////////////
	// Custom FCharacterNetworkMoveDataContainer					//
	//////////////////////////////////////////////////////////////////
	class FMCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
	{
	public:
		FMCharacterNetworkMoveDataContainer();
		FMCharacterNetworkMoveData CustomDefaultMoveData[3]; // [New, Pending, Old];
	
		
	};
	
	
	//////////////////////////////////////////////////////////////////
	// Custom FSavedMove_Character									//
	//////////////////////////////////////////////////////////////////
	class FMSavedMove : public FSavedMove_Character
	{
		public:
			typedef FSavedMove_Character Super;

			/* Clear saved move properties, so it can be re-used. */
			virtual void Clear() override;

			// @brief Store input commands in the compressed flags.
			// This is the minimal movement information that's sent to the server every frame for replication
			virtual uint8 GetCompressedFlags() const override;

			// @brief Returns true if this move can be combined with NewMove for replication without changing any behavior.
			// Basically you just check to make sure that the saved variables are the same.
			virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;

			// @brief Sets up the move before sending it to the server. 
			virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;

			// @brief Sets variables on character movement component before making a predictive correction.
			virtual void PrepMoveFor(ACharacter* Character) override;
			
			// Custom saved move information and Other values values we want to pass across the network
			float Time;
			FVector PlayerInput;
		
			// Without customizing the movement component these are the remaining flags for creating new functionality
			uint8 SavedRequestToStartWallJumping : 1;
			uint8 SavedRequestToStartAiming : 1;
			uint8 SavedRequestToStartMantling : 1;
			uint8 SavedRequestToStartSprinting : 1;
			//FLAG_Reserved_1 = 0x04, // Reserved for future use (be a rebel and use these anyways)
			//FLAG_Reserved_2 = 0x08, // Reserved for future use (be a rebel and use these anyways)
			//FLAG_Custom_3 = 0x80, // WallJumping
			//FLAG_Custom_2 = 0x40, // Aiming
			//FLAG_Custom_1 = 0x20, // Mantling
			//FLAG_Custom_0 = 0x10, // Sprinting
		
	};

	
	//////////////////////////////////////////////////////////////////
	// Custom FNetworkPredictionData_Client							//
	//////////////////////////////////////////////////////////////////
	// Indicate to the cmc that we're using our custom move FSavedMove_Bhop
	class FMNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
	{
		public:
			typedef FNetworkPredictionData_Client_Character Super;
			FMNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement);

			// @brief Allocates a new copy of our custom saved move
			virtual FSavedMovePtr AllocateNewMove() override;
	};

	
	UAdvancedMovementComponent();
	friend class FMSavedMove;
	FMCharacterNetworkMoveDataContainer CustomMoveDataContainer;

	// Custom movement information
	UPROPERTY(BlueprintReadWrite) FVector PlayerInput; // VelocityOriented input values (Acceleration)
	UPROPERTY(BlueprintReadWrite) uint8 WallJumpPressed : 1;
	UPROPERTY(BlueprintReadWrite) uint8 AimPressed : 1;
	UPROPERTY(BlueprintReadWrite) uint8 Mantling : 1;
	UPROPERTY(BlueprintReadWrite) uint8 SprintPressed : 1;
	UPROPERTY(BlueprintReadWrite) float Time;

	
	
	
//------------------------------------------------------------------------------//
// Input Functions																//
//------------------------------------------------------------------------------//
public:
	UFUNCTION(BlueprintCallable) void StartMantling();
	UFUNCTION(BlueprintCallable) void StopMantling();
	
	UFUNCTION(BlueprintCallable) void StartSprinting();
	UFUNCTION(BlueprintCallable) void StopSprinting();

	UFUNCTION(BlueprintCallable) void StartAiming();
	UFUNCTION(BlueprintCallable) void StopAiming();

	UFUNCTION(BlueprintCallable) void UpdatePlayerInput(const FVector& InputVector);
	
	UFUNCTION(BlueprintCallable) void StartWallJump();
	UFUNCTION(BlueprintCallable) void StopWallJump();
	
	UFUNCTION(BlueprintCallable) void DisableStrafeSwayPhysics();
	UFUNCTION(BlueprintCallable) void EnableStrafeSwayPhysics();

	
//------------------------------------------------------------------------------//
// Jump Logic																	//
//------------------------------------------------------------------------------//
public:
	/**
	 * Perform jump. Called by Character when a jump has been detected because Character->bPressedJump was true. Checks Character->CanJump().
	 * Note that you should usually trigger a jump through Character::Jump() instead.
	 * @param	bReplayingMoves: true if this is being done as part of replaying moves on a locally controlled client after a server correction.
	 * @return	True if the jump was triggered successfully.
	 */
	virtual bool DoJump(bool bReplayingMoves) override;
	
	/**
	 * Returns true if current movement state allows an attempt at jumping. Used by Character::CanJump().
	 * @note This needs to be linked to the character's function in order for this to work
	*/
	virtual bool CanAttemptJump() const override;
	
	
//------------------------------------------------------------------------------//
// Crouch Logic																	//
//------------------------------------------------------------------------------//
public:
	/**
	 * Checks if new capsule size fits (no encroachment), and call CharacterOwner->OnStartCrouch() if successful.
	 * In general you should set bWantsToCrouch instead to have the crouch persist during movement, or just use the crouch functions on the owning Character.
	 * @param	bClientSimulation	true when called when bIsCrouched is replicated to non owned clients, to update collision cylinder and offset.
	 */
	virtual void Crouch(bool bClientSimulation) override;
	
	/**
	 * Checks if default capsule size fits (no encroachment), and trigger OnEndCrouch() on the owner if successful.
	 * @param	bClientSimulation	true when called when bIsCrouched is replicated to non owned clients, to update collision cylinder and offset.
	 */
	virtual void UnCrouch(bool bClientSimulation) override;
	
	/** Add State tags for whether the character is crouching */
	virtual void HandleCrouchLogic();
	

//------------------------------------------------------------------------------//
// Get And Set functions														//
//------------------------------------------------------------------------------//
	/** Returns maximum acceleration for the current state. */
	virtual float GetMaxAcceleration() const override;
	
	/** Returns maximum deceleration for the current state when braking (ie when there is no acceleration). */
	virtual float GetMaxBrakingDeceleration() const override;

	/** Updates another components movement mode information by references */
	virtual void UpdateExternalMovementModeInformation(EMovementMode& MovementModeRef, uint8& CustomMovementModeRef);
	
	/** Returns the player's current input */
	UFUNCTION(BlueprintCallable) virtual FVector GetPlayerInput() const;

	
//------------------------------------------------------------------------------//
// Utility																		//
//------------------------------------------------------------------------------//
public:
	/**
	 * Calculate slide vector along a surface.
	 * Has special treatment when falling, to avoid boosting up slopes (calling HandleSlopeBoosting() in this case).
	 *
	 * @param Delta:	Attempted move.
	 * @param HitTime:		Amount of move to apply (between 0 and 1).
	 * @param Normal:	Normal opposed to movement. Not necessarily equal to Hit.Normal (but usually is).
	 * @param Hit:		HitResult of the move that resulted in the slide.
	 * @return			New deflected vector of movement.
	 */
	virtual FVector ComputeSlideVector(const FVector& Delta, const float HitTime, const FVector& Normal, const FHitResult& Hit) const override;
	
	/** Gets the ability system from the character component */
	// UBaseAbilitySystem* GetAbilitySystem() const;

	/** Logic to do once the in air state has been updated */
	virtual void HandleInAirLogic();
	
	/** Util function for printing debug messages */
	virtual void DebugGroundMovement(FString Message, FColor Color, bool DrawSphere = false);

	/** Prints the input direction as a string for help with sensemaking of gaining momentum during strafing */
	FString GetMovementDirection(const FVector& InputVector) const;

	
};
