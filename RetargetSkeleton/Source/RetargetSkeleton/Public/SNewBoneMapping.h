// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SComboButton.h"
#include "BoneSelectionWidget.h"

class IEditableSkeleton;
class URig;
class USkeleton;
template <typename ItemType> class SListView;

//////////////////////////////////////////////////////////////////////////
// FRigBoneMappingInfo

class FRigBoneMappingInfo
{
public:
	FName Name;
	FString DisplayName;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FRigBoneMappingInfo> Make(const FName NodeName, const FString DisplayName)
	{
		return MakeShareable(new FRigBoneMappingInfo(NodeName, DisplayName));
	}

	FName GetNodeName() const
	{
		return Name;
	}

	FString GetDisplayName() const
	{
		return DisplayName;
	}

protected:
	/** Hidden constructor, always use Make above */
	FRigBoneMappingInfo(const FName InNodeName, const FString InDisplayName)
		: Name(InNodeName)
		, DisplayName(InDisplayName)
	{}

	/** Hidden constructor, always use Make above */
	FRigBoneMappingInfo() {}
};

typedef SListView< TSharedPtr<FRigBoneMappingInfo> > SBoneMappingListType;

//////////////////////////////////////////////////////////////////////////
// SBoneMappingListRow
typedef TSharedPtr< FRigBoneMappingInfo > FRigBoneMappingInfoPtr;

DECLARE_DELEGATE_TwoParams(FOnRigBoneMappingChanged, FName /** NodeName */, FName /** BoneName **/);
DECLARE_DELEGATE_RetVal_OneParam(FName, FOnGetRigBoneMapping, FName /** Node Name **/);
DECLARE_DELEGATE_RetVal(FText&, FOnGetRigFilteredText)
DECLARE_DELEGATE_TwoParams(FOnCreateRigBoneMapping, const FString&, TArray< TSharedPtr<FRigBoneMappingInfo> >&)

class SRigBoneMappingListRow
	: public SMultiColumnTableRow< FRigBoneMappingInfoPtr >
{
public:

	SLATE_BEGIN_ARGS(SRigBoneMappingListRow) {}

	/** The item for this row **/
	SLATE_ARGUMENT(FRigBoneMappingInfoPtr, Item)

		/* Widget used to display the list of retarget sources*/
		SLATE_ARGUMENT(TSharedPtr<SBoneMappingListType>, BoneMappingListView)

		SLATE_EVENT(FOnRigBoneMappingChanged, OnBoneMappingChanged)

		SLATE_EVENT(FOnGetRigBoneMapping, OnGetBoneMapping)

		SLATE_EVENT(FGetReferenceSkeleton, OnGetReferenceSkeleton)

		SLATE_EVENT(FOnGetRigFilteredText, OnGetFilteredText)

		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/** Widget used to display the list of retarget sources*/
	TSharedPtr<SBoneMappingListType> BoneMappingListView;

	/** The name and weight of the retarget source*/
	FRigBoneMappingInfoPtr	Item;

	// Bone tree widget delegates
	void OnBoneSelectionChanged(FName Name);
	FReply OnClearButtonClicked();
	FName GetSelectedBone(bool& bMultipleValues) const;
	FText GetFilterText() const;

	FOnRigBoneMappingChanged	OnBoneMappingChanged;
	FOnGetRigBoneMapping		OnGetBoneMapping;
	FGetReferenceSkeleton	OnGetReferenceSkeleton;
	FOnGetRigFilteredText		OnGetFilteredText;
};

//////////////////////////////////////////////////////////////////////////
// SBoneMappingBase

class SRigBoneMapping : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRigBoneMapping)
	{}

	SLATE_EVENT(FOnRigBoneMappingChanged, OnBoneMappingChanged)
		SLATE_EVENT(FOnGetRigBoneMapping, OnGetBoneMapping)
		SLATE_EVENT(FGetReferenceSkeleton, OnGetReferenceSkeleton)
		SLATE_EVENT(FOnCreateRigBoneMapping, OnCreateBoneMapping)

		SLATE_END_ARGS()

		/**
		* Slate construction function
		*
		* @param InArgs - Arguments passed from Slate
		*
		*/
		void Construct(const FArguments& InArgs);

	/**
	* Filters the SListView when the user changes the search text box (NameFilterBox)
	*
	* @param SearchText - The text the user has typed
	*
	*/
	void OnFilterTextChanged(const FText& SearchText);

	/**
	* Filters the SListView when the user hits enter or clears the search box
	* Simply calls OnFilterTextChanged
	*
	* @param SearchText - The text the user has typed
	* @param CommitInfo - Not used
	*
	*/
	void OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo);

	/**
	* Create a widget for an entry in the tree from an info
	*
	* @param InInfo - Shared pointer to the morph target we're generating a row for
	* @param OwnerTable - The table that owns this row
	*
	* @return A new Slate widget, containing the UI for this row
	*/
	TSharedRef<ITableRow> GenerateBoneMappingRow(TSharedPtr<FRigBoneMappingInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	* Handler for the delete of retarget source
	*/
	void RefreshBoneMappingList();
private:

	/**
	* Accessor so our rows can grab the filtertext for highlighting
	*
	*/
	FText& GetFilterText() { return FilterText; }

	/** Box to filter to a specific morph target name */
	TSharedPtr<SSearchBox>	NameFilterBox;

	/** Widget used to display the list of retarget sources */
	TSharedPtr<SBoneMappingListType> BoneMappingListView;

	/** A list of retarget sources. Used by the BoneMappingListView. */
	TArray< TSharedPtr<FRigBoneMappingInfo> > BoneMappingList;

	/** Current text typed into NameFilterBox */
	FText FilterText;

	/** Delegate for undo/redo transaction **/
	void PostUndo();

	void OnBoneMappingChanged(FName NodeName, FName BoneName);
	FName GetBoneMapping(FName NodeName);

	FGetReferenceSkeleton	OnGetReferenceSkeletonDelegate;
	FOnRigBoneMappingChanged	OnBoneMappingChangedDelegate;
	FOnGetRigBoneMapping		OnGetBoneMappingDelegate;
	FOnCreateRigBoneMapping	OnCreateBoneMappingDelegate;
};
