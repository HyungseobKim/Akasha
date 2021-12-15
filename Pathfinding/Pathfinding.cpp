// Copyright Epic Games, Inc. All Rights Reserved.
#include "Pathfinding.h"

#include "GridUtils.h"
#include "LogPathfinding.h"
#include "Modules/ModuleManager.h"

#include "HexGrid.h"

IMPLEMENT_GAME_MODULE( FDefaultGameModuleImpl, Pathfinding );
DEFINE_LOG_CATEGORY(LogPathfinding);

SearchNode::SearchNode(const FIntPoint& InPosition)
		: position(InPosition)
{}

NodePool::NodePool()
		: Super()
{
	Super::Reserve(Policy::NodePoolSize);
}

SearchNode& NodePool::Add(const SearchNode& searchNode)
{
	const int32 index = TArray<SearchNode>::Add(searchNode);
	NodeMap.Add(searchNode.position, index);
		
	SearchNode& newNode = (*this)[index];
	newNode.searchNodeIndex = index;
	newNode.tileData = HexGrid->GetTileData(newNode.position);
	
	return newNode;
}

SearchNode& NodePool::FindOrAdd(const FIntPoint& position)
{
	const int32* indexPtr = NodeMap.Find(position);
	return indexPtr ? (*this)[*indexPtr] : Add(position);
}

void NodePool::Reset()
{
	Super::Reset(Policy::NodePoolSize);
	NodeMap.Reset();
}

NodeSorter::NodeSorter(const TArray<SearchNode>& InNodePool)
		: NodePool(InNodePool)
{}

bool NodeSorter::operator()(const int32 lhs, const int32 rhs) const
{
	return NodePool[lhs].cost < NodePool[rhs].cost;
}

OpenList::OpenList(TArray<SearchNode>& InNodePool, const NodeSorter& InNodeSorter)
		: nodePool(InNodePool), nodeSorter(InNodeSorter)
{
	Super::Reserve(Policy::OpenSetSize);
}

void OpenList::Push(SearchNode& searchNode)
{
	HeapPush(searchNode.searchNodeIndex, nodeSorter);
	searchNode.bIsOpened = true;
}

int32 OpenList::PopIndex(bool bAllowShrinking)
{
	int32 searchNodeIndex = INDEX_NONE;
	HeapPop(searchNodeIndex, nodeSorter, false);
	nodePool[searchNodeIndex].bIsOpened = false;
	return searchNodeIndex;
}

bool NodeTester::Test_None(const SearchNode&, const SearchNode&, UHexGrid*)
{
	return true;
}

bool NodeTester::Test_Block(const SearchNode& , const SearchNode& neighborNode, UHexGrid*)
{
	return neighborNode.tileData->bBlocked == false;
}

bool NodeTester::Test_Height(const SearchNode& parentNode, const SearchNode& neighborNode, UHexGrid* HexGrid)
{
	return neighborNode.tileData->bBlocked == false
		&& HexGrid->IsPassable(parentNode.position, neighborNode.position);
}

uint8 ElementMask::MapColor(EAkElementType ElementType)
{
	switch (ElementType)
	{
	case EAkElementType::Fire:
		return ElementMask::Fire;

	case EAkElementType::Lightning:
		return ElementMask::Lightning;

	case EAkElementType::Stone:
		return ElementMask::Stone;

	case EAkElementType::Vine:
		return ElementMask::Vine;

	case EAkElementType::Water:
		return ElementMask::Water;

	default:
		return ElementMask::None;
	}
}