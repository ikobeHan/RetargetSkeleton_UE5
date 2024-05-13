/*
* IK Retargeter 骨骼重定向的拷贝，用来实现整个骨架关联动画资源的重定向。
* Author：Hanminglu
*/

#pragma once

#include "CoreMinimal.h"
#include "EditorAnimUtils.h"
#include "IKRigEditor/Public/RetargetEditor/IKRetargetBatchOperation.h"

class UIKRetargeter;

//** Encapsulate ability to batch duplicate and retarget a set of animation assets */
struct FIKRetargetBatchOperation_Copy
{

public:

	/* Actually run the process to duplicate and retarget the assets for the given context */
	void RunRetarget(FIKRetargetBatchOperationContext& Context);


private:
	void Reset();

	/**
	* Initialize set of referenced assets to retarget.
	* @return	Number of assets that need retargeting.
	*/
	int32 GenerateAssetLists(const FIKRetargetBatchOperationContext& Context);

	/* Duplicate all the assets to retarget */
	void DuplicateRetargetAssets(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	/* Retarget skeleton and animation on all the duplicates */
	void RetargetAssets(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	/* Convert animation on all the duplicates */
	void ConvertAnimation(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Copy/remap curves on all the duplicates
	void RemapCurves(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	// Replace existing assets (optional)
	void OverwriteExistingAssets(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);

	/* Output notifications of results */
	void NotifyUserOfResults(const FIKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress) const;

	/* Generate list of newly created assets to report to user */
	void GetNewAssets(TArray<UObject*>& NewAssets) const;

	void GetOldAssets(TArray<UObject*>& OldAssets) const;

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

	/**
	* Duplicates the supplied AssetsToDuplicate and returns a map of original asset to duplicate. Templated wrapper that calls DuplicateAssetInternal.
	*
	* @param	AssetsToDuplicate	The animations to duplicate
	* @param	DestinationPackage	The package that the duplicates should be placed in
	*
	* @return	TMap of original animation to duplicate
	*/
	template<class AssetType>
	static TMap<AssetType*, AssetType*> DuplicateAssets(
		const TArray<AssetType*>& AssetsToDuplicate,
		UPackage* DestinationPackage,
		const EditorAnimUtils::FNameDuplicationRule* NameRule);

	/** Lists of assets to retarget. Populated from selection during init */
	TArray<UAnimationAsset*>	AnimationAssetsToRetarget;
	TArray<UAnimBlueprint*>		AnimBlueprintsToRetarget;

	/** Lists of original assets map to duplicate assets */
	TMap<UAnimationAsset*, UAnimationAsset*>	DuplicatedAnimAssets;
	TMap<UAnimBlueprint*, UAnimBlueprint*>		DuplicatedBlueprints;

	TMap<UAnimationAsset*, UAnimationAsset*>	RemappedAnimAssets;

	/** If we only chose one object to retarget store it here */
	UObject* SingleTargetObject = nullptr;
};