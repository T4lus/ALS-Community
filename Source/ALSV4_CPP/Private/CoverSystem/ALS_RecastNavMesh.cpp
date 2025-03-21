// © 2020 T4lus All Rights Reserved


#include "CoverSystem/ALS_RecastNavMesh.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

DECLARE_LOG_CATEGORY_EXTERN(OurNavMesh, Log, All);
DEFINE_LOG_CATEGORY(OurNavMesh)

AALS_RecastNavMesh::AALS_RecastNavMesh() : Super()
{
}

AALS_RecastNavMesh::AALS_RecastNavMesh(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AALS_RecastNavMesh::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(TileUpdateTimerHandle, this, &AALS_RecastNavMesh::ProcessQueuedTiles, TileBufferInterval, true);
	//TODO: remove? started getting double registrations after the mission selector integration
	UNavigationSystemV1::GetCurrent(GetWorld())->OnNavigationGenerationFinishedDelegate.RemoveDynamic(this, &AALS_RecastNavMesh::OnNavmeshGenerationFinishedHandler);
	UNavigationSystemV1::GetCurrent(GetWorld())->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &AALS_RecastNavMesh::OnNavmeshGenerationFinishedHandler); // avoid a name clash with OnNavMeshGenerationFinished()
}

void AALS_RecastNavMesh::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TileUpdateTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AALS_RecastNavMesh::OnNavMeshTilesUpdated(const TArray<uint32>& ChangedTiles)
{
	Super::OnNavMeshTilesUpdated(ChangedTiles);

	TSet<uint32> updatedTiles;
	for (uint32 changedTile : ChangedTiles)
	{
		updatedTiles.Add(changedTile);
		UpdatedTilesIntervalBuffer.Add(changedTile);
		UpdatedTilesUntilFinishedBuffer.Add(changedTile);
	}

	// fire the immediate delegate
	NavmeshTilesUpdatedImmediateDelegate.Broadcast(updatedTiles);
}

void AALS_RecastNavMesh::ProcessQueuedTiles()
{
	FScopeLock TileUpdateLock(&TileUpdateLockObject);
	if (UpdatedTilesIntervalBuffer.Num() > 0)
	{
		UE_LOG(OurNavMesh, Log, TEXT("OnNavMeshTilesUpdated - tile count: %d"), UpdatedTilesIntervalBuffer.Num());
		NavmeshTilesUpdatedBufferedDelegate.Broadcast(UpdatedTilesIntervalBuffer);
		UpdatedTilesIntervalBuffer.Empty();
	}
}

void AALS_RecastNavMesh::OnNavmeshGenerationFinishedHandler(ANavigationData* NavData)
{
	FScopeLock TileUpdateLock(&TileUpdateLockObject);
	NavmeshTilesUpdatedUntilFinishedDelegate.Broadcast(UpdatedTilesUntilFinishedBuffer);
	UpdatedTilesUntilFinishedBuffer.Empty();
}
