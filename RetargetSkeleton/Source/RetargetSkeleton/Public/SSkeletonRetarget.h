// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "EditorStyleSet.h"
#include "AssetData.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "PreviewScene.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "EditorAnimUtils.h"
#include "SNewBoneMapping.h"


class UAnimSet;
class USkeletalMesh;
class IEditableSkeleton;
class URig;
class USkeleton;
struct FAssetData;
class SRigWindow;

using namespace EditorAnimUtils;

#define LOCTEXT_NAMESPACE "SkeletonRetargetWidget" 


//////////////////////////
class SReferPoseViewport : public /*SCompoundWidget*/SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SReferPoseViewport)
	{}

	SLATE_ARGUMENT(USkeleton*, Skeleton)
		SLATE_END_ARGS()

public:
	SReferPoseViewport();

	void Construct(const FArguments& InArgs);
	void SetSkeleton(USkeleton* Skeleton);

protected:
	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

private:
	/** Skeleton */
	USkeleton* TargetSkeleton;

	FPreviewScene PreviewScene;

	class UDebugSkelMeshComponent* PreviewComponent;

	bool IsVisible() const override;
};

/////////////////////////////////////////////
/**
 * Slate panel for choose Skeleton for assets to relink
 */
DECLARE_DELEGATE_SixParams(FOnRetargetAnimation, USkeleton* /*OldSkeleton*/, USkeleton* /*NewSkeleton*/, bool /*bRemapReferencedAssets*/, bool /*bAllowRemapToExisting*/, bool /*bConvertSpaces*/, const FNameDuplicationRule* /*NameRule*/)

class SAnimationRemapSkeleton : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAnimationRemapSkeleton)
		: _CurrentSkeleton(NULL)
		, _WidgetWindow(NULL)
		, _WarningMessage()
		, _ShowRemapOption(false)
		, _ShowExistingRemapOption(false)
	{}

	/** The anim sequences to compress */
	SLATE_ARGUMENT(USkeleton*, CurrentSkeleton)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_ARGUMENT(FText, WarningMessage)
		SLATE_ARGUMENT(bool, ShowRemapOption)
		SLATE_ARGUMENT(bool, ShowExistingRemapOption)
		SLATE_ARGUMENT(bool, ShowConvertSpacesOption)
		SLATE_ARGUMENT(bool, ShowCompatibleDisplayOption)
		SLATE_ARGUMENT(bool, ShowDuplicateAssetOption)
		SLATE_EVENT(FOnRetargetAnimation, OnRetargetDelegate)
		SLATE_END_ARGS()

		/**
		 * Constructs this widget
		 *
		 * @param	InArgs	The declaration data for this widget
		 */
		void Construct(const FArguments& InArgs);

	/**
	 * Old Skeleton that was mapped
	 * This data is needed to prevent users from selecting same skeleton
	 */
	USkeleton* OldSkeleton;
	/**
	 * New Skeleton that they would like to map to
	 */
	USkeleton* NewSkeleton;

private:
	/**
	 * Whether we are remapping assets that are referenced by the assets the user selects to remap
	 */
	bool bRemapReferencedAssets;

	/**
	 * Whether we allow remapping to existing assets for the new skeleton
	 */
	bool bAllowRemappingToExistingAssets;

	/**
	 * Whether we are remapping assets that are referenced by the assets the user selects to remap
	 */
	bool bConvertSpaces;

	/**
	 * Whether to show skeletons with the same rig set up
	*/
	bool bShowOnlyCompatibleSkeletons;

	TSharedPtr<SReferPoseViewport> SourceViewport;
	TSharedPtr<SReferPoseViewport> TargetViewport;

	TSharedPtr<SBox> AssetPickerBox;

	TWeakPtr<SWindow> WidgetWindow;

	FOnRetargetAnimation OnRetargetAnimationDelegate;

	/** Handlers for check box for remapping assets option */
	ECheckBoxState IsRemappingReferencedAssets() const;
	void OnRemappingReferencedAssetsChanged(ECheckBoxState InNewRadioState);

	/** Handlers for check box for remapping to existing assets */
	ECheckBoxState IsRemappingToExistingAssetsChecked() const;
	bool IsRemappingToExistingAssetsEnabled() const;
	void OnRemappingToExistingAssetsChanged(ECheckBoxState InNewRadioState);

	/** Handlers for check box for converting spaces*/
	ECheckBoxState IsConvertSpacesChecked() const;
	void OnConvertSpacesCheckChanged(ECheckBoxState InNewRadioState);

	/** Handlers for check box for converting spaces*/
	ECheckBoxState IsShowOnlyCompatibleSkeletonsChecked() const;
	bool IsShowOnlyCompatibleSkeletonsEnabled() const;
	void OnShowOnlyCompatibleSkeletonsCheckChanged(ECheckBoxState InNewRadioState);

	/** should filter asset */
	bool OnShouldFilterAsset(const struct FAssetData& AssetData);

	/**
	 * return true if it can apply
	 */
	bool CanApply() const;

	FReply OnApply();
	FReply OnCancel();

	void CloseWindow();

	/** Handler for dialog window close button */
	void OnRemapDialogClosed(const TSharedRef<SWindow>& Window);

	/**
	 * Handler for when asset is selected
	 */
	void OnAssetSelectedFromPicker(const FAssetData& AssetData);

	/*
	* Refreshes asset picker - call when asset picker option changes
	*/
	void UpdateAssetPicker();

	/*
	 * Duplicate Name Rule
	 */
	FNameDuplicationRule NameDuplicateRule;
	FText ExampleText;

	FText GetPrefixName() const
	{
		return FText::FromString(NameDuplicateRule.Prefix);
	}

	void SetPrefixName(const FText& InText)
	{
		NameDuplicateRule.Prefix = InText.ToString();
		UpdateExampleText();
	}

	FText GetSuffixName() const
	{
		return FText::FromString(NameDuplicateRule.Suffix);
	}

	void SetSuffixName(const FText& InText)
	{
		NameDuplicateRule.Suffix = InText.ToString();
		UpdateExampleText();
	}

	FText GetReplaceFrom() const
	{
		return FText::FromString(NameDuplicateRule.ReplaceFrom);
	}

	void SetReplaceFrom(const FText& InText)
	{
		NameDuplicateRule.ReplaceFrom = InText.ToString();
		UpdateExampleText();
	}

	FText GetReplaceTo() const
	{
		return FText::FromString(NameDuplicateRule.ReplaceTo);
	}

	void SetReplaceTo(const FText& InText)
	{
		NameDuplicateRule.ReplaceTo = InText.ToString();
		UpdateExampleText();
	}

	FText GetExampleText() const
	{
		return ExampleText;
	}

	void UpdateExampleText();

	FReply ShowFolderOption();
	FText GetFolderPath() const
	{
		return FText::FromString(NameDuplicateRule.FolderPath);
	}

public:

	/**
	 *  Show window
	 *
	 * @param OldSkeleton		Old Skeleton to change from
	 *
	 * @return true if successfully selected new skeleton
	 */
	static void ShowWindow(USkeleton* OldSkeleton, const FText& WarningMessage, bool bDuplicateAssets, FOnRetargetAnimation RetargetDelegate);

	static TSharedPtr<SWindow> DialogWindow;

	bool bShowDuplicateAssetOption;
};


//////////////////////////////////////////////////////////////////////////
class FRetargetSkeletonEntryInfo
{
public:
	USkeleton* NewSkeleton;
	UObject* AnimAsset;
	UObject* RemapAsset;

	static TSharedRef<FRetargetSkeletonEntryInfo> Make(UObject* InAsset, USkeleton* InNewSkeleton);

protected:

	FRetargetSkeletonEntryInfo() {};
	FRetargetSkeletonEntryInfo(UObject* InAsset, USkeleton* InNewSkeleton);
};

class SRetargetAssetEntryRow : public SMultiColumnTableRow<TSharedPtr<FRetargetSkeletonEntryInfo>>
{
public:
	SLATE_BEGIN_ARGS(SRetargetAssetEntryRow)
	{}

	SLATE_ARGUMENT(TSharedPtr<FRetargetSkeletonEntryInfo>, DisplayedInfo)

		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

protected:

	TSharedRef<SWidget> GetRemapMenuContent();
	FText GetRemapMenuButtonText() const;
	void OnAssetSelected(const FAssetData& AssetData);
	bool OnShouldFilterAsset(const FAssetData& AssetData) const;

	TSharedPtr<FRetargetSkeletonEntryInfo> DisplayedInfo;

	TWeakObjectPtr<UObject> RemapAsset;

	FString SkeletonExportName;
};

typedef SListView<TSharedPtr<FRetargetSkeletonEntryInfo>> SRemapAssetEntryList;

class SAnimationRemapAssets : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAnimationRemapAssets)
		: _NewSkeleton(nullptr)
		, _RetargetContext(nullptr)
	{}

	SLATE_ARGUMENT(USkeleton*, NewSkeleton)
		SLATE_ARGUMENT(FAnimationRetargetContext*, RetargetContext)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)

		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	static void ShowWindow(FAnimationRetargetContext& RetargetContext, USkeleton* RetargetToSkeleton);
	static TSharedPtr<SWindow> DialogWindow;

private:

	TSharedRef<ITableRow> OnGenerateMontageReferenceRow(TSharedPtr<FRetargetSkeletonEntryInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);

	void OnDialogClosed(const TSharedRef<SWindow>& Window);

	/** Button Handlers */
	FReply OnOkClicked();
	FReply OnBestGuessClicked();

	/** Best guess functions to try and match asset names */
	const FAssetData* FindBestGuessMatch(const FAssetData& AssetName, const TArray<FAssetData>& PossibleAssets) const;

	/** The retargetting context we're managing*/
	FAnimationRetargetContext* RetargetContext;

	/** List of entries to the remap list*/
	TArray<TSharedPtr<FRetargetSkeletonEntryInfo>> AssetListInfo;

	/** Skeleton we're about to retarget to*/
	USkeleton* NewSkeleton;

	/** The list viewing AssetListInfo*/
	TSharedPtr<SRemapAssetEntryList> ListWidget;
};

////////////////////////////////////////////////////
/**
* FDlgRemapSkeleton
*
* Wrapper class for SAnimationRemapSkeleton. This class creates and launches a dialog then awaits the
* result to return to the user.
*/
class FDlgRemapSkeleton
{
public:
	FDlgRemapSkeleton(USkeleton* Skeleton);

	/**  Shows the dialog box and waits for the user to respond. */
	bool ShowModal();

	/** New Skeleton that is chosen **/
	USkeleton* NewSkeleton;

	/** true if you'd like to retarget skeletal meshes as well **/
	bool RetargetSkeletalMesh;

private:

	/** Cached pointer to the modal window */
	TSharedPtr<SWindow> DialogWindow;

	/** Cached pointer to the LOD Chain widget */
	TSharedPtr<class SAnimationRemapSkeleton> DialogWidget;
};


class SRemapFailures : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRemapFailures) {}
	SLATE_ARGUMENT(TArray<FText>, FailedRemaps)
		SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	static  void OpenRemapFailuresDialog(const TArray<FText>& InFailedRemaps);

private:
	TSharedRef<ITableRow> MakeListViewWidget(TSharedRef<FText> Item, const TSharedRef<STableViewBase>& OwnerTable);
	FReply CloseClicked();

private:
	TArray< TSharedRef<FText> > FailedRemaps;
};

class SSelectRetargetFolderDlg : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SSelectRetargetFolderDlg)
	{
	}

	SLATE_ARGUMENT(FText, DefaultAssetPath)
		SLATE_END_ARGS()

		SSelectRetargetFolderDlg()
		: UserResponse(EAppReturnType::Cancel)
	{
	}

	void Construct(const FArguments& InArgs);

public:
	/** Displays the dialog in a blocking fashion */
	EAppReturnType::Type ShowModal();

	/** Gets the resulting asset path */
	FString GetAssetPath();

protected:
	void OnPathChange(const FString& NewPath);
	FReply OnButtonClick(EAppReturnType::Type ButtonID);

	EAppReturnType::Type UserResponse;
	FText AssetPath;
};



////////////////////////////////////////////////////////////////////
class SSkeletonRetargetPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSkeletonRetargetPanel)
	{}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FAssetEditorToolkit> InHostingApp);
	
private:
	TWeakPtr<class ISkeletonEditor> SkeletonEditorPtr;
	TWeakPtr<class IEditableSkeleton> EditableSkeletonPtr;

	TWeakPtr<class IPersonaPreviewScene> PreviewScenePtr;
};






//////////////////////////////////////////////////////////////////////////
// SRigWindow
class SRigWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRigWindow)
	{}

	SLATE_END_ARGS()

	/**
	* Slate construction function
	*
	* @param InArgs - Arguments passed from Slate
	*
	*/
	void Construct(const FArguments& InArgs, const TWeakPtr<class IEditableSkeleton>& InEditableSkeleton, const TWeakPtr<class IPersonaPreviewScene>& InPreviewScene);

private:
	
	/**
	* Clears and rebuilds the table, according to an optional search string
	* @param SearchText - Optional search string
	*
	*/
	void CreateBoneMappingList(const FString& SearchText, TArray< TSharedPtr<FRigBoneMappingInfo> >& BoneMappingList);

	/**
	 * Callback for asset picker
	 */
	 /* Set rig set combo box*/
	void OnAssetSelected(UObject* Object);
	FText GetAssetName() const;
	void CloseComboButton();
	TSharedRef<SWidget> MakeRigPickerWithMenu();

	/** Returns true if the asset shouldn't show  */
	bool ShouldFilterAsset(const struct FAssetData& AssetData);

	URig* GetRigObject() const;

	void OnBoneMappingChanged(FName NodeName, FName BoneName);
	FName GetBoneMapping(FName NodeName);
	const struct FReferenceSkeleton& GetReferenceSkeleton() const;

	FReply OnAutoMapping();
	FReply OnClearMapping();
	FReply OnSaveMapping();
	FReply OnLoadMapping();
	FReply OnToggleView();

	// set selected mapping asset
	void SetSelectedMappingAsset(const FAssetData& InAssetData);

	FReply OnToggleAdvanced();
	FText GetAdvancedButtonText() const;

	bool SelectSourceReferenceSkeleton(URig* Rig) const;
	bool OnTargetSkeletonSelected(USkeleton* SelectedSkeleton, URig* Rig) const;

	/** Pointer back to the Persona that owns us */
	TWeakPtr<class IEditableSkeleton> EditableSkeletonPtr;

	/** show advanced? */
	bool bDisplayAdvanced;

	/** rig combo button */
	TSharedPtr< class SComboButton > AssetComboButton;

	// bone mapping widget
	TSharedPtr<class SRigBoneMapping> BoneMappingWidget;

	/** The preview scene  */
	TWeakPtr<class IPersonaPreviewScene> PreviewScenePtr;
};

#undef LOCTEXT_NAMESPACE
