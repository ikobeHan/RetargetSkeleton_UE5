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
#include "HAL/FileManager.h"


#define LOCTEXT_NAMESPACE "FRetargetSkeletonModule"

void FRetargetSkeletonModule::StartupModule()
{
	//骨架重定向
	ExtendSkeletonBrowserMenu();
}

void FRetargetSkeletonModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
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
		Section.AddDynamicEntry("SkeletonRetarget", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& Section)
		{
			CreateRetargetSubMenu(Section);
		}));

		/*Section.AddSubMenu(
			"RetargetSkeltonSubmenu",
			LOCTEXT("RetargetSkeltonSubmenu", "Retarget Skeleton to Another"),
			LOCTEXT("RetargetSkeltonSubmenu_ToolTip", "Open retargeting Skeleton menu."),
			FNewToolMenuDelegate::CreateLambda([this](UToolMenu* AlignmentMenu)
			{
				FToolMenuSection& InSection = AlignmentMenu->AddSection("SkeletonRetargetMenu", LOCTEXT("SkeletonRetargetHeader", "Retarget Skeleton"));
				InSection.AddDynamicEntry("SkeletonRetarget", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
				{
					CreateRetargetSubMenu(InSection);
				}));
			}),
			false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.RetargetManager")
		);*/
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
		"Skeleton_Retarget_ue5",
		LOCTEXT("Skeleton_Retarget_ue5", "Retarget to Another Skeleton"),
		LOCTEXT("Skeleton_Retarget_ue5Tooltip", "通过 IK Retargetor 重定向骨架关联的全部动画以及动画蓝图"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.AssetActions.AssignSkeleton"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FRetargetSkeletonModule::ExecuteRetargetSkeleton_Retargeter, Skeletons),
			FCanExecuteAction()
		)
	);
}


/** Handler for when Skeleton Retarget is selected */
void FRetargetSkeletonModule::ExecuteRetargetSkeleton_Retargeter(TArray<TWeakObjectPtr<USkeleton>> Skeletons)
{
	FAssetTypeActions_SkeletonExtern SK;
	SK.ExecuteRetargetSkeleton_IKRetargeter(Skeletons);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRetargetSkeletonModule, AnimationUtilTool)