// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkPlayerController.h"
#include "UObject/NoExportTypes.h"
#include "PlayerAI.generated.h"

class AAkPlayerController;
class AAkCharacter;
class UAkPlayer;
class UTurnSystem;
class UAStar;
class UMovementRange;
class ATerrain;
class AAkFlag;
class UHexGrid;

/**
 * 
 */
UCLASS()
class AKASHA_API UPlayerAI : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UAkPlayer* InPlayer, UAkPlayer* InHumanPlayer);

	void OnTurnBegan(UAkPlayer* InPlayer);

	void Planning();

private:
	void AnimationEnd();
	void UseAbility(AAkCharacter* Character, UAbility* Ability, FIntPoint TargetPoint) const;
	
	FIntPoint GetPositionToMove() const;
	//void PlanAbility(AAkCharacter* character, FIntPoint position, const FIntPoint& destination);
	void UseShiftAbility();
	
protected:
	UPROPERTY(Transient)
	UAkPlayer* Player;

	UPROPERTY(Transient)
	UAkPlayer* HumanPlayer;
	
private:
	UPROPERTY()
	class AAkPlayerController* PlayerController;
	UPROPERTY()
	class UTurnSystem* TurnSystem;
	UPROPERTY()
	class UAStar* AStar;
	UPROPERTY()
	class UMovementRange* MovementRange;
	UPROPERTY()
	class ATerrain* Terrain;
	UPROPERTY()
	class UHexGrid* HexGrid;

	UPROPERTY()
	TArray<AAkCharacter*> ControlledUnits;

	UPROPERTY()
	class AAkFlag* FlagToProtect;
	UPROPERTY()
	class AAkFlag* FlagToTake;
	
	FAnimEndDel AnimEndCallback;
	int AbilityNum;
	TArray<FIntPoint> path;

	UPROPERTY()
	class AAkCharacter* ControllingCharacter;
	const FTileData* prevTile;
	FIntPoint position;
	FIntPoint destination;
	FIntPoint abilityTarget;
	
	enum class Personality
	{
		TakeFlag,
		CarryingFlag,
		Attack
	};

	TMap<AAkCharacter*, Personality> personalities;
};
