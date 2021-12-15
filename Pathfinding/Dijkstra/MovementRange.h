// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Pathfinding.h"
#include "TileData.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MovementRange.generated.h"

class UHexGrid;

/**
 * 
 */
UCLASS(BlueprintType, DefaultToInstanced)
class PATHFINDING_API UMovementRange : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UHexGrid* InHexGrid);

	/*!
	 * \brief Find movable tiles in this turn from the given position.
	 *
	 * \param position
	 *		  Origin position to start the search.
	 *
	 * \param distance
	 *		  Maximum distance can move from the position.
	 *
	 * \param OutMovablePoints
	 *		  Output container which contains coordinates of reachable tiles.
	 *		  Tiles have the same distance are in the same array.
	 *		  Note: OutMovablePoints[0] = { position } always.
	 */
	void GetMovementRange(const FIntPoint& position, int32 distance, TArray<FIntPoint>& OutMovablePoints, EAkElementType InElementType, bool allowWaterType, bool lightningSpecial, bool allowAnyDestination);

private:
	typedef bool (*NodeBlockTest)(const SearchNode&, const SearchNode&, UHexGrid*);
	typedef bool (*NodeColorTest)(const SearchNode&, EAkElementType ElementType);
	
	void GetTilesInRange(const FIntPoint& position, int32 distance, TArray<FIntPoint>& OutMovablePoints, uint8 destinationColor, uint8 pathColor, NodeBlockTest nodeBlockTest);
	bool ReachableTileExist(const FIntPoint& position, int32 distance, const TArray<FIntPoint>& Tiles, uint8 ElementType, NodeBlockTest nodeBlockTest) const;
	
private:
	UPROPERTY(Transient)
	UHexGrid* HexGrid = nullptr;

	NodePool nodePool;
	NodeSorter nodeSorter = NodeSorter(nodePool);
	OpenList openList = OpenList(nodePool, nodeSorter);

	FIntPoint start;
	bool validStart;
};
