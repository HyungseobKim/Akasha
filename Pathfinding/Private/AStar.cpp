// Fill out your copyright notice in the Description page of Project Settings.


#include "AStar.h"
#include "HexGrid.h"

void UAStar::Initialize(UHexGrid* InHexGrid)
{
	HexGrid = InHexGrid;
	nodePool.HexGrid = HexGrid;
}

bool UAStar::GetShortestPath(const FIntPoint& start, const FIntPoint& destination, TArray<FIntPoint>& OutPath)
{
	return AstarSearch(start, destination, OutPath, ElementMask::Any, &NodeTester::Test_None);
}

bool UAStar::GetPath(const FIntPoint& start, const FIntPoint& destination, TArray<FIntPoint>& OutPath, EAkElementType InElementType, bool allowWaterType, bool allowAnyDestination)
{
	const uint8 ElementType = ElementMask::MapColor(InElementType);

	// Check destination.
	if (allowAnyDestination == false)
	{
		// Check destination tile type.
		if (HexGrid->TestTileElement(destination, InElementType) == false &&
			HexGrid->TestTileElement(destination, EAkElementType::Stone) == false &&
			(allowWaterType == false || HexGrid->TestTileElement(destination, EAkElementType::Water) == false))
		{
			return false;
		}
	}

	if (allowWaterType)
	{
		return AstarSearch(start, destination, OutPath, ElementType | ElementMask::Stone | ElementMask::Water, &NodeTester::Test_Height); 
	}
	
	return AstarSearch(start, destination, OutPath, ElementType | ElementMask::Stone, &NodeTester::Test_Height);
}

bool UAStar::AstarSearch(const FIntPoint& start, const FIntPoint& destination, TArray<FIntPoint>& OutPath, uint8 pathColor, NodeBlockTest nodeBlockTest)
{
	SearchKickOff(start, OutPath);

	// Do search.
	while (openList.Num() > 0)
	{
		const int32 currNodeIndex = openList.PopIndex();
		SearchNode& currNodeUnsafe = nodePool[currNodeIndex];
		currNodeUnsafe.bIsClosed = true;

		const FIntPoint& currNodePos = currNodeUnsafe.position;
		
		// We found destination.
		if (currNodePos == destination)
		{
			while (currNodeUnsafe.position != start)
			{
				OutPath.Add(currNodeUnsafe.position);
				currNodeUnsafe = nodePool[currNodeUnsafe.parentIndex];
			}
			
			return true;
		}

		// Color test must be after destination checking to allow different types of destinations.
		if ((ElementMask::MapColor(currNodeUnsafe.tileData->TopType) & pathColor) == 0)
		{
			// Not allowed color.
			continue;
		}

		// Grab neighbors to expand.
		THexNeighbors neighbors;
		const int neighborCount = HexGrid->GetNeighbors(currNodePos, neighbors);

		// Check all neighbors.
		for (int i = 0; i < neighborCount; ++i)
		{
			const FIntPoint& neighborNodePos = neighbors[i];
			SearchNode& neighborNode = nodePool.FindOrAdd(neighborNodePos);

			// If it is starting point, it is guaranteed to be passable.
			if (neighborNodePos != start)
			{
				// If it isn't, do test.
				if ((*nodeBlockTest)(currNodeUnsafe, neighborNode, HexGrid) == false)
				{
					// Blocked tile.
					continue;
				}
			}
			
			const int32 newCost = currNodeUnsafe.cost + HexGrid->GetCost(currNodePos, neighborNodePos);
			const int32 newHeuristic = UHexGrid::Distance(neighborNodePos, destination);
			const int32 newTotalCost = newCost + newHeuristic;

			// If this is not better than previous approach,
			if (newTotalCost >= neighborNode.totalCost)
			{
				// skip.
				continue;
			}

			// Fill in.
			neighborNode.cost = newCost;
			ensure(newCost > 0);
			neighborNode.totalCost = newTotalCost;
			
			neighborNode.parentPos = currNodePos;
			neighborNode.parentIndex = currNodeIndex;
			neighborNode.bIsClosed = false;

			// If this node is not in the open list,
			if (neighborNode.bIsOpened == false)
			{
				// add to the open list.
				openList.Push(neighborNode);
			}
		}
	}

	// No path found.
	return false;
}

void UAStar::SearchKickOff(const FIntPoint& start, TArray<FIntPoint>& OutPath)
{
	// Reset all containers.
	nodePool.Reset();
	openList.Reset();
	OutPath.Reset();

	// Push start node and kick off the search.
	SearchNode& startNode = nodePool.Add(start);
	startNode.cost = 0;
	startNode.totalCost = 0;

	openList.Push(startNode);
}
