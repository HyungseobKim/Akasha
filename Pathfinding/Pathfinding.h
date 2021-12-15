// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "TileData.h"

class UHexGrid;

using Policy = FGraphAStarDefaultPolicy;

struct SearchNode
{
	SearchNode(const FIntPoint& InPosition);
	
	FIntPoint position;
	FIntPoint parentPos;
	
	int32 searchNodeIndex = INDEX_NONE;
	int32 parentIndex = INDEX_NONE;
	
	int32 cost = INT_MAX;
	int32 totalCost = INT_MAX;

	bool bIsOpened = false;
	bool bIsClosed = false;

	const FTileData* tileData;
};

struct NodePool : TArray<SearchNode>
{
	typedef  TArray<SearchNode> Super;
	TMap<FIntPoint, int32> NodeMap;
	UHexGrid* HexGrid = nullptr;

	NodePool();

	SearchNode& Add(const SearchNode& searchNode);
	SearchNode& FindOrAdd(const FIntPoint& position);

	void Reset();

	// FORCEINLINE void ReinitNodes()
	// {
	// 	for (SearchNode& node : *this)
	// 	{
	// 		new (&node) SearchNode(node.position);
	// 	}
	// }
};

struct NodeSorter
{
	const TArray<SearchNode>& NodePool;

	NodeSorter(const TArray<SearchNode>& InNodePool);
	bool operator()(const int32 lhs, const int32 rhs) const;
};

struct OpenList : TArray<int32>
{
	typedef TArray<int32> Super;
	TArray<SearchNode>& nodePool;
	const NodeSorter nodeSorter;

	OpenList(TArray<SearchNode>& InNodePool, const NodeSorter& InNodeSorter);

	void Push(SearchNode& searchNode);
	int32 PopIndex(bool bAllowShrinking = true);
};

namespace NodeTester
{
	bool Test_None(const SearchNode&, const SearchNode&, UHexGrid*);
	bool Test_Block(const SearchNode& parentNode, const SearchNode& neighborNode, UHexGrid*);
	bool Test_Height(const SearchNode& parentNode, const SearchNode& neighborNode, UHexGrid* HexGrid);
}

namespace ElementMask
{
	constexpr uint8 None = 0;
	constexpr uint8 Stone = 1;
	constexpr uint8 Water = 2;
	constexpr uint8 Vine = 4;
	constexpr uint8 Fire = 8;
	constexpr uint8 Lightning = 16;
	constexpr uint8 Any = Stone | Water | Vine | Fire | Lightning;

	uint8 MapColor(EAkElementType ElementType);
};