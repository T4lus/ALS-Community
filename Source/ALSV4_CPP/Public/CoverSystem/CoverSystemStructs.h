// © 2020 T4lus All Rights Reserved
#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Math/GenericOctreePublic.h"
#include "Math/GenericOctree.h"
#include "GameFramework/Actor.h"

#include "CoverSystemStructs.generated.h"

/**
 * DTO for FCoverPointOctreeData
 */
struct FDTOCoverData
{
public:
	AActor* CoverObject;
	FVector Location;
	bool bForceField;

	FDTOCoverData() : CoverObject(), Location(), bForceField()
	{
	}

	FDTOCoverData(AActor* _CoverObject, FVector _Location, bool _bForceField) : CoverObject(_CoverObject), Location(_Location), bForceField(_bForceField)
	{
	}
};


struct FCoverPointOctreeData : public TSharedFromThis<FCoverPointOctreeData, ESPMode::ThreadSafe>
{
public:
	// Location of the cover point
	const FVector Location;

	// no leaning if it's a force field wall

	// true if it's a force field, i.e. units can walk through but projectiles are blocked
	const bool bForceField;

	// Object that generated this cover point
	const TWeakObjectPtr<AActor> CoverObject;

	// Whether the cover point is taken by a unit
	bool bTaken = false;

	FCoverPointOctreeData() : Location(), bForceField(false), CoverObject(), bTaken(false)
	{
	}

	FCoverPointOctreeData(FDTOCoverData CoverData) : Location(CoverData.Location), bForceField(CoverData.bForceField), CoverObject(CoverData.CoverObject), bTaken(false)
	{
	}
};

USTRUCT(BlueprintType)
struct FCoverPointOctreeElement
{
	GENERATED_USTRUCT_BODY()

public:
	TSharedRef<FCoverPointOctreeData, ESPMode::ThreadSafe> Data;

	FBoxSphereBounds Bounds;

	FCoverPointOctreeElement() : Data(new FCoverPointOctreeData()), Bounds()
	{
	}

	FCoverPointOctreeElement(FDTOCoverData& CoverData) : Data(new FCoverPointOctreeData(CoverData)), Bounds(FSphere(CoverData.Location, 1.0f))
	{
	}

	FORCEINLINE bool IsEmpty() const
	{
		const FBox boundingBox = Bounds.GetBox();
		return boundingBox.IsValid == 0 || boundingBox.GetSize().IsNearlyZero();
	}

	FORCEINLINE UObject* GetOwner() const
	{
		return Data->CoverObject.Get();
	}
};

struct FCoverPointOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FCoverPointOctreeElement& Element)
	{
		return Element.Bounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FCoverPointOctreeElement& A, const FCoverPointOctreeElement& B)
	{
		//TODO: revisit this when introducing new properties to FCoverPointData
		return A.Bounds == B.Bounds;
	}

	static void SetElementId(const FCoverPointOctreeElement& Element, FOctreeElementId2 ID);
};