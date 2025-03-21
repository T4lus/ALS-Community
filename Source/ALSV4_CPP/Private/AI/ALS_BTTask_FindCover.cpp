// © 2020 T4lus All Rights Reserved


#include "AI/ALS_BTTask_FindCover.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"

FVector UALS_BTTask_FindCover::GetPerpendicularVector(const FVector& Vector)
{
	return FVector(Vector.Y, -Vector.X, Vector.Z);
}

const void UALS_BTTask_FindCover::GetCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, UWorld* World, const FVector& CharacterLocation, const FVector& EnemyLocation) const
{
	const float minAttackRangeSquared = FMath::Square(MinAttackRange);

	// get cover points around the enemy that are inside our attack range
	const FBoxCenterAndExtent coverScanArea = FBoxCenterAndExtent(EnemyLocation, FVector(AttackRange * 0.5f));
	TArray<FCoverPointOctreeElement> coverPoints;
	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(World)->FindCoverPoints(coverPoints, coverScanArea.GetBox());

	// filter out cover points that are too close to the enemy based on our min attack range, or already taken; populate a new array with the remaining, valid cover points only
	for (FCoverPointOctreeElement coverPoint : coverPoints)
		if (!coverPoint.Data->bTaken
			&& FVector::DistSquared(EnemyLocation, coverPoint.Data->Location) >= minAttackRangeSquared)
		{
			OutCoverPoints.Add(coverPoint);
		}

	// sort cover points by their distance to our unit
	OutCoverPoints.Sort([CharacterLocation](const FCoverPointOctreeElement cp1, const FCoverPointOctreeElement cp2) {
		return FVector::DistSquared(CharacterLocation, cp1.Data->Location) < FVector::DistSquared(CharacterLocation, cp2.Data->Location);
	});
}



const bool UALS_BTTask_FindCover::CheckHitByLeaning(const FVector& CoverLocation, const ACharacter* OurUnit, const AActor* TargetEnemy, UWorld* World) const
{
	const FVector enemyLocation = TargetEnemy->GetActorLocation();

	FHitResult hit;
	FCollisionShape sphereColl;
	sphereColl.SetSphere(5.0f);
	FCollisionQueryParams collQueryParamsExclCharacter;
	collQueryParamsExclCharacter.AddIgnoredActor(OurUnit);
	collQueryParamsExclCharacter.TraceTag = "CoverPointFinder_CheckHitByLeaning";

	// check leaning left and right
	for (int directionMultiplier = 1; directionMultiplier > -2; directionMultiplier -= 2)
	{
		// calculate our reach for when leaning out of cover
		const FVector coverEdge = enemyLocation - CoverLocation;
		const FVector coverEdgeDir = coverEdge.GetUnsafeNormal();
		FVector2D coverLean2D = FVector2D(GetPerpendicularVector(coverEdgeDir)) * directionMultiplier;
		coverLean2D *= WeaponLeanOffset;
		const FVector coverLean = FVector(CoverLocation.X + coverLean2D.X, CoverLocation.Y + coverLean2D.Y, CoverLocation.Z);

		// check if we can hit our target by leaning out of cover
		World->SweepSingleByChannel(hit, coverLean, enemyLocation, FQuat::Identity, ECollisionChannel::ECC_Camera, sphereColl, collQueryParamsExclCharacter);
		if (hit.GetActor() == TargetEnemy)
		{
			return true;
		}
	}

	// can't hit the enemy by leaning out of cover
	return false;
}

const bool UALS_BTTask_FindCover::EvaluateCoverPoint(const FCoverPointOctreeElement coverPoint, const ACharacter* Character, const float CharEyeHeight, const AActor* TargetEnemy, const FVector& EnemyLocation, UWorld* World) const
{
	const FVector coverLocation = coverPoint.Data->Location;
	const FVector coverLocationInEyeHeight = FVector(coverLocation.X, coverLocation.Y, coverLocation.Z - CoverPointGroundOffset + CharEyeHeight);

	FHitResult hit;
	FCollisionShape sphereColl;
	sphereColl.SetSphere(5.0f);
	FCollisionQueryParams collQueryParamsExclCharacter;
	collQueryParamsExclCharacter.AddIgnoredActor(Character);
	collQueryParamsExclCharacter.TraceTag = "CoverPointFinder_EvaluateCoverPoint";

	// check if we can hit the enemy straight from the cover point. if we can, then the cover point is no good
	if (!World->SweepSingleByChannel(hit, coverLocationInEyeHeight, EnemyLocation, FQuat::Identity, ECollisionChannel::ECC_Camera, sphereColl, collQueryParamsExclCharacter))
		return false;

	// if the cover point is behind a shield then we shouldn't do any leaning checks, however we must be able to hit the enemy directly and through the shield
	// for this, we will need a second raycast to determine if we're hitting the shield, which has a different collision response than regular objects
	const AActor* hitActor = hit.GetActor();
	if (coverPoint.Data->bForceField // cover is a force field (shield)
		&& hitActor == TargetEnemy) // should be able to hit the enemy directly
	{
		collQueryParamsExclCharacter.TraceTag = "CoverPointFinder_HitShieldFromCover";
		FHitResult shieldHit;
		bool bHitShield = World->SweepSingleByChannel(shieldHit, coverLocationInEyeHeight, EnemyLocation, FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel2, sphereColl, collQueryParamsExclCharacter);
		if (bHitShield // we must hit the shield
			&& shieldHit.Distance <= CoverPointMaxObjectHitDistance) // cover point and cover object must be close to one another
			return true;
	}
	// if the cover point is not behind a shield then check if we can hit the enemy by leaning out of cover
	else if (!coverPoint.Data->bForceField // cover is not a force field (shield)
		&& hit.Distance <= CoverPointMaxObjectHitDistance // cover point and cover object must be close to one another
		&& hitActor != TargetEnemy // shouldn't be able to hit the enemy directly
		&& !hitActor->IsA<APawn>() // can't hide behind other units, for now
		&& CheckHitByLeaning(coverLocationInEyeHeight, Character, TargetEnemy, World)) // we should only be able to hit the enemy by leaning out of cover
		return true;

	return false;
}

EBTNodeResult::Type UALS_BTTask_FindCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// profiling
	SCOPE_CYCLE_COUNTER(STAT_FindCover);
	INC_DWORD_STAT(STAT_FindCoverHistoricalCount);
	SCOPE_SECONDS_ACCUMULATOR(STAT_FindCoverTotalTimeSpent);

	UWorld* world = GetWorld();
	UBlackboardComponent* blackBoardComp = OwnerComp.GetBlackboardComponent();
	const AActor* targetEnemy = Cast<AActor>(blackBoardComp->GetValue<UBlackboardKeyType_Object>(Enemy.SelectedKeyName));
	const ACharacter* character = Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn());
	const FVector characterLocation = character->GetActorLocation();
	const FVector enemyLocation = targetEnemy->GetActorLocation();

	// release the former cover point, if any
	if (blackBoardComp->IsVectorValueSet(OutputVector.SelectedKeyName))
	{
		FVector formerCover = blackBoardComp->GetValueAsVector(OutputVector.SelectedKeyName);

		if (UCoverSystem::bShutdown)
			return EBTNodeResult::Type::Failed;
		UCoverSystem::GetInstance(world)->ReleaseCover(formerCover);
	}

	// get the cover points
	TArray<FCoverPointOctreeElement> coverPoints;
	GetCoverPoints(coverPoints, world, characterLocation, enemyLocation);

	// get navigation data
	const UNavigationSystemV1* navsys = UNavigationSystemV1::GetCurrent(GetWorld());
	const ANavigationData* navdata = navsys->GetDefaultNavDataInstance();

	// calculate the character's standing and crouched eye height offsets
	const float capsuleHalfHeight = character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float charEyeHeightStanding = capsuleHalfHeight + character->BaseEyeHeight;
	const float charEyeHeightCrouched = capsuleHalfHeight + character->CrouchedEyeHeight;

	// find the first adequate cover point
	for (const FCoverPointOctreeElement coverPoint : coverPoints)
	{
		const FVector coverLocation = coverPoint.Data->Location;

		// our unit must be able to reach the cover point
		// TODO: this is a relatively expensive operation, consider implementing an async query instead?
		if (!navsys->TestPathSync(FPathFindingQuery(character, *navdata, characterLocation, coverLocation)))
		{
			continue;
		}

		// check from a standing position and if that fails then from a crouched one
		bool bFoundCover = EvaluateCoverPoint(coverPoint, character, charEyeHeightStanding, targetEnemy, enemyLocation, world)
			|| EvaluateCoverPoint(coverPoint, character, charEyeHeightCrouched, targetEnemy, enemyLocation, world);

		if (bFoundCover)
		{
			// mark the cover point as taken
			if (UCoverSystem::bShutdown)
				return EBTNodeResult::Type::Failed;
			UCoverSystem::GetInstance(world)->HoldCover(coverLocation);

			// set the cover location in the BB
			blackBoardComp->SetValueAsVector(OutputVector.SelectedKeyName, coverLocation);
			return EBTNodeResult::Type::Succeeded;
		}
	}

	// no cover found: unset the cover location in the BB
	blackBoardComp->ClearValue(OutputVector.SelectedKeyName);
	return EBTNodeResult::Type::Failed;
}