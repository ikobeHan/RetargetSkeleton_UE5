/*
* IK Retargeter 骨骼重定向的拷贝，用来实现整个骨架关联动画资源的重定向。
* Author：Hanminglu
*/

#pragma once

#include "CoreMinimal.h"
#include "EditorAnimUtils.h"
#include "IKRigEditor/Public/RetargetEditor/IKRetargetBatchOperation.h"

#include "IKRetargetBatchOperation_Copy.generated.h"

class UIKRetargeter;
struct FScopedSlowTask;
class USkeletalMesh;
class UAnimationAsset;
class UAnimBlueprint;

//** Encapsulate ability to batch duplicate and retarget a set of animation assets */
UCLASS(BlueprintType)
class UIKRetargetBatchOperation_Copy : public UObject
{

GENERATED_BODY()
	
public:

	/* Convenience function to run a batch animation retarget from Blueprint / Python. This function will duplicate a list of
	 * assets and use the supplied IK Retargeter to retarget the animation from the source to the target using the
	 * settings in the provided IK Retargeter asset.
	 * 
	 * @param AssetsToRetarget A list of animation assets to retarget (sequences, blendspaces or montages)
	 * @param SourceMesh The skeletal mesh with desired proportions to playback the assets to retarget
	 * @param TargetMesh The skeletal mesh to retarget the animation onto.
	 * @param IKRetargetAsset The IK Retargeter asset with IK Rigs appropriate for the source and target skeletal mesh
	 * @param Search A string to search for in the file name (replaced with "Replace" string)
	 * @param Replace A string to replace with in the file name
	 * @param Suffix A string to add at the end of the new file name
	 * @param Prefix A string to add to the start of the new file name
	 * @param bRemapReferencedAssets Whether to remap existing references in the animation assets
	 * 
	 * Return: list of new animation files created.*/
	UFUNCTION(BlueprintCallable, Category=IKBatchRetarget)
	static TArray<FAssetData> DuplicateAndRetarget(
		const TArray<FAssetData>& AssetsToRetarget,
		USkeletalMesh* SourceMesh,
		USkeletalMesh* TargetMesh,
		UIKRetargeter* IKRetargetAsset,
		const FString& Search = "",
		const FString& Replace = "",
		const FString& Prefix = "",
		const FString& Suffix = "",
		const bool bIncludeReferencedAssets=true);
	
	// Actually run the process to duplicate and retarget the assets for the given context
	void RunRetarget(FIKRetargetBatchOperationContext& Context);

private:

	void Reset();

	// Initialize set of referenced assets to retarget.
	// @return	Number of assets that need retargeting. 
	int32 GenerateAssetLists(const FIKRetargetBatchOperationContext& Context);

	// Duplicate all the assets to retarget
	void DuplicateRetargetAssets(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Retarget skeleton and animation on all the duplicates
	void RetargetAssets(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Convert animation on all the duplicates
	void ConvertAnimation(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Copy/remap curves on all the duplicates
	void RemapCurves(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Replace existing assets (optional)
	void OverwriteExistingAssets(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Output notifications of results
	void NotifyUserOfResults(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress) const;

	// Generate list of newly created assets to report to user
	void GetNewAssets(TArray<UObject*>& NewAssets) const;

	// If user cancelled half way, cleanup all the duplicated assets
	void CleanupIfCancelled(const FScopedSlowTask& Progress) const;
	
	// Lists of assets to retarget. Populated from selection during init
	TArray<UAnimationAsset*>	AnimationAssetsToRetarget;
	TArray<UAnimBlueprint*>		AnimBlueprintsToRetarget;

	// Lists of original assets map to duplicate assets
	TMap<UAnimationAsset*, UAnimationAsset*>	DuplicatedAnimAssets;
	TMap<UAnimBlueprint*, UAnimBlueprint*>		DuplicatedBlueprints;

	TMap<UAnimationAsset*, UAnimationAsset*>	RemappedAnimAssets;

	template<class T>
	void ExchangeMapKeyValue(TMap<T*, T*>& InOutMap) const
	{
		for (auto& pairCom : InOutMap)
		{
			auto* v = pairCom.Value;
			auto* k = pairCom.Key;
			pairCom.Value = k;
			pairCom.Key = v;
		}
	};
};