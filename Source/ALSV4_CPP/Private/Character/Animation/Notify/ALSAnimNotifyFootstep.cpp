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
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "Library/ALSStructEnumLibrary.h"

#include "DrawDebugHelpers.h"

void UALSAnimNotifyFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* MeshOwner = MeshComp->GetOwner();
	if (!MeshOwner)
	{
		return;
	}

	if (FootstepsFXData)
	{
		UWorld* World = MeshComp->GetWorld();

		const FVector FootLocation = MeshComp->GetSocketLocation(FootSocketName);
		const FRotator FootRotation = MeshComp->GetSocketRotation(FootSocketName);
		const FVector TraceEnd = FootLocation - MeshOwner->GetActorUpVector() * TraceLength;

		FVector StartTrace = MeshComp->GetComponentTransform().GetLocation() + FVector(0.f, 0.f, 90.f);
		FVector EndTrace = StartTrace + FVector(0.0f, 0.0f, -300.0f);
		FCollisionQueryParams TraceParams;
		FHitResult Hit(ForceInit);

		TraceParams = FCollisionQueryParams::FCollisionQueryParams(false);
		TraceParams.bReturnPhysicalMaterial = true;

		World->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, TraceParams);	
		
		FText CurrentSurface;

		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPhysicalSurface"), true);
		if (EnumPtr)
		{
			CurrentSurface = EnumPtr->GetDisplayNameTextByIndex(UGameplayStatics::GetSurfaceType(Hit));
		}

		FALSFootstepsFX* FootstepsFX = FootstepsFXData->FindRow<FALSFootstepsFX>(FName(CurrentSurface.ToString()), TEXT("Footstep Data Surface"), true);
		if (FootstepsFX)
		{
			if (bSpawnSound && FootstepsFX->SoundCue)
			{
				UAudioComponent* SpawnedSound = nullptr;

				const float MaskCurveValue = MeshComp->GetAnimInstance()->GetCurveValue(FName(TEXT("Mask_FootstepSound")));
				const float FinalVolMult = bOverrideMaskCurve ? VolumeMultiplier : VolumeMultiplier * (1.0f - MaskCurveValue);

				switch (FootstepsFX->SoundSpawnType)
				{
					case EALSSpawnType::Location:
						SpawnedSound = UGameplayStatics::SpawnSoundAtLocation(
							World, 
							FootstepsFX->SoundCue,
							Hit.Location + FootstepsFX->SoundLocationOffset,
							FootstepsFX->SoundRotationOffset,
							FinalVolMult, 
							PitchMultiplier
						);
						break;

					case EALSSpawnType::Attached:
						SpawnedSound = UGameplayStatics::SpawnSoundAttached(
							FootstepsFX->SoundCue,
							MeshComp, 
							FootSocketName,
							FootstepsFX->SoundLocationOffset,
							FootstepsFX->SoundRotationOffset,
							FootstepsFX->SoundAttachmentType,
							true, 
							FinalVolMult,
							PitchMultiplier
						);
						break;
				}
				if (SpawnedSound)
					SpawnedSound->SetIntParameter(FName(TEXT("FootstepType")), static_cast<int32>(FootstepType));
			}

			if (bSpawnNiagara && FootstepsFX->NiagaraSystem)
			{
				UNiagaraComponent* SpawnedParticle = nullptr;
				const FVector Location = Hit.Location + MeshOwner->GetTransform().TransformVector(FootstepsFX->DecalLocationOffset);

				switch (FootstepsFX->NiagaraSpawnType)
				{
				case EALSSpawnType::Location:
					SpawnedParticle = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						World, 
						FootstepsFX->NiagaraSystem,
						Location, 
						FootRotation + FootstepsFX->NiagaraRotationOffset
					);
					break;

				case EALSSpawnType::Attached:
					SpawnedParticle = UNiagaraFunctionLibrary::SpawnSystemAttached(
						FootstepsFX->NiagaraSystem,
						MeshComp, 
						FootSocketName, 
						FootstepsFX->NiagaraLocationOffset,
						FootstepsFX->NiagaraRotationOffset,
						FootstepsFX->NiagaraAttachmentType,
						true
					);
					break;
				}
			}

			if (bSpawnDecal && FootstepsFX->DecalMaterial)
			{
				const FVector Location = Hit.Location + MeshOwner->GetTransform().TransformVector(FootstepsFX->DecalLocationOffset);

				const FVector DecalSize = FVector(
					bMirrorDecalX ? -FootstepsFX->DecalSize.X : FootstepsFX->DecalSize.X,
					bMirrorDecalY ? -FootstepsFX->DecalSize.Y : FootstepsFX->DecalSize.Y,
					bMirrorDecalZ ? -FootstepsFX->DecalSize.Z : FootstepsFX->DecalSize.Z
				);

				UDecalComponent* SpawnedDecal =  UGameplayStatics::SpawnDecalAtLocation(
					World, 
					FootstepsFX->DecalMaterial,
					DecalSize, 
					Location,
					FootRotation + FootstepsFX->DecalRotationOffset,
					FootstepsFX->DecalLifeSpan
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
