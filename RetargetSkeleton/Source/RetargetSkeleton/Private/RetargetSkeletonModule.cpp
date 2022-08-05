// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetSkeletonModule.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_SkeletonExtern.h"
#include "ISkeletonEditorModule.h"
#include "SkeletonRetargetCommands.h"
#include "SkeletonRetargetFactory.h"


#define LOCTEXT_NAMESPACE "FRetargetSkeletonModule"

void FRetargetSkeletonModule::StartupModule()
{
	FSkeletonRetargetCommands::Register();

	// Add Application mode extender-skeleton
	SkeletonRetargetExtender = FWorkflowApplicationModeExtender::CreateLambda([this](const FName ModeName, TSharedRef<FApplicationMode> InMode) {
		if (ModeName == TEXT("SkeletonEditorMode"))
		{
			InMode->AddTabFactory(FCreateWorkflowTabFactory::CreateStatic(&FSkeletonRetargetTabSummoner::CreateFactory));
			RegisteredApplicationModes.Add(InMode);
		}
		return InMode;
	});
	FWorkflowCentricApplication::GetModeExtenderList().Add(SkeletonRetargetExtender);
	//骨架重定向
	ExtendSkeletonBrowserMenu();
}

void FRetargetSkeletonModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FSkeletonRetargetCommands::Unregister();

	// Remove extender delegate
	FWorkflowCentricApplication::GetModeExtenderList().RemoveAll([this](FWorkflowApplicationModeExtender& StoredExtender) { 
		return StoredExtender.GetHandle() == SkeletonRetargetExtender.GetHandle(); 
	});

	// During shutdown clean up all factories from any modes which are still active/alive
	for (TWeakPtr<FApplicationMode> WeakMode : RegisteredApplicationModes)
	{
		if (WeakMode.IsValid())
		{
			TSharedPtr<FApplicationMode> Mode = WeakMode.Pin();
			Mode->RemoveTabFactory(FSkeletonRetargetTabSummoner::RetargetTabName);
		}
	}
	RegisteredApplicationModes.Empty();

}

void FRetargetSkeletonModule::ExtendSkeletonBrowserMenu()
{
	static const TArray<FName> MenusToExtend
	{
		//"ContentBrowser.AssetContextMenu.AnimSequence",
		"ContentBrowser.AssetContextMenu.Skeleton"
	};

	for (const FName& MenuName : MenusToExtend)
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName);
		if (Menu == nullptr)
		{
			continue;
		}

		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
		Section.AddSubMenu(
			"RetargetSkeltonSubmenu",
			LOCTEXT("RetargetSkeltonSubmenu", "Retarget Skeleton to Another(UE4)"),
			LOCTEXT("RetargetSkeltonSubmenu_ToolTip", "Open retargeting Skeleton menu."),
			FNewToolMenuDelegate::CreateLambda([this](UToolMenu* AlignmentMenu)
				{
					FToolMenuSection& InSection = AlignmentMenu->AddSection("SkeletonRetargetMenu", LOCTEXT("SkeletonRetargetHeader", "Retarget Skeleton(UE4)"));
					InSection.AddDynamicEntry("SkeletonRetarget", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
					{
						CreateRetargetSubMenu(InSection);
					}));
				}),
			false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.RetargetManager")
		);
	}
}

void FRetargetSkeletonModule::CreateRetargetSubMenu(FToolMenuSection& InSection)
{

	UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
	if (!Context)
	{
		return;
	}

	TArray<UObject*> SelectedObjects = Context->GetSelectedObjects();
	if (SelectedObjects.IsEmpty())
	{
		return;
	}
	auto Skeletons = GetAssetWeakObjectPtrs<USkeleton>(SelectedObjects);
	if (Skeletons.Num() == 0)
	{
		return;
	}

	InSection.AddMenuEntry(
		"Skeleton_Retarget",
		LOCTEXT("Skeleton_Retarget", "Retarget to Another Skeleton"),
		LOCTEXT("Skeleton_RetargetTooltip", "Allow all animation assets for this skeleton retarget to another skeleton."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Persona.AssetActions.RetargetSkeleton"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FRetargetSkeletonModule::ExecuteRetargetSkeleton, Skeletons),
			FCanExecuteAction()
		)
	);

	InSection.AddMenuEntry(
		"Skeleton_CreateRig",
		LOCTEXT("Skeleton_CreateRig", "Create UE4 Rig"),
		LOCTEXT("Skeleton_CreateRigTooltip", "Create Rig from this skeleton Before RetargetSkeleton in OLD way."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Persona.AssetActions.RetargetSkeleton"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FRetargetSkeletonModule::ExecuteCreateRig, Skeletons),
			FCanExecuteAction()
		)
	);
}

/** Handler for when Create Rig is selected */
void FRetargetSkeletonModule::ExecuteCreateRig(TArray<TWeakObjectPtr<USkeleton>> Skeletons)
{
	FAssetTypeActions_SkeletonExtern SK;
	SK.ExecuteCreateRig(Skeletons);
}

/** Handler for when Skeleton Retarget is selected */
void FRetargetSkeletonModule::ExecuteRetargetSkeleton(TArray<TWeakObjectPtr<USkeleton>> Skeletons)
{
	FAssetTypeActions_SkeletonExtern SK;
	SK.ExecuteRetargetSkeleton(Skeletons);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRetargetSkeletonModule, AnimationUtilTool)