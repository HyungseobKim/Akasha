// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TileData.h"
#include "Pathfinding.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AStar.generated.h"

class UHexGrid;

/**
 * 
 */
UCLASS()
class PATHFINDING_API UAStar : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UHexGrid* InHexGrid);

	bool GetShortestPath(const FIntPoint& start, const FIntPoint& destination, TArray<FIntPoint>& OutPath);
	bool GetPath(const FIntPoint& start, const FIntPoint& destination, TArray<FIntPoint>& OutPath, EAkElementType InElementType, bool allowWaterType, bool allowAnyDestination);
	
private:
	typedef bool (*NodeBlockTest)(const SearchNode&, const SearchNode&, UHexGrid*);
	
	/*!
	* \brief Find a path from the given position to the destination.
	*
	* \param start
	*		 Origin position to start the search.
	*
	* \param destination
	*		 Goal of the path.
	*
	* \param OutPath
	*		 All points for the path includes the destination.
	*		 Start position is excluded.
	*	
	* \return bool
	*		  If a path is found, return true.
	*		  If there is no path, return false.
	*/
	bool AstarSearch(const FIntPoint& start, const FIntPoint& destination, TArray<FIntPoint>& OutPath, uint8 pathColor, NodeBlockTest nodeBlockTest);
	
	void SearchKickOff(const FIntPoint& start, TArray<FIntPoint>& OutPath);
	
private:
	UPROPERTY(Transient)
	UHexGrid* HexGrid = nullptr;

	NodePool nodePool;
	NodeSorter nodeSorter = NodeSorter(nodePool);
	OpenList openList = OpenList(nodePool, nodeSorter);
};

