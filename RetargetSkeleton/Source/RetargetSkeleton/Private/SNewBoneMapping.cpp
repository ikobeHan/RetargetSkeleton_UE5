// Copyright Epic Games, Inc. All Rights Reserved.


#include "SNewBoneMapping.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SWindow.h"
#include "ReferenceSkeleton.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Widgets/Input/SButton.h"
#include "AssetNotifications.h"
#include "Animation/Rig.h"
#include "BoneSelectionWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SSkeletonWidget.h"
#include "IEditableSkeleton.h"

class FPersona;

#define LOCTEXT_NAMESPACE "SRigBoneMapping"

static const FName ColumnId_NodeNameLabel("Node Name");
static const FName ColumnID_BoneNameLabel("Bone");

void SRigBoneMappingListRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;
	BoneMappingListView = InArgs._BoneMappingListView;
	OnBoneMappingChanged = InArgs._OnBoneMappingChanged;
	OnGetBoneMapping = InArgs._OnGetBoneMapping;
	OnGetReferenceSkeleton = InArgs._OnGetReferenceSkeleton;
	OnGetFilteredText = InArgs._OnGetFilteredText;

	check(Item.IsValid());

	SMultiColumnTableRow< FRigBoneMappingInfoPtr >::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef< SWidget > SRigBoneMappingListRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == ColumnId_NodeNameLabel)
	{
		TSharedPtr< SInlineEditableTextBlock > InlineWidget;
		TSharedRef< SWidget > NewWidget =
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(InlineWidget, SInlineEditableTextBlock)
				.Text(FText::FromString(Item->GetDisplayName()))
			.HighlightText(this, &SRigBoneMappingListRow::GetFilterText)
			.IsReadOnly(true)
			.IsSelected(this, &SMultiColumnTableRow< FRigBoneMappingInfoPtr >::IsSelectedExclusively)
			];

		return NewWidget;
	}
	else
	{
		// show bone list
		// Encase the SSpinbox in an SVertical box so we can apply padding. Setting ItemHeight on the containing SListView has no effect :-(
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
			[
				SNew(SBoneSelectionWidget)
				.ToolTipText(FText::Format(LOCTEXT("BoneSelectinWidget", "Select Bone for node {0}"), FText::FromString(Item->GetDisplayName())))
			.OnBoneSelectionChanged(this, &SRigBoneMappingListRow::OnBoneSelectionChanged)
			.OnGetSelectedBone(this, &SRigBoneMappingListRow::GetSelectedBone)
			.OnGetReferenceSkeleton(OnGetReferenceSkeleton)
			.bShowVirtualBones(false)
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SRigBoneMappingListRow::OnClearButtonClicked))
			.Text(FText::FromString(TEXT("x")))
			]
			];
	}
}

FReply SRigBoneMappingListRow::OnClearButtonClicked()
{
	if (OnBoneMappingChanged.IsBound())
	{
		OnBoneMappingChanged.Execute(Item->GetNodeName(), NAME_None);
	}

	return FReply::Handled();
}

void SRigBoneMappingListRow::OnBoneSelectionChanged(FName Name)
{
	if (OnBoneMappingChanged.IsBound())
	{
		OnBoneMappingChanged.Execute(Item->GetNodeName(), Name);
	}
}

FName SRigBoneMappingListRow::GetSelectedBone(bool& bMultipleValues) const
{
	if (OnGetBoneMapping.IsBound())
	{
		return OnGetBoneMapping.Execute(Item->GetNodeName());
	}

	return NAME_None;
}

FText SRigBoneMappingListRow::GetFilterText() const
{
	if (OnGetFilteredText.IsBound())
	{
		return OnGetFilteredText.Execute();
	}

	return FText::GetEmpty();
}

//////////////////////////////////////////////////////////////////////////
// SRigBoneMapping
void SRigBoneMapping::Construct(const FArguments& InArgs)
{
	OnGetReferenceSkeletonDelegate = InArgs._OnGetReferenceSkeleton;
	OnGetBoneMappingDelegate = InArgs._OnGetBoneMapping;
	OnBoneMappingChangedDelegate = InArgs._OnBoneMappingChanged;
	OnCreateBoneMappingDelegate = InArgs._OnCreateBoneMapping;

	ChildSlot
		[
			SNew(SVerticalBox)

			// now show bone mapping
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(SHorizontalBox)
			// Filter entry
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SAssignNew(NameFilterBox, SSearchBox)
			.SelectAllTextWhenFocused(true)
		.OnTextChanged(this, &SRigBoneMapping::OnFilterTextChanged)
		.OnTextCommitted(this, &SRigBoneMapping::OnFilterTextCommitted)
		]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SAssignNew(BoneMappingListView, SBoneMappingListType)
			.ListItemsSource(&BoneMappingList)
		.OnGenerateRow(this, &SRigBoneMapping::GenerateBoneMappingRow)
		.ItemHeight(22.0f)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(ColumnId_NodeNameLabel)
			.DefaultLabel(LOCTEXT("BoneMappingBase_SourceNameLabel", "Source"))
			.FixedWidth(150.f)

			+ SHeaderRow::Column(ColumnID_BoneNameLabel)
			.DefaultLabel(LOCTEXT("BoneMappingBase_TargetNameLabel", "Target"))
		)
		]
		];

	RefreshBoneMappingList();
}

void SRigBoneMapping::OnFilterTextChanged(const FText& SearchText)
{
	// need to make sure not to have the same text go
	// otherwise, the widget gets recreated multiple times causing 
	// other issue
	if (FilterText.CompareToCaseIgnored(SearchText) != 0)
	{
		FilterText = SearchText;
		RefreshBoneMappingList();
	}
}

void SRigBoneMapping::OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo)
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged(SearchText);
}

TSharedRef<ITableRow> SRigBoneMapping::GenerateBoneMappingRow(TSharedPtr<FRigBoneMappingInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(InInfo.IsValid());

	return
		SNew(SRigBoneMappingListRow, OwnerTable)
		.Item(InInfo)
		.BoneMappingListView(BoneMappingListView)
		.OnBoneMappingChanged(OnBoneMappingChangedDelegate)
		.OnGetBoneMapping(OnGetBoneMappingDelegate)
		.OnGetReferenceSkeleton(OnGetReferenceSkeletonDelegate)
		.OnGetFilteredText(this, &SRigBoneMapping::GetFilterText);
}

void SRigBoneMapping::RefreshBoneMappingList()
{
	OnCreateBoneMappingDelegate.ExecuteIfBound(FilterText.ToString(), BoneMappingList);

	BoneMappingListView->RequestListRefresh();
}

void SRigBoneMapping::PostUndo()
{
	RefreshBoneMappingList();
}
#undef LOCTEXT_NAMESPACE

