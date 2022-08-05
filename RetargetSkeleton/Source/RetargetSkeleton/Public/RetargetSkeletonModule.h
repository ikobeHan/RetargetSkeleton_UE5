// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FToolBarBuilder;
class FMenuBuilder;

class FRetargetSkeletonModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	////////////////////////////////////////////////////////////////////////////////////
	void ExtendSkeletonBrowserMenu();

	void CreateRetargetSubMenu(FToolMenuSection& InSection);

	/** Handler for when Create Rig is selected */
	void ExecuteCreateRig(TArray<TWeakObjectPtr<class USkeleton>> Skeletons);

	/** Handler for when Skeleton Retarget is selected */
	void ExecuteRetargetSkeleton(TArray<TWeakObjectPtr<class USkeleton>> Skeletons);

	template <typename T>
	static TArray<TWeakObjectPtr<T>> GetAssetWeakObjectPtrs(const TArray<UObject*>& InObjects)
	{
		check(InObjects.Num() > 0);

		TArray<TWeakObjectPtr<T>> TypedObjects;
		for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			TypedObjects.Add(CastChecked<T>(*ObjIt));
		}

		return TypedObjects;
	}
private:
	TSharedPtr<class FUICommandList> PluginCommands;

	FString SelectPath;

	int32 Index;

	/** Weak list of application modes for which a tab factory was registered */
	TArray<TWeakPtr<class FApplicationMode>> RegisteredApplicationModes;
	FWorkflowApplicationModeExtender SkeletonRetargetExtender;

};
