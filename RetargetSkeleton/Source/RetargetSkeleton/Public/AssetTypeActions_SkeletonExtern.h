// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "EditorAnimUtils.h"

//class FAssetTypeActions_SkeletonOverride : public FAssetTypeActions_Base
class FAssetTypeActions_SkeletonExtern
{

///////////////////////////////  ue5 ikretargeter  ////////////////////////////
public:
	void ExecuteRetargetSkeleton_IKRetargeter(TArray<TWeakObjectPtr<USkeleton>> Skeletons);

};
