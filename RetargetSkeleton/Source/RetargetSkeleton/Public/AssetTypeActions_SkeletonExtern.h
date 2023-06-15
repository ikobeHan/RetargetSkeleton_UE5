// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"
#include "EditorAnimUtils.h"

/**
 * Remap Skeleton Asset Data
 */
struct FAssetToRemapSkeleton
{
	FName					PackageName;
	TWeakObjectPtr<UObject> Asset;
	FText					FailureReason;
	bool					bRemapFailed;

	FAssetToRemapSkeleton(FName InPackageName)
		: PackageName(InPackageName)
		, bRemapFailed(false)
	{}

	// report it failed
	void ReportFailed(const FText& InReason)
	{
		FailureReason = InReason;
		bRemapFailed = true;
	}
};


//class FAssetTypeActions_SkeletonOverride : public FAssetTypeActions_Base
class FAssetTypeActions_SkeletonExtern
{

////////////////////////////// ue4 Rig&BoneMapping //////////////////////////
public:
	bool OnAssetCreated(TArray<UObject*> NewAssets) const;

	/** Handler for when Create Rig is selected */
	void ExecuteCreateRig(TArray<TWeakObjectPtr<USkeleton>> Skeletons);
	void CreateRig(const TWeakObjectPtr<USkeleton> Skeleton);


	/** Handler for when Skeleton Retarget is selected */
	void ExecuteRetargetSkeleton(TArray<TWeakObjectPtr<USkeleton>> Skeletons);
	void PerformRetarget(USkeleton* OldSkeleton, USkeleton* NewSkeleton, TArray<FName> Packages, bool bConvertSpaces) const;

	// utility functions for performing retargeting,these codes are from AssetRenameManager workflow
	void LoadPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& OutPackagesToSave) const;
	bool CheckOutPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& InOutPackagesToSave) const;
	void ReportFailures(const TArray<FAssetToRemapSkeleton>& AssetsToRemap) const;
	void RetargetSkeleton(TArray<FAssetToRemapSkeleton>& AssetsToRemap, USkeleton* OldSkeleton, USkeleton* NewSkeleton, bool bConvertSpaces) const;
	void SavePackages(const TArray<UPackage*> PackagesToSave) const;
	void DetectReadOnlyPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& InOutPackagesToSave) const;
	/** Handler for retargeting */
	void RetargetAnimationHandler(USkeleton* OldSkeleton, USkeleton* NewSkeleton, bool bRemapReferencedAssets, bool bAllowRemapToExisting, bool bConvertSpaces, const EditorAnimUtils::FNameDuplicationRule* NameRule);

	void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName) const;


///////////////////////////////  ue5 ikretargeter  ////////////////////////////
public:
	void ExecuteRetargetSkeleton_IKRetargeter(TArray<TWeakObjectPtr<USkeleton>> Skeletons);

};
