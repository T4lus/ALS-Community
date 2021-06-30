// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    


#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Runtime/Engine/Classes/Sound/SoundBase.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Library/ALSCharacterEnumLibrary.h"

#include "ALSAnimNotifyFootstep.generated.h"

/**
 * Character footstep anim notify
 */
UCLASS()
class ALSV4_CPP_API UALSAnimNotifyFootstep : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	virtual FString GetNotifyName_Implementation() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotify)
	UDataTable* FootstepsFXData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	FName FootSocketName = FName(TEXT("Foot_R"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ETraceTypeQuery> TraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceLength = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bSpawnDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bMirrorDecalX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bMirrorDecalY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bMirrorDecalZ = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bSpawnSound = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	FName SoundParameterName = FName(TEXT("FootstepType"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	EALSFootstepType FootstepType = EALSFootstepType::Step;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bOverrideMaskCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (EditCondition = bOverrideMaskCurve))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (EditCondition = bOverrideMaskCurve))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	bool bSpawnNiagara = false;
};
