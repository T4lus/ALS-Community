// © 2020 T4lus All Rights Reserved


#include "ALS_GameModeBase.h"
#include "CoverSystem/CoverSystem.h"
#include "CoverSystem/CoverOctree.h"
#include "CoverSystem/CoverSystemStructs.h"
#include "CoverSystem/ALS_RecastNavMesh.h"

#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif

AALS_GameModeBase::AALS_GameModeBase() : Super()
{
	//FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ACoverDemoGameModeBase::OnWorldInitDelegate);
}

void AALS_GameModeBase::PostLoad()
{
	Super::PostLoad();
}

void AALS_GameModeBase::PostActorCreated()
{
	Super::PostActorCreated();

}

void AALS_GameModeBase::OnWorldInitDelegate(UWorld* World, const UWorld::InitializationValues IVS)
{
}

void AALS_GameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AALS_GameModeBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AALS_GameModeBase::BeginPlay()
{
	Super::BeginPlay();

	CoverSystem->bShutdown = false;
	CoverSystem = UCoverSystem::GetInstance(GetWorld()); // see the CoverSystem variable for why this is here
	CoverSystem->MapBounds = MapBounds;

#if DEBUG_RENDERING
	CoverSystem->bDebugDraw = bDebugDraw;
#endif
}

void AALS_GameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CoverSystem->bShutdown = true;
	CoverSystem = nullptr;
}


void AALS_GameModeBase::DebugShowCoverPoints()
{
#if DEBUG_RENDERING
	UWorld* world = GetWorld();
	FlushPersistentDebugLines(world);
	TArray<FCoverPointOctreeElement> coverPoints;

	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(world)->FindCoverPoints(coverPoints, FBoxCenterAndExtent(FVector::ZeroVector, FVector(64000.0f)).GetBox());

	for (FCoverPointOctreeElement cp : coverPoints)
		DrawDebugSphere(world, cp.Data->Location, 25, 4, cp.Data->bTaken ? FColor::Red : cp.Data->bForceField ? FColor::Orange : FColor::Blue, true, -1, 0, 5);
#endif
}

void AALS_GameModeBase::ForceGC()
{
#if DEBUG_RENDERING
	GEngine->ForceGarbageCollection(true);
#endif
}