// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerAI.h"

#include <limits>

#include "EngineUtils.h"
#include "Math/UnrealMathUtility.h"

#include "AkCharacter.h"
#include "AkGameMode.h"
#include "AkPlayer.h"

#include "Terrain.h"
#include "AStar.h"
#include "MovementRange.h"
#include "HexGrid.h"
#include "AkFlag.h"
#include "Abilities/Ability.h"


#define print(text) if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, text);
#define printFString(text, fstring) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT(text), fstring));

void UPlayerAI::Initialize(UAkPlayer* InPlayer, UAkPlayer* InHumanPlayer)
{
	Player = InPlayer;
	HumanPlayer = InHumanPlayer;
	
	// Set this player controlled by AI.
	InPlayer->bPlayable = false;
	Player->TurnBeginEvent.AddUObject(this, &UPlayerAI::OnTurnBegan);

	// Get pointers to necessary objects.
	AAkGameMode* GameMode = Cast<AAkGameMode>(GetWorld()->GetAuthGameMode());
	
	Terrain = GameMode->GetTerrain();
	HexGrid = Terrain->Grid;
		
	MovementRange = GameMode->GetMovementRange();
	AStar = GameMode->GetAStar();

	AnimEndCallback = FAnimEndDel::CreateUObject(this, &UPlayerAI::AnimationEnd);
	
	// Set flags.
	const TArray<AAkCharacter*>& characters = Player->ControllableUnits;
	const uint8 team = characters[0]->TeamId;
	
	for (AAkFlag* flag : GameMode->GetFlags())
	{
		if (flag->TeamId == team)
		{
			FlagToProtect = flag;
		}
		else
		{
			FlagToTake = flag;
		}
	}
	destination = FlagToTake->OccupiedTile;
	
	// Set personalities.
	if (GetTypeCanSpread(characters[0]->ElementType) == characters[1]->ElementType)
	{
		personalities.Add(characters[0], Personality::Attack);
		personalities.Add(characters[1], Personality::TakeFlag);
	}
	else
	{
		personalities.Add(characters[0], Personality::TakeFlag);
		personalities.Add(characters[1], Personality::Attack);
	}
}

void UPlayerAI::OnTurnBegan(UAkPlayer* InPlayer)
{
	FTimerHandle TimerHandle;
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UPlayerAI::Planning, 1.f, false, 0);
}

void UPlayerAI::Planning()
{
	// Get character and current position.
	ControllingCharacter = Player->GetControllingUnit();
	position = Terrain->Grid->WorldToGrid(ControllingCharacter->GetActorLocation());

	// Is this turn for bump?
	if (ControllingCharacter->ActiveAbility->AbilityType == EActionType::Bump)
	{
		THexNeighbors Neighbors;
		const int32 NeighborNum = HexGrid->GetNeighbors(position, Neighbors);
		
		AbilityNum = 0;
		for (int32 i = 0; i < NeighborNum; ++i)
		{
			if (HexGrid->GetTileData(Neighbors[i])->TopType == ControllingCharacter->ElementType)
			{
				UseAbility(ControllingCharacter, ControllingCharacter->ActiveAbility, Neighbors[i]);
				return;
			}
		}
		
		UseAbility(ControllingCharacter, ControllingCharacter->ActiveAbility, Neighbors[FMath::RandRange(0, NeighborNum)]);
		return;
	}
	
	if (ControllingCharacter->HasFlag)
	{
		destination = HexGrid->GetEndZoneTileCoords()[Player->TeamId][FMath::RandRange(0, HexGrid->GetEndZoneTileCoords()[Player->TeamId].Num()-1)];
		personalities[ControllingCharacter] = Personality::CarryingFlag;
	}
	
	switch (personalities[ControllingCharacter])
	{
	case Personality::TakeFlag:
		abilityTarget = FlagToTake->OccupiedTile;
		break;

	case Personality::CarryingFlag:
		abilityTarget = destination;
		break;
		
	case Personality::Attack:
		const AAkCharacter* target;

		if (HumanPlayer->ControllableUnits[0]->ElementType == GetTypeCanSpread(ControllingCharacter->ElementType))
		{
			target = HumanPlayer->ControllableUnits[0];
		}
		else
		{
			target = HumanPlayer->ControllableUnits[1];
		}
		
		abilityTarget = HexGrid->WorldToGrid(target->GetActorLocation());
		break;
	}

	bool move = false;
	
	// If there is a tile to move, move.
	const FIntPoint posToMove = GetPositionToMove();
	if (position != posToMove)
	{
		UseAbility(ControllingCharacter, ControllingCharacter->MoveAbility, posToMove);
		position = posToMove;
		move = true;
	}

	// Path to ability target.
	if (AStar->GetShortestPath(position, abilityTarget, path))
	{
		AbilityNum = 3;
		prevTile = nullptr;
	}

	// No movement.
	if (move == false)
	{
		// Start to use shift abilities manually.
		AnimationEnd();
	}
}

void UPlayerAI::AnimationEnd()
{
	if (path.Num() == 0 || AbilityNum <= 0)
	{
		ControllingCharacter->EndTurn();
		return;
	}

	if (HexGrid->GetTileData(abilityTarget)->TopType == ControllingCharacter->ElementType)
	{
		ControllingCharacter->EndTurn();
		return;
	}

	--AbilityNum;
	
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UPlayerAI::UseShiftAbility, 3.f, false, 0);
}

void UPlayerAI::UseAbility(AAkCharacter* Character,UAbility* Ability, FIntPoint TargetPoint) const
{
	Character->SelectAbility(Ability);
	Ability->AnimEndCallback = AnimEndCallback;
	
	TArray<FIntPoint> tiles;

	if (Ability == Character->MoveAbility)
	{
		AStar->GetPath(position, TargetPoint, tiles, Character->ElementType, true, true);		
	}
	else
	{
		tiles.Add(TargetPoint);
	}

	Character->ActiveAbility->SetEffectingTiles(tiles);
	Character->ActiveAbility->Use();
}

FIntPoint UPlayerAI::GetPositionToMove() const
{
	const FIntPoint pos = Terrain->Grid->WorldToGrid(ControllingCharacter->GetActorLocation());
	
	TArray<FIntPoint> MovablePoints;
	MovementRange->GetMovementRange(pos, 4, MovablePoints, ControllingCharacter->ElementType, false, false, false);

	const int32 PointsNum = MovablePoints.Num();
	int32 index = -1;
	int32 minDistance = std::numeric_limits<int32>::max();

	for (int32 i = 0; i < PointsNum; ++i)
	{
		if (HexGrid->GetTileData(MovablePoints[i])->TopType != ControllingCharacter->ElementType)
		{
			continue;
		}
		
		const int32 distance = HexGrid->Distance(MovablePoints[i], destination);
		if (distance < minDistance)
		{
			index = i;
			minDistance = distance;
		}
	}

	if (index >= 0)
	{
		return MovablePoints[index];
	}

	return pos;
}

void UPlayerAI::UseShiftAbility()
{
	while (path.Num() > 0)
	{
		const FTileData* tile = HexGrid->GetTileData(position);

		const FIntPoint& nextPos = path.Last();
		const FTileData* nextTile = HexGrid->GetTileData(nextPos);

		if (tile->TopType == nextTile->TopType)
		{
			if (nextTile->Height > tile->Height + 1)
			{
				UseAbility(ControllingCharacter, ControllingCharacter->Abilities[(uint8)EActionType::LowerTile], nextPos);
				return;
			}
			else if (nextTile->Height < tile->Height - 1)
			{
				UseAbility(ControllingCharacter, ControllingCharacter->Abilities[(uint8)EActionType::RaiseTile], nextPos);
				return;
			}
			else // Passable
			{
				// Don't need to do anything here.
				position = nextPos;
				path.RemoveAt(path.Num()-1);
				prevTile = tile;
			}
		}
		else // Different color.
		{
			if (tile->Height == nextTile->Height)
			{
				UseAbility(ControllingCharacter, ControllingCharacter->Abilities[(uint8)EActionType::ExpandTile], nextPos);
			}
			else if (nextTile->Height > tile->Height)
			{
				// After raising the current tile, it is still movable from the previous tile.
				if (prevTile == nullptr ||
					FMath::Abs(prevTile->Height - (tile->Height + 1)) <= 1)
				{
					UseAbility(ControllingCharacter, ControllingCharacter->Abilities[(uint8)EActionType::RaiseTile], position);
				}
				else // Can't raise the tile anymore.
				{
					// Nothing we can do.
					ControllingCharacter->EndTurn();
				}
			}
			else
			{
				// After lowering the current tile, it is still movable from the previous tile.
				if (prevTile == nullptr ||
					FMath::Abs(prevTile->Height - (tile->Height - 1)) <= 1)
				{
					UseAbility(ControllingCharacter, ControllingCharacter->Abilities[(uint8)EActionType::LowerTile], position);
				}
				else // Can't lower the tile anymore.
				{
					// Nothing we can do.
					ControllingCharacter->EndTurn();
				}
			}

			return;
		}
	}
}
