// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    


#include "Character/Animation/Notify/ALSAnimNotifyFootstep.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"


#include "Library/ALSStructEnumLibrary.h"

#include "DrawDebugHelpers.h"

void UALSAnimNotifyFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp || !MeshComp->GetAnimInstance())
	{
		return;
	}

	const float MaskCurveValue = MeshComp->GetAnimInstance()->GetCurveValue(FName(TEXT("Mask_FootstepSound")));
	const float FinalVolMult = bOverrideMaskCurve ? VolumeMultiplier : VolumeMultiplier * (1.0f - MaskCurveValue);

	if (FootstepsData)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(MeshComp, EGetWorldErrorMode::LogAndReturnNull);

		FVector StartTrace = MeshComp->GetComponentTransform().GetLocation() + FVector(0.f, 0.f, 90.f);
		FVector EndTrace = StartTrace + FVector(0.0f, 0.0f, -300.0f);
		FCollisionQueryParams TraceParams;
		FHitResult HitResult(ForceInit);

		TraceParams = FCollisionQueryParams::FCollisionQueryParams(false);
		TraceParams.bReturnPhysicalMaterial = true;

		World->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, TraceParams);	
		
		FText CurrentSurface;

		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPhysicalSurface"), true);
		if (EnumPtr)
		{
			CurrentSurface = EnumPtr->GetDisplayNameTextByIndex(UGameplayStatics::GetSurfaceType(HitResult));
		}

		FALSFootsteps* Footsteps = FootstepsData->FindRow<FALSFootsteps>(FName(CurrentSurface.ToString()), TEXT("Footstep Data Surface"), true);
		if (Footsteps)
		{
			if (Footsteps->SoundCue)
			{
				UAudioComponent* SpawnedAudio = UGameplayStatics::SpawnSoundAttached(
					Footsteps->SoundCue, 
					MeshComp, 
					AttachPointName,
					FVector::ZeroVector, 
					FRotator::ZeroRotator,
					EAttachLocation::Type::KeepRelativeOffset,
					true, 
					FinalVolMult, 
					PitchMultiplier
				);
				if (SpawnedAudio)
					SpawnedAudio->SetIntParameter(FName(TEXT("FootstepType")), static_cast<int32>(FootstepType));
			}

			if (Footsteps->NiagaraSystem)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World,
					Footsteps->NiagaraSystem,
					MeshComp->GetBoneLocation(AttachPointName),
					MeshComp->GetComponentRotation(),
					FVector(1.f),
					true,
					true
				);
			}
		}

	}
}

FString UALSAnimNotifyFootstep::GetNotifyName_Implementation() const
{
	FString Name(TEXT("Footstep Type: "));
	Name.Append(GetEnumerationToString(FootstepType));
	return Name;
}
