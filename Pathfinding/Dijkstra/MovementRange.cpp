// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementRange.h"
#include "HexGrid.h"

void UMovementRange::Initialize(UHexGrid* InHexGrid)
{
	HexGrid = InHexGrid;
	nodePool.HexGrid = HexGrid;
}

void UMovementRange::GetMovementRange(const FIntPoint& position, int32 distance, TArray<FIntPoint>& OutMovablePoints, EAkElementType InElementType, bool allowWaterType, bool lightningSpecial, bool allowAnyDestination)
{
	const uint8 ElementType = ElementMask::MapColor(InElementType);
	start = position;

	int destinationColor = ElementType;
	int pathColor = ElementType | ElementMask::Stone;
	
	if (lightningSpecial)
	{
		destinationColor |= ElementMask::Stone;
	}

	if (allowWaterType)
	{
		destinationColor |= ElementMask::Water;
		pathColor |= ElementMask::Water;
	}

	if (allowAnyDestination)
	{
		GetTilesInRange(position, distance, OutMovablePoints, ElementMask::Any, pathColor, &NodeTester::Test_Height);
		return;
	}

	GetTilesInRange(position, distance, OutMovablePoints, destinationColor, pathColor, &NodeTester::Test_Height);
	
	for (int i = 0; i < OutMovablePoints.Num(); ++i)
	{
		const FIntPoint& pos = OutMovablePoints[i];
		
		if (ReachableTileExist(pos, distance - nodePool.FindOrAdd(pos).cost, OutMovablePoints, destinationColor, &NodeTester::Test_Height) == false)
		{
			OutMovablePoints.RemoveAt(i);
			--i;
		}
	}
}

void UMovementRange::GetTilesInRange(const FIntPoint& position, int32 distance, TArray<FIntPoint>& OutMovablePoints, uint8 destinationColor, uint8 pathColor, NodeBlockTest nodeBlockTest)
{
	// Reset all containers.
	nodePool.Reset();
	openList.Reset();
	OutMovablePoints.Reset();

	// Push start node and kick off the search.
	SearchNode& startNode = nodePool.Add(position);
	startNode.cost = 0;
	
	openList.Push(startNode);

	// Do search.
	while (openList.Num() > 0)
	{
		const int32 currNodeIndex = openList.PopIndex();
		SearchNode& currNodeUnsafe = nodePool[currNodeIndex];
		currNodeUnsafe.bIsClosed = true;

		// Minimum cost node is not reachable.
		if (currNodeUnsafe.cost > distance)
		{
			// Done.
			return;
		}

		const FIntPoint& currNodePos = currNodeUnsafe.position;

		// Don't need to check initial node.
		if (currNodePos != position)
		{
			// It is destination node.
			if (currNodeUnsafe.cost == distance)
			{
				if ((ElementMask::MapColor(currNodeUnsafe.tileData->TopType) & destinationColor) == 0)
				{
					// Not valid color.
					continue;
				}
			}
			else if ((ElementMask::MapColor(currNodeUnsafe.tileData->TopType) & pathColor) == 0) 
			{
				continue;
			}
		}

		// This node is reachable. Store it.
		OutMovablePoints.Push(currNodePos);

		// Grab neighbors to expand.
		THexNeighbors neighbors;
		const int neighborCount = HexGrid->GetNeighbors(currNodePos, neighbors);

		// Check all neighbors.
		for (int i = 0; i < neighborCount; ++i)
		{
			const FIntPoint& neighborNodePos = neighbors[i];
			SearchNode& neighborNode = nodePool.FindOrAdd(neighborNodePos);

			// If it is starting point, it is guaranteed to be passable.
			if (neighborNodePos != position)
			{
				// If it isn't, do test.
				if ((*nodeBlockTest)(currNodeUnsafe, neighborNode, HexGrid) == false)
				{
					// Blocked.
					continue;
				}
			}
			
			const int32 newCost = currNodeUnsafe.cost + HexGrid->GetCost(currNodePos, neighborNodePos);

			// If this is not better than previous approach,
			if (newCost >= neighborNode.cost)
			{
				// skip.
				continue;
			}

			// Fill in.
			neighborNode.cost = newCost;
			ensure(newCost > 0);
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
}

bool UMovementRange::ReachableTileExist(const FIntPoint& position, int32 distance, const TArray<FIntPoint>& Tiles, uint8 ElementType, NodeBlockTest nodeBlockTest) const
{
	NodePool pool;
	pool.HexGrid = HexGrid;
	
	NodeSorter sorter = NodeSorter(pool);
	OpenList list = OpenList(pool, sorter);
	
	// Push start node and kick off the search.
	SearchNode& startNode = pool.Add(position);
	startNode.cost = 0;
	
	list.Push(startNode);

	// Do search.
	while (list.Num() > 0)
	{
		const int32 currNodeIndex = list.PopIndex();
		SearchNode& currNodeUnsafe = pool[currNodeIndex];
		currNodeUnsafe.bIsClosed = true;

		// Node having minimum cost is not reachable.
		if (currNodeUnsafe.cost > distance)
		{
			// Done.
			return false;
		}

		if (ElementType & ElementMask::MapColor(currNodeUnsafe.tileData->TopType))
		{
			return true;
		}
		
		const FIntPoint& currNodePos = currNodeUnsafe.position;

		// Grab neighbors to expand.
		THexNeighbors neighbors;
		const int neighborCount = HexGrid->GetNeighbors(currNodePos, neighbors);

		// Check all neighbors.
		for (int i = 0; i < neighborCount; ++i)
		{
			const FIntPoint& neighborNodePos = neighbors[i];
			SearchNode& neighborNode = pool.FindOrAdd(neighborNodePos);

			// If it is starting point, it is guaranteed to be passable.
			if (neighborNodePos != start)
			{
				// If it isn't, do test.
				if ((*nodeBlockTest)(currNodeUnsafe, neighborNode, HexGrid) == false)
				{
					// Blocked.
					continue;
				}	
			}
						
			const int32 newCost = currNodeUnsafe.cost + HexGrid->GetCost(currNodePos, neighborNodePos);

			// If this is not better than previous approach,
			if (newCost >= neighborNode.cost)
			{
				// skip.
				continue;
			}

			// Fill in.
			neighborNode.cost = newCost;
			ensure(newCost > 0);
			neighborNode.parentPos = currNodePos;
			neighborNode.parentIndex = currNodeIndex;
			neighborNode.bIsClosed = false;

			// If this node is not in the open list,
			if (neighborNode.bIsOpened == false)
			{
				// add to the open list.
				list.Push(neighborNode);
			}
		}
	}

	return false;
}
