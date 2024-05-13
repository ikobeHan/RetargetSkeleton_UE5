// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_SkeletonExtern.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimBlueprint.h"
#include "PhysicsEngine/PhysicsAsset.h"
#if WITH_EDITOR
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#endif

#include "AnimationBlueprintLibrary.h"
#include "IKRetargeter/SSkeletonRetarget_IK.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_Override"


void FAssetTypeActions_SkeletonExtern::ExecuteRetargetSkeleton_IKRetargeter(TArray<TWeakObjectPtr<USkeleton>> Skeletons)
{
	TArray<UObject*> AllEditedAssets;
#if WITH_EDITOR
	AllEditedAssets = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetAllEditedAssets();
#endif

	for (auto SkelIt = Skeletons.CreateConstIterator(); SkelIt; ++SkelIt)
	{
		USkeleton* OldSkeleton = (*SkelIt).Get();

		// Close any assets that reference this skeleton
		for (UObject* EditedAsset : AllEditedAssets)
		{
			bool bCloseAssetEditor = false;
			if (USkeleton* Skeleton = Cast<USkeleton>(EditedAsset))
			{
				bCloseAssetEditor = OldSkeleton == Skeleton;
			}
			else if (UAnimationAsset* AnimationAsset = Cast<UAnimationAsset>(EditedAsset))
			{
				bCloseAssetEditor = OldSkeleton == AnimationAsset->GetSkeleton();
			}
			else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedAsset))
			{
				bCloseAssetEditor = OldSkeleton == SkeletalMesh->GetSkeleton();
			}
			else if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(EditedAsset))
			{
				bCloseAssetEditor = OldSkeleton == AnimBlueprint->TargetSkeleton;
			}
			else if (UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(EditedAsset))
			{
				USkeletalMesh* PhysicsSkeletalMesh = PhysicsAsset->PreviewSkeletalMesh.LoadSynchronous();
				if (PhysicsSkeletalMesh != nullptr)
				{
					bCloseAssetEditor = OldSkeleton == PhysicsSkeletalMesh->GetSkeleton();
				}
			}

			if (bCloseAssetEditor)
			{
#if WITH_EDITOR
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(EditedAsset);
#endif
			}
		}

		SSIKRetargetSkel_AnimAssetsWindow::ShowWindow(OldSkeleton);
	}
}







#undef LOCTEXT_NAMESPACE
