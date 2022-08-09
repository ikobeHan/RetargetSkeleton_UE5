// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorAnimUtils.h"
#include "SEditorViewport.h"
#include "Settings/SkeletalMeshEditorSettings.h"
#include "PreviewScene.h"
#include "PropertyCustomizationHelpers.h"
#include "IKRigEditor/Public/RetargetEditor/IKRetargetBatchOperation.h"

class UIKRetargeter;


//** Viewport in retarget dialog that shows source / target retarget pose */
class SIKRetargetSkel_PoseViewport : public SEditorViewport
{

public:

	SLATE_BEGIN_ARGS(SIKRetargetSkel_PoseViewport)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, SkeletalMesh)
		SLATE_END_ARGS()

		SIKRetargetSkel_PoseViewport();

	void Construct(const FArguments& InArgs);

	void SetSkeletalMesh(USkeletalMesh* SkeletalMesh);

protected:

	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	/** END SEditorViewport interface */

private:

	USkeletalMesh* Mesh;

	FPreviewScene PreviewScene;

	class UDebugSkelMeshComponent* PreviewComponent;

	virtual bool IsVisible() const override;
};

//** Client for SRetargetPoseViewport */
class FSIKRetargetSkel_PoseViewportClient : public FEditorViewportClient
{

public:

	FSIKRetargetSkel_PoseViewportClient(
		FPreviewScene& InPreviewScene,
		const TSharedRef<SIKRetargetSkel_PoseViewport>& InRetargetPoseViewport)
		: FEditorViewportClient(
			nullptr,
			&InPreviewScene,
			StaticCastSharedRef<SEditorViewport>(InRetargetPoseViewport))
	{
		SetViewMode(VMI_Lit);

		// Always composite editor objects after post processing in the editor
		EngineShowFlags.SetCompositeEditorPrimitives(true);
		EngineShowFlags.DisableAdvancedFeatures();

		UpdateLighting();

		// Setup defaults for the common draw helper.
		DrawHelper.bDrawPivot = false;
		DrawHelper.bDrawWorldBox = false;
		DrawHelper.bDrawKillZ = false;
		DrawHelper.bDrawGrid = true;
		DrawHelper.GridColorAxis = FColor(70, 70, 70);
		DrawHelper.GridColorMajor = FColor(40, 40, 40);
		DrawHelper.GridColorMinor = FColor(20, 20, 20);
		DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;

		bDisableInput = true;
	}

	/** FEditorViewportClient interface */
	virtual void Tick(float DeltaTime) override
	{
		if (PreviewScene)
		{
			PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaTime);
		}
	}


	virtual FSceneInterface* GetScene() const override
	{
		return PreviewScene->GetScene();
	}

	virtual FLinearColor GetBackgroundColor() const override
	{
		return FLinearColor::White;
	}
	virtual void SetViewMode(EViewModeIndex Index) override final
	{
		FEditorViewportClient::SetViewMode(Index);
	}
	/** END FEditorViewportClient interface */

	void UpdateLighting() const
	{
		const USkeletalMeshEditorSettings* Options = GetDefault<USkeletalMeshEditorSettings>();

		PreviewScene->SetLightDirection(Options->AnimPreviewLightingDirection);
		PreviewScene->SetLightColor(Options->AnimPreviewDirectionalColor);
		PreviewScene->SetLightBrightness(Options->AnimPreviewLightBrightness);
	}
};

//** Window to display when configuring batch duplicate & retarget process */
class SSIKRetargetSkel_AnimAssetsWindow : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS(SSIKRetargetSkel_AnimAssetsWindow)
		: _CurrentSkeletalMesh(nullptr)
		, _WidgetWindow(nullptr)
		, _ShowRemapOption(false)
	{
	}

	SLATE_ARGUMENT(USkeletalMesh*, CurrentSkeletalMesh)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_ARGUMENT(bool, ShowRemapOption)
		SLATE_END_ARGS()

		/** Constructs this widget for the window */
		void Construct(const FArguments& InArgs);

private:

	/** Retarget or Cancel buttons */
	bool CanApply() const;
	FReply OnApply();
	FReply OnCancel();
	void CloseWindow();

	/** Handler for dialog window close button */
	void OnDialogClosed(const TSharedRef<SWindow>& Window);

	/** Modifying Source Mesh */
	void SourceMeshAssigned(const FAssetData& InAssetData);
	FString GetCurrentSourceMeshPath() const;

	/** Modifying Target Mesh */
	void TargetMeshAssigned(const FAssetData& InAssetData);
	FString GetCurrentTargetMeshPath() const;

	/** Modifying Retargeter */
	FString GetCurrentRetargeterPath() const;
	void RetargeterAssigned(const FAssetData& InAssetData);

	/** Modifying "Remap Assets" checkbox */
	ECheckBoxState IsRemappingReferencedAssets() const;
	void OnRemappingReferencedAssetsChanged(ECheckBoxState InNewRadioState);
	
	void UpdateTempFolder();

	static TArray<TWeakObjectPtr<UObject>> FilterRelativeAnimAssets(USkeleton* InSkel);

private:
	/** Necessary data collected from UI to run retarget. */
	FIKRetargetBatchOperationContext BatchContext;

	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool;

	/** Viewport displaying SOURCE mesh in retarget pose */
	TSharedPtr<SIKRetargetSkel_PoseViewport> SourceViewport;

	/** Viewport displaying TARGET mesh in retarget pose */
	TSharedPtr<SIKRetargetSkel_PoseViewport> TargetViewport;

public:
	static void ShowWindow(USkeleton* InOldSkeleton);

	static TSharedPtr<SWindow> DialogWindow;
};