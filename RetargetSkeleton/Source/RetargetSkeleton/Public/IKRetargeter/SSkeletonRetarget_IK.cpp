// Copyright Epic Games, Inc. All Rights Reserved.

#include "SSkeletonRetarget_IK.h"

#include "AnimPose.h"
#include "AnimPreviewInstance.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ObjectEditorUtils.h"
#include "SSkeletonWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SSeparator.h"
#include "PropertyCustomizationHelpers.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Misc/ScopedSlowTask.h"
#include "Retargeter/IKRetargeter.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IKRetargetBatchOperation_Copy.h"

#define LOCTEXT_NAMESPACE "SIKRetargetSkel_PoseViewport"



void SIKRetargetSkel_PoseViewport::Construct(const FArguments& InArgs)
{
	SEditorViewport::Construct(SEditorViewport::FArguments());

	PreviewComponent = NewObject<UDebugSkelMeshComponent>();
	PreviewComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	PreviewScene.AddComponent(PreviewComponent, FTransform::Identity);

	SetSkeletalMesh(InArgs._SkeletalMesh);
}

void SIKRetargetSkel_PoseViewport::SetSkeletalMesh(USkeletalMesh* InSkeltalMesh)
{
	if (InSkeltalMesh == Mesh)
	{
		return;
	}

	Mesh = InSkeltalMesh;

	if (Mesh)
	{
		PreviewComponent->SetSkeletalMesh(Mesh);
		PreviewComponent->EnablePreview(true, nullptr);
		// todo add IK retargeter and set it to output the retarget pose
		PreviewComponent->PreviewInstance->SetForceRetargetBasePose(true);
		PreviewComponent->RefreshBoneTransforms(nullptr);

		//Place the camera at a good viewer position
		FBoxSphereBounds Bounds = Mesh->GetBounds();
		Client->FocusViewportOnBox(Bounds.GetBox(), true);
	}
	else
	{
		PreviewComponent->SetSkeletalMesh(nullptr);
	}

	Client->Invalidate();
}

SIKRetargetSkel_PoseViewport::SIKRetargetSkel_PoseViewport()
	: PreviewScene(FPreviewScene::ConstructionValues())
{
}

bool SIKRetargetSkel_PoseViewport::IsVisible() const
{
	return true;
}

TSharedRef<FEditorViewportClient> SIKRetargetSkel_PoseViewport::MakeEditorViewportClient()
{
	TSharedPtr<FEditorViewportClient> EditorViewportClient = MakeShareable(new FSIKRetargetSkel_PoseViewportClient(PreviewScene, SharedThis(this)));

	EditorViewportClient->ViewportType = LVT_Perspective;
	EditorViewportClient->bSetListenerPosition = false;
	EditorViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
	EditorViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);

	EditorViewportClient->SetRealtime(false);
	EditorViewportClient->VisibilityDelegate.BindSP(this, &SIKRetargetSkel_PoseViewport::IsVisible);
	EditorViewportClient->SetViewMode(VMI_Lit);

	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SIKRetargetSkel_PoseViewport::MakeViewportToolbar()
{
	return nullptr;
}










TSharedPtr<SWindow> SSIKRetargetSkel_AnimAssetsWindow::DialogWindow;

void SSIKRetargetSkel_AnimAssetsWindow::Construct(const FArguments& InArgs)
{
	AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(1024));
	BatchContext.bIncludeReferencedAssets = true;

	this->ChildSlot
	[
	SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.AutoWidth()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0, 5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Retarget_SourceTitle", "Source Skeletal Mesh"))
				.Font(FAppStyle::GetFontStyle("Persona.RetargetManager.BoldFont"))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 5)
			[
				SAssignNew(SourceViewport, SIKRetargetSkel_PoseViewport)
				.SkeletalMesh(BatchContext.SourceMesh)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 5)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(USkeletalMesh::StaticClass())
				.AllowClear(true)
				.DisplayUseSelected(true)
				.DisplayBrowse(true)
				.DisplayThumbnail(true)
				.ThumbnailPool(AssetThumbnailPool)
				.IsEnabled_Lambda([this]()
				{
					if (!BatchContext.IKRetargetAsset)
					{
						return false;
					}

					return BatchContext.IKRetargetAsset->GetIKRig(ERetargetSourceOrTarget::Source) != nullptr;
				})
				.ObjectPath(this, &SSIKRetargetSkel_AnimAssetsWindow::GetCurrentSourceMeshPath)
				.OnObjectChanged(this, &SSIKRetargetSkel_AnimAssetsWindow::SourceMeshAssigned)
				.OnShouldFilterAsset_Lambda([this](const FAssetData& AssetData)
					{
						if (!BatchContext.IKRetargetAsset)
						{
							return true;
						}

						USkeletalMesh* Mesh = Cast<USkeletalMesh>(AssetData.GetAsset());
						if (!Mesh)
						{
							return true;
						}
						USkeletalMesh* PreviewMesh = BatchContext.IKRetargetAsset->GetIKRig(ERetargetSourceOrTarget::Source)->GetPreviewMesh();
						if (!PreviewMesh)
						{
							return true;
						}

						return Mesh->GetSkeleton() != PreviewMesh->GetSkeleton();
					})
				]
			]

		+ SHorizontalBox::Slot()
		.Padding(5)
		.AutoWidth()
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		+ SHorizontalBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Retarget_TargetTitle", "Target Skeletal Mesh"))
				.Font(FAppStyle::GetFontStyle("Persona.RetargetManager.BoldFont"))
				.AutoWrapText(true)
			]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5, 5)
				[
					SAssignNew(TargetViewport, SIKRetargetSkel_PoseViewport)
					.SkeletalMesh(nullptr)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5, 5)
				[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(USkeletalMesh::StaticClass())
				.AllowClear(true)
				.DisplayUseSelected(true)
				.DisplayBrowse(true)
				.DisplayThumbnail(true)
				.ThumbnailPool(AssetThumbnailPool)
				.IsEnabled_Lambda([this]()
					{
						if (!BatchContext.IKRetargetAsset)
						{
							return false;
						}
						return BatchContext.IKRetargetAsset->GetIKRig(ERetargetSourceOrTarget::Target) != nullptr;
					})
					.ObjectPath(this, &SSIKRetargetSkel_AnimAssetsWindow::GetCurrentTargetMeshPath)
					.OnObjectChanged(this, &SSIKRetargetSkel_AnimAssetsWindow::TargetMeshAssigned)
					.OnShouldFilterAsset_Lambda([this](const FAssetData& AssetData)
					{
						if (!BatchContext.IKRetargetAsset)
						{
							return true;
						}

						USkeletalMesh* Mesh = Cast<USkeletalMesh>(AssetData.GetAsset());
						if (!Mesh)
						{
							return true;
						}
						USkeletalMesh* PreviewMesh = BatchContext.IKRetargetAsset->GetIKRig(ERetargetSourceOrTarget::Target)->GetPreviewMesh();
						if (!PreviewMesh)
						{
							return true;
						}

						return Mesh->GetSkeleton() != PreviewMesh->GetSkeleton();
					})
				]
			]
		]
	]

	+ SHorizontalBox::Slot()
		.Padding(5)
		.AutoWidth()
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 5)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Retarget_RetargetAsset", "IK Retargeter"))
				.Font(FAppStyle::GetFontStyle("Persona.RetargetManager.BoldFont"))
				.AutoWrapText(true)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(2)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UIKRetargeter::StaticClass())
				.AllowClear(true)
				.DisplayUseSelected(true)
				.DisplayBrowse(true)
				.DisplayThumbnail(true)
				.ThumbnailPool(AssetThumbnailPool)
				.ObjectPath(this, &SSIKRetargetSkel_AnimAssetsWindow::GetCurrentRetargeterPath)
				.OnObjectChanged(this, &SSIKRetargetSkel_AnimAssetsWindow::RetargeterAssigned)
			]

			+ SVerticalBox::Slot()
			.Padding(5)
			.AutoHeight()
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(2)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SSIKRetargetSkel_AnimAssetsWindow::IsRemappingReferencedAssets)
				.OnCheckStateChanged(this, &SSIKRetargetSkel_AnimAssetsWindow::OnRemappingReferencedAssetsChanged)
				[
					SNew(STextBlock).Text(LOCTEXT("DuplicateAndRetarget_AllowRemap", "Remap Referenced Assets"))
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
				.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
				.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton).HAlign(HAlign_Center)
					.Text(LOCTEXT("RetargetOptions_Cancel", "Cancel"))
					.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
					.OnClicked(this, &SSIKRetargetSkel_AnimAssetsWindow::OnCancel)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton).HAlign(HAlign_Center)
					.Text(LOCTEXT("RetargetOptions_Apply", "Retarget"))
					.IsEnabled(this, &SSIKRetargetSkel_AnimAssetsWindow::CanApply)
					.OnClicked(this, &SSIKRetargetSkel_AnimAssetsWindow::OnApply)
					.HAlign(HAlign_Center)
					.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
				]

			]
		]
	];
}

bool SSIKRetargetSkel_AnimAssetsWindow::CanApply() const
{
	return BatchContext.IsValid();
}

FReply SSIKRetargetSkel_AnimAssetsWindow::OnApply()
{
	UpdateTempFolder();
	CloseWindow();
	FIKRetargetBatchOperation_Copy BatchOperation;
	BatchOperation.RunRetarget(BatchContext);
	return FReply::Handled();
}

FReply SSIKRetargetSkel_AnimAssetsWindow::OnCancel()
{
	CloseWindow();
	return FReply::Handled();
}

void SSIKRetargetSkel_AnimAssetsWindow::CloseWindow()
{
	if (DialogWindow.IsValid())
	{
		DialogWindow->RequestDestroyWindow();
	}
}

void SSIKRetargetSkel_AnimAssetsWindow::ShowWindow(USkeleton* InOldSkeleton)
{
	if (DialogWindow.IsValid())
	{
		FSlateApplication::Get().DestroyWindowImmediately(DialogWindow.ToSharedRef());
	}

	DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("IKRetargetSkeletonTitleW", "Retarget Skeletion to Another ( with all reference anim assets )"))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.HasCloseButton(true)
		.IsTopmostWindow(true)
		.SizingRule(ESizingRule::Autosized);

	TSharedPtr<class SSIKRetargetSkel_AnimAssetsWindow> DialogWidget;
	TSharedPtr<SBorder> DialogWrapper =
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(DialogWidget, SSIKRetargetSkel_AnimAssetsWindow)
		];

	DialogWidget->BatchContext.AssetsToRetarget = FilterRelativeAnimAssets(InOldSkeleton);;
	DialogWindow->SetOnWindowClosed(FRequestDestroyWindowOverride::CreateSP(DialogWidget.Get(), &SSIKRetargetSkel_AnimAssetsWindow::OnDialogClosed));
	DialogWindow->SetContent(DialogWrapper.ToSharedRef());

	FSlateApplication::Get().AddWindow(DialogWindow.ToSharedRef());
}

void SSIKRetargetSkel_AnimAssetsWindow::OnDialogClosed(const TSharedRef<SWindow>& Window)
{
	DialogWindow = nullptr;
}

void SSIKRetargetSkel_AnimAssetsWindow::SourceMeshAssigned(const FAssetData& InAssetData)
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(InAssetData.GetAsset());
	BatchContext.SourceMesh = Mesh;
	SourceViewport->SetSkeletalMesh(BatchContext.SourceMesh);
}

void SSIKRetargetSkel_AnimAssetsWindow::TargetMeshAssigned(const FAssetData& InAssetData)
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(InAssetData.GetAsset());
	BatchContext.TargetMesh = Mesh;
	TargetViewport->SetSkeletalMesh(BatchContext.TargetMesh);
}

FString SSIKRetargetSkel_AnimAssetsWindow::GetCurrentSourceMeshPath() const
{
	return BatchContext.SourceMesh ? BatchContext.SourceMesh->GetPathName() : FString("");
}

FString SSIKRetargetSkel_AnimAssetsWindow::GetCurrentTargetMeshPath() const
{
	return BatchContext.TargetMesh ? BatchContext.TargetMesh->GetPathName() : FString("");
}

FString SSIKRetargetSkel_AnimAssetsWindow::GetCurrentRetargeterPath() const
{
	return BatchContext.IKRetargetAsset ? BatchContext.IKRetargetAsset->GetPathName() : FString("");
}

void SSIKRetargetSkel_AnimAssetsWindow::RetargeterAssigned(const FAssetData& InAssetData)
{
	UIKRetargeter* InRetargeter = Cast<UIKRetargeter>(InAssetData.GetAsset());
	BatchContext.IKRetargetAsset = InRetargeter;
	const UIKRigDefinition* SourceIKRig = InRetargeter ? InRetargeter->GetIKRig(ERetargetSourceOrTarget::Source) : nullptr;
	const UIKRigDefinition* TargetIKRig = InRetargeter ? InRetargeter->GetIKRig(ERetargetSourceOrTarget::Target) : nullptr;
	USkeletalMesh* SourceMesh = SourceIKRig ? SourceIKRig->GetPreviewMesh() : nullptr;
	USkeletalMesh* TargetMesh = TargetIKRig ? TargetIKRig->GetPreviewMesh() : nullptr;
	SourceMeshAssigned(FAssetData(SourceMesh));
	TargetMeshAssigned(FAssetData(TargetMesh));
}

ECheckBoxState SSIKRetargetSkel_AnimAssetsWindow::IsRemappingReferencedAssets() const
{
	//return BatchContext.bIncludeReferencedAssets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	return ECheckBoxState::Checked;
}

void SSIKRetargetSkel_AnimAssetsWindow::OnRemappingReferencedAssetsChanged(ECheckBoxState InNewRadioState)
{
	BatchContext.bIncludeReferencedAssets = true; //(InNewRadioState == ECheckBoxState::Checked); //实际上不需要这个值，因为我们本来就会收集骨骼上的所有动画
}


void SSIKRetargetSkel_AnimAssetsWindow::UpdateTempFolder()
{
	//TODO:get current path + /TempRetargetFolder/
	BatchContext.NameRule.FolderPath = "/Game/_TempRetargetFolder_";
}


TArray<TWeakObjectPtr<UObject>> SSIKRetargetSkel_AnimAssetsWindow::FilterRelativeAnimAssets(USkeleton* InSkel)
{
	TArray<FName> Packages;
	TArray<TWeakObjectPtr<UObject>> RemapData;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetReferencers(InSkel->GetOutermost()->GetFName(), Packages);

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	for (FName PackName : Packages)
	{
		const FString PackageName = PackName.ToString();
		// load package
		UPackage* Package = LoadPackage(NULL, *PackageName, LOAD_None);
		if (!Package)
		{
			continue;
		}
		// get all the objects
		TArray<UObject*> Objects;
		GetObjectsWithOuter(Package, Objects);

		// see if we have skeletalmesh
		bool bSkeletalMeshPackage = false;
		for (auto Iter = Objects.CreateIterator(); Iter; ++Iter)
		{
			// we only care animation asset or animation blueprint
			if ((*Iter)->GetClass()->IsChildOf(UAnimationAsset::StaticClass()) ||
				(*Iter)->GetClass()->IsChildOf(UAnimBlueprint::StaticClass()))
			{
				// add to asset 
				RemapData.Add(*Iter);
				break;
			}
			else if ((*Iter)->GetClass()->IsChildOf(USkeletalMesh::StaticClass()))
			{
				break;
			}
		}
	}
	return RemapData;
}

#undef LOCTEXT_NAMESPACE
