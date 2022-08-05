// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "Toolkits/AssetEditorToolkit.h"

#include "SSkeletonRetarget.h"

/**
 *
 */
#define LOCTEXT_NAMESPACE "SSkeletonRetargetTabFactory"

struct FSkeletonRetargetTabSummoner : public FWorkflowTabFactory
{
public:
	/** Tab ID name */
	static const FName RetargetTabName;

	FSkeletonRetargetTabSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp) :FWorkflowTabFactory(RetargetTabName, InHostingApp)
	{
		TabLabel = LOCTEXT("SkeletonRetargetTitle", "Set Rig & BoneMaping");
		TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.AnimationModifier");
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override
	{
		return LOCTEXT("SkeletonRetargetTitleTabToolTip", "为旧版骨骼重定向设置Rig和骨骼映射配置");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		return SNew(SSkeletonRetargetPanel, HostingApp);
	}

	static TSharedPtr<FWorkflowTabFactory> CreateFactory(TSharedPtr<FAssetEditorToolkit> InAssetEditor)
	{
		return MakeShareable(new FSkeletonRetargetTabSummoner(InAssetEditor));
	}
};

const FName FSkeletonRetargetTabSummoner::RetargetTabName = TEXT("HSeletonRetarget");

#undef LOCTEXT_NAMESPACE