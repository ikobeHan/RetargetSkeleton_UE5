// Copyright Epic Games, Inc. All Rights Reserved.

#include "SSkeletonRetarget.h"
#include "Modules/ModuleManager.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/AnimBlueprint.h"
#include "Editor.h"
#include "Animation/AnimSet.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "IDocumentation.h"
#include "Animation/Rig.h"
#include "AnimPreviewInstance.h"
#include "AssetRegistryModule.h"
#include "AnimationRuntime.h"
#include "Settings/SkeletalMeshEditorSettings.h"
#include "Styling/CoreStyle.h"
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ISkeletonTree.h"
#include "ISkeletonEditorModule.h"
#include "IPersonaToolkit.h"
#include "PersonaModule.h"

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
#include "RigBoneMappingHelper.h"
#include "SSkeletonWidget.h"
#include "IEditableSkeleton.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SNullWidget.h"
#include "Animation/NodeMappingContainer.h"
#include "IPersonaPreviewScene.h"
#include "SNewRigPicker.h"
#include "UObject/SavePackage.h"

class FPersona;


#define LOCTEXT_NAMESPACE "SkeletonRetargetWidget"

TSharedPtr<SWindow> SAnimationRemapSkeleton::DialogWindow;

bool SAnimationRemapSkeleton::OnShouldFilterAsset(const struct FAssetData& AssetData)
{
	USkeleton* AssetSkeleton = nullptr;
	if (AssetData.IsAssetLoaded())
	{
		AssetSkeleton = Cast<USkeleton>(AssetData.GetAsset());
	}

	// do not show same skeleton
	if (OldSkeleton && OldSkeleton == AssetSkeleton)
	{
		return true;
	}

	if (bShowOnlyCompatibleSkeletons)
	{
		if (OldSkeleton && OldSkeleton->GetRig())
		{
			URig* Rig = OldSkeleton->GetRig();

			const FString Value = AssetData.GetTagValueRef<FString>(USkeleton::RigTag);

			if (Rig->GetFullName() == Value)
			{
				return false;
			}

			// if loaded, check to see if it has same rig
			if (AssetData.IsAssetLoaded())
			{
				USkeleton* LoadedSkeleton = Cast<USkeleton>(AssetData.GetAsset());

				if (LoadedSkeleton && LoadedSkeleton->GetRig() == Rig)
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}

void SAnimationRemapSkeleton::UpdateAssetPicker()
{
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(USkeleton::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAnimationRemapSkeleton::OnAssetSelectedFromPicker);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SAnimationRemapSkeleton::OnShouldFilterAsset);
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowTypeInColumnView = false;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	if (AssetPickerBox.IsValid())
	{
		AssetPickerBox->SetContent(
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		);
	}
}

void SAnimationRemapSkeleton::Construct(const FArguments& InArgs)
{
	OldSkeleton = InArgs._CurrentSkeleton;
	NewSkeleton = nullptr;
	WidgetWindow = InArgs._WidgetWindow;
	bRemapReferencedAssets = true;
	bConvertSpaces = false;
	bShowOnlyCompatibleSkeletons = false;
	OnRetargetAnimationDelegate = InArgs._OnRetargetDelegate;
	bShowDuplicateAssetOption = InArgs._ShowDuplicateAssetOption;

	TSharedRef<SVerticalBox> RetargetWidget = SNew(SVerticalBox);

	RetargetWidget->AddSlot()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.SmallBoldFont"))
			.Text(LOCTEXT("RetargetBasePose_OptionLabel", "Retarget Options"))
		];

	if (InArgs._ShowRemapOption)
	{
		RetargetWidget->AddSlot()
		[
			SNew(SCheckBox)
			.IsChecked(this, &SAnimationRemapSkeleton::IsRemappingReferencedAssets)
			.OnCheckStateChanged(this, &SAnimationRemapSkeleton::OnRemappingReferencedAssetsChanged)
			[
				SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_RemapAssets", "Remap referenced assets "))
			]
		];

		bRemapReferencedAssets = true;

		if (InArgs._ShowExistingRemapOption)
		{
			RetargetWidget->AddSlot()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SAnimationRemapSkeleton::IsRemappingToExistingAssetsChecked)
				.IsEnabled(this, &SAnimationRemapSkeleton::IsRemappingToExistingAssetsEnabled)
				.OnCheckStateChanged(this, &SAnimationRemapSkeleton::OnRemappingToExistingAssetsChanged)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RemapSkeleton_RemapToExisting", "Allow remapping to existing assets"))
				]
			];

			// Not by default, user must specify
			bAllowRemappingToExistingAssets = false;
		}
	}

	if (InArgs._ShowConvertSpacesOption)
	{
		TSharedPtr<SToolTip> ConvertSpaceTooltip = IDocumentation::Get()->CreateToolTip(FText::FromString("Check if you'd like to convert animation data to new skeleton space. If this is false, it won't convert any animation data to new space."),
			nullptr, FString("Shared/Editors/Persona"), FString("AnimRemapSkeleton_ConvertSpace"));
		RetargetWidget->AddSlot()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SAnimationRemapSkeleton::IsConvertSpacesChecked)
				.OnCheckStateChanged(this, &SAnimationRemapSkeleton::OnConvertSpacesCheckChanged)
				[
					SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_ConvertSpaces", "Convert Spaces to new Skeleton"))
					.ToolTip(ConvertSpaceTooltip)
				]
			];

		bConvertSpaces = true;
	}

	if (InArgs._ShowCompatibleDisplayOption)
	{
		TSharedPtr<SToolTip> ConvertSpaceTooltip = IDocumentation::Get()->CreateToolTip(FText::FromString("Check if you'd like to show only the skeleton that uses the same rig."),
			nullptr, FString("Shared/Editors/Persona"), FString("AnimRemapSkeleton_ShowCompatbielSkeletons")); // @todo add tooltip
		RetargetWidget->AddSlot()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SAnimationRemapSkeleton::IsShowOnlyCompatibleSkeletonsChecked)
				.IsEnabled(this, &SAnimationRemapSkeleton::IsShowOnlyCompatibleSkeletonsEnabled)
				.OnCheckStateChanged(this, &SAnimationRemapSkeleton::OnShowOnlyCompatibleSkeletonsCheckChanged)
				[
					SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_ShowCompatible", "Show Only Compatible Skeletons"))
					.ToolTip(ConvertSpaceTooltip)
				]
			];

		bShowOnlyCompatibleSkeletons = true;
	}

	TSharedRef<SHorizontalBox> OptionWidget = SNew(SHorizontalBox);
	OptionWidget->AddSlot()
		[
			RetargetWidget
		];

	if (bShowDuplicateAssetOption)
	{
		TSharedRef<SVerticalBox> NameOptionWidget = SNew(SVerticalBox);

		NameOptionWidget->AddSlot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2, 3)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
		.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.SmallBoldFont"))
		.Text(LOCTEXT("RetargetBasePose_RenameLable", "New Asset Name"))
		]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2, 1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			[
				SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_DupeName_Prefix", "Prefix"))
			]

		+ SHorizontalBox::Slot()
			[
				SNew(SEditableTextBox)
				.Text(this, &SAnimationRemapSkeleton::GetPrefixName)
			.MinDesiredWidth(100)
			.OnTextChanged(this, &SAnimationRemapSkeleton::SetPrefixName)
			.IsReadOnly(false)
			.RevertTextOnEscape(true)
			]

			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2, 1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			[
				SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_DupeName_Suffix", "Suffix"))
			]

		+ SHorizontalBox::Slot()
			[
				SNew(SEditableTextBox)
				.Text(this, &SAnimationRemapSkeleton::GetSuffixName)
			.MinDesiredWidth(100)
			.OnTextChanged(this, &SAnimationRemapSkeleton::SetSuffixName)
			.IsReadOnly(false)
			.RevertTextOnEscape(true)
			]

			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2, 1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			[
				SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_DupeName_ReplaceFrom", "Replace "))
			]

		+ SHorizontalBox::Slot()
			[
				SNew(SEditableTextBox)
				.Text(this, &SAnimationRemapSkeleton::GetReplaceFrom)
			.MinDesiredWidth(50)
			.OnTextChanged(this, &SAnimationRemapSkeleton::SetReplaceFrom)
			.IsReadOnly(false)
			.RevertTextOnEscape(true)
			]

		+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_DupeName_ReplaceTo", "with "))
			]

		+ SHorizontalBox::Slot()
			[
				SNew(SEditableTextBox)
				.Text(this, &SAnimationRemapSkeleton::GetReplaceTo)
			.MinDesiredWidth(50)
			.OnTextChanged(this, &SAnimationRemapSkeleton::SetReplaceTo)
			.IsReadOnly(false)
			.RevertTextOnEscape(true)
			]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2, 3)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.Padding(5, 5)
			[
				SNew(STextBlock).Text(this, &SAnimationRemapSkeleton::GetExampleText)
				.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.ItalicFont"))
			]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2, 3)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RemapSkeleton_DupeName_Folder", "Folder "))
			.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.SmallBoldFont"))
			]

		+ SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(this, &SAnimationRemapSkeleton::GetFolderPath)
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
			.Text(LOCTEXT("RemapSkeleton_DupeName_ShowFolderOption", "Change..."))
			.OnClicked(this, &SAnimationRemapSkeleton::ShowFolderOption)
			]
			]
			];

		OptionWidget->AddSlot()
			[
				NameOptionWidget
			];
	}

	TSharedPtr<SToolTip> SkeletonTooltip = IDocumentation::Get()->CreateToolTip(FText::FromString("Pick a skeleton for this mesh"), nullptr, FString("Shared/Editors/Persona"), FString("Skeleton"));

	this->ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CurrentlySelectedSkeletonLabel_SelectSkeleton", "Select Skeleton"))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
		.ToolTip(SkeletonTooltip)
		]
	+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			IDocumentation::Get()->CreateAnchor(FString("Engine/Animation/Skeleton"))
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(STextBlock)
			.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.FilterFont"))
		.Text(InArgs._WarningMessage)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SSeparator)
		]

	+ SVerticalBox::Slot()
		.MaxHeight(500)
		[
			SAssignNew(AssetPickerBox, SBox)
			.WidthOverride(400)
		.HeightOverride(300)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SSeparator)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
		.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
		.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton).HAlign(HAlign_Center)
			.Text(LOCTEXT("RemapSkeleton_Apply", "Retarget"))
		.IsEnabled(this, &SAnimationRemapSkeleton::CanApply)
		.OnClicked(this, &SAnimationRemapSkeleton::OnApply)
		.HAlign(HAlign_Center)
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		]
	+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton).HAlign(HAlign_Center)
			.Text(LOCTEXT("RemapSkeleton_Cancel", "Cancel"))
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.OnClicked(this, &SAnimationRemapSkeleton::OnCancel)
		]
		]
		]

	+ SHorizontalBox::Slot()
		.Padding(2)
		.AutoWidth()
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 5)
		[
			// put nice message here
			SNew(STextBlock)
			.AutoWrapText(true)
		.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.FilterFont"))
		.ColorAndOpacity(FLinearColor::Red)
		.Text(LOCTEXT("RetargetBasePose_WarningMessage", "*Make sure you have the similar retarget base pose. \nIf they don't look alike here, you can edit your base pose in the Retarget Manager window."))
		]

	+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(0, 5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SourceSkeleteonTitle", "[Source]"))
		.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.BoldFont"))
		.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 5)
		[
			SAssignNew(SourceViewport, SReferPoseViewport)
			.Skeleton(OldSkeleton)
		]
		]

	+ SHorizontalBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TargetSkeleteonTitle", "[Target]"))
		.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.BoldFont"))
		.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 5)
		[
			SAssignNew(TargetViewport, SReferPoseViewport)
			.Skeleton(nullptr)
		]
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 5)
		[
			OptionWidget
		]
		]
		]
		];

	UpdateAssetPicker();
	UpdateExampleText();
}

FReply SAnimationRemapSkeleton::ShowFolderOption()
{
	TSharedRef<SSelectRetargetFolderDlg> NewAnimDlg =
		SNew(SSelectRetargetFolderDlg);

	if (NewAnimDlg->ShowModal() != EAppReturnType::Cancel)
	{
		NameDuplicateRule.FolderPath = NewAnimDlg->GetAssetPath();
	}

	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->BringToFront(true);
	}

	return FReply::Handled();
}

void SAnimationRemapSkeleton::UpdateExampleText()
{
	FString ReplaceFrom = FString::Printf(TEXT("Old Name : ###%s###"), *NameDuplicateRule.ReplaceFrom);
	FString ReplaceTo = FString::Printf(TEXT("New Name : %s###%s###%s"), *NameDuplicateRule.Prefix, *NameDuplicateRule.ReplaceTo, *NameDuplicateRule.Suffix);

	ExampleText = FText::FromString(FString::Printf(TEXT("%s\n%s"), *ReplaceFrom, *ReplaceTo));
}

ECheckBoxState SAnimationRemapSkeleton::IsRemappingReferencedAssets() const
{
	return bRemapReferencedAssets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAnimationRemapSkeleton::OnRemappingReferencedAssetsChanged(ECheckBoxState InNewRadioState)
{
	bRemapReferencedAssets = (InNewRadioState == ECheckBoxState::Checked);
}

ECheckBoxState SAnimationRemapSkeleton::IsRemappingToExistingAssetsChecked() const
{
	return bAllowRemappingToExistingAssets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool SAnimationRemapSkeleton::IsRemappingToExistingAssetsEnabled() const
{
	return bRemapReferencedAssets;
}

void SAnimationRemapSkeleton::OnRemappingToExistingAssetsChanged(ECheckBoxState InNewRadioState)
{
	bAllowRemappingToExistingAssets = (InNewRadioState == ECheckBoxState::Checked);
}

ECheckBoxState SAnimationRemapSkeleton::IsConvertSpacesChecked() const
{
	return bConvertSpaces ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAnimationRemapSkeleton::OnConvertSpacesCheckChanged(ECheckBoxState InNewRadioState)
{
	bConvertSpaces = (InNewRadioState == ECheckBoxState::Checked);
}

bool SAnimationRemapSkeleton::IsShowOnlyCompatibleSkeletonsEnabled() const
{
	// if convert space is false, compatible skeletons won't matter either. 
	return (bConvertSpaces == true);
}

ECheckBoxState SAnimationRemapSkeleton::IsShowOnlyCompatibleSkeletonsChecked() const
{
	return bShowOnlyCompatibleSkeletons ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAnimationRemapSkeleton::OnShowOnlyCompatibleSkeletonsCheckChanged(ECheckBoxState InNewRadioState)
{
	bShowOnlyCompatibleSkeletons = (InNewRadioState == ECheckBoxState::Checked);

	UpdateAssetPicker();
}

bool SAnimationRemapSkeleton::CanApply() const
{
	return (NewSkeleton != nullptr && NewSkeleton != OldSkeleton);
}

void SAnimationRemapSkeleton::OnAssetSelectedFromPicker(const FAssetData& AssetData)
{
	if (AssetData.GetAsset())
	{
		NewSkeleton = Cast<USkeleton>(AssetData.GetAsset());

		TargetViewport->SetSkeleton(NewSkeleton);
	}
}

FReply SAnimationRemapSkeleton::OnApply()
{
	if (OnRetargetAnimationDelegate.IsBound())
	{
		OnRetargetAnimationDelegate.Execute(OldSkeleton, NewSkeleton, bRemapReferencedAssets, bAllowRemappingToExistingAssets, bConvertSpaces, bShowDuplicateAssetOption ? &NameDuplicateRule : nullptr);
	}

	CloseWindow();
	return FReply::Handled();
}

FReply SAnimationRemapSkeleton::OnCancel()
{
	NewSkeleton = nullptr;
	CloseWindow();
	return FReply::Handled();
}

void SAnimationRemapSkeleton::OnRemapDialogClosed(const TSharedRef<SWindow>& Window)
{
	NewSkeleton = nullptr;
	DialogWindow = nullptr;
}

void SAnimationRemapSkeleton::CloseWindow()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
}

void SAnimationRemapSkeleton::ShowWindow(USkeleton* OldSkeleton, const FText& WarningMessage, bool bDuplicateAssets, FOnRetargetAnimation RetargetDelegate)
{
	if (DialogWindow.IsValid())
	{
		FSlateApplication::Get().DestroyWindowImmediately(DialogWindow.ToSharedRef());
	}

	DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("RemapSkeleton", "Select Skeleton"))
		.SupportsMinimize(false).SupportsMaximize(false)
		.SizingRule(ESizingRule::Autosized);

	TSharedPtr<class SAnimationRemapSkeleton> DialogWidget;

	TSharedPtr<SBorder> DialogWrapper =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(DialogWidget, SAnimationRemapSkeleton)
			.CurrentSkeleton(OldSkeleton)
		.WidgetWindow(DialogWindow)
		.WarningMessage(WarningMessage)
		.ShowRemapOption(true)
		.ShowExistingRemapOption(true)
		.ShowConvertSpacesOption(OldSkeleton != nullptr)
		.ShowCompatibleDisplayOption(OldSkeleton != nullptr)
		.ShowDuplicateAssetOption(bDuplicateAssets)
		.OnRetargetDelegate(RetargetDelegate)
		];

	DialogWindow->SetOnWindowClosed(FRequestDestroyWindowOverride::CreateSP(DialogWidget.Get(), &SAnimationRemapSkeleton::OnRemapDialogClosed));
	DialogWindow->SetContent(DialogWrapper.ToSharedRef());

	FSlateApplication::Get().AddWindow(DialogWindow.ToSharedRef());
}

////////////////////////////////////////////////////

FDlgRemapSkeleton::FDlgRemapSkeleton(USkeleton* Skeleton)
{
	if (FSlateApplication::IsInitialized())
	{
		DialogWindow = SNew(SWindow)
			.Title(LOCTEXT("RemapSkeleton", "Select Skeleton"))
			.SupportsMinimize(false).SupportsMaximize(false)
			.SizingRule(ESizingRule::Autosized);

		TSharedPtr<SBorder> DialogWrapper =
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				SAssignNew(DialogWidget, SAnimationRemapSkeleton)
				.CurrentSkeleton(Skeleton)
			.WidgetWindow(DialogWindow)
			];

		DialogWindow->SetContent(DialogWrapper.ToSharedRef());
	}
}

bool FDlgRemapSkeleton::ShowModal()
{
	GEditor->EditorAddModalWindow(DialogWindow.ToSharedRef());

	NewSkeleton = DialogWidget.Get()->NewSkeleton;

	return (NewSkeleton != nullptr && NewSkeleton != DialogWidget.Get()->OldSkeleton);
}

//////////////////////////////////////////////////////////////
void SRemapFailures::Construct(const FArguments& InArgs)
{
	for (auto RemapIt = InArgs._FailedRemaps.CreateConstIterator(); RemapIt; ++RemapIt)
	{
		FailedRemaps.Add(MakeShareable(new FText(*RemapIt)));
	}

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
		.Padding(FMargin(4, 8, 4, 4))
		[
			SNew(SVerticalBox)

			// Title text
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("RemapFailureTitle", "The following assets could not be Remaped."))
		]

	// Failure list
	+ SVerticalBox::Slot()
		.Padding(0, 8)
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SListView<TSharedRef<FText>>)
			.ListItemsSource(&FailedRemaps)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SRemapFailures::MakeListViewWidget)
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
		.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
		.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.Text(LOCTEXT("RemapFailuresCloseButton", "Close"))
		.OnClicked(this, &SRemapFailures::CloseClicked)
		]
		]
		]
		];
}

void SRemapFailures::OpenRemapFailuresDialog(const TArray<FText>& InFailedRemaps)
{
	TSharedRef<SWindow> RemapWindow = SNew(SWindow)
		.Title(LOCTEXT("FailedRemapsDialog", "Failed Remaps"))
		.ClientSize(FVector2D(800, 400))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SRemapFailures).FailedRemaps(InFailedRemaps)
		];

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));

	if (MainFrameModule.GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(RemapWindow, MainFrameModule.GetParentWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(RemapWindow);
	}
}

TSharedRef<ITableRow> SRemapFailures::MakeListViewWidget(TSharedRef<FText> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow< TSharedRef<FText> >, OwnerTable)
		[
			SNew(STextBlock).Text(Item.Get())
		];
}

FReply SRemapFailures::CloseClicked()
{
	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());

	if (Window.IsValid())
	{
		Window->RequestDestroyWindow();
	}

	return FReply::Handled();
}

////////////////////////////////////////
class FReferPoseViewportClient : public FEditorViewportClient
{
public:
	FReferPoseViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SReferPoseViewport>& InBasePoseViewport)
		: FEditorViewportClient(nullptr, &InPreviewScene, StaticCastSharedRef<SEditorViewport>(InBasePoseViewport))
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


	// FEditorViewportClient interface
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

	// End of FEditorViewportClient

	void UpdateLighting()
	{
		const USkeletalMeshEditorSettings* Options = GetDefault<USkeletalMeshEditorSettings>();

		PreviewScene->SetLightDirection(Options->AnimPreviewLightingDirection);
		PreviewScene->SetLightColor(Options->AnimPreviewDirectionalColor);
		PreviewScene->SetLightBrightness(Options->AnimPreviewLightBrightness);
	}
};

////////////////////////////////
// SReferPoseViewport
void SReferPoseViewport::Construct(const FArguments& InArgs)
{
	SEditorViewport::Construct(SEditorViewport::FArguments());

	PreviewComponent = NewObject<UDebugSkelMeshComponent>();
	PreviewComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	PreviewScene.AddComponent(PreviewComponent, FTransform::Identity);

	SetSkeleton(InArgs._Skeleton);
}

void SReferPoseViewport::SetSkeleton(USkeleton* Skeleton)
{
	if (Skeleton != TargetSkeleton)
	{
		TargetSkeleton = Skeleton;

		if (TargetSkeleton)
		{
			USkeletalMesh* PreviewSkeletalMesh = Skeleton->GetPreviewMesh();
			if (PreviewSkeletalMesh)
			{
				PreviewComponent->SetSkeletalMesh(PreviewSkeletalMesh);
				PreviewComponent->EnablePreview(true, nullptr);
				//				PreviewComponent->AnimScriptInstance = PreviewComponent->PreviewInstance;
				PreviewComponent->PreviewInstance->SetForceRetargetBasePose(true);
				PreviewComponent->RefreshBoneTransforms(nullptr);

				//Place the camera at a good viewer position
				FVector NewPosition = Client->GetViewLocation();
				NewPosition.Normalize();
				NewPosition *= (PreviewSkeletalMesh->GetImportedBounds().SphereRadius * 1.5f);
				Client->SetViewLocation(NewPosition);
			}
			else
			{
				PreviewComponent->SetSkeletalMesh(nullptr);
			}
		}
		else
		{
			PreviewComponent->SetSkeletalMesh(nullptr);
		}

		Client->Invalidate();
	}
}

SReferPoseViewport::SReferPoseViewport()
	: PreviewScene(FPreviewScene::ConstructionValues())
{
}

bool SReferPoseViewport::IsVisible() const
{
	return true;
}

TSharedRef<FEditorViewportClient> SReferPoseViewport::MakeEditorViewportClient()
{
	TSharedPtr<FEditorViewportClient> EditorViewportClient = MakeShareable(new FReferPoseViewportClient(PreviewScene, SharedThis(this)));

	EditorViewportClient->ViewportType = LVT_Perspective;
	EditorViewportClient->bSetListenerPosition = false;
	EditorViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
	EditorViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);

	EditorViewportClient->SetRealtime(false);
	EditorViewportClient->VisibilityDelegate.BindSP(this, &SReferPoseViewport::IsVisible);
	EditorViewportClient->SetViewMode(VMI_Lit);

	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SReferPoseViewport::MakeViewportToolbar()
{
	return nullptr;
}


/////////////////////////////////////////////////
// select folder dialog
//////////////////////////////////////////////////
void SSelectRetargetFolderDlg::Construct(const FArguments& InArgs)
{
	AssetPath = FText::FromString(FPackageName::GetLongPackagePath(InArgs._DefaultAssetPath.ToString()));

	if (AssetPath.IsEmpty())
	{
		AssetPath = FText::FromString(TEXT("/Game"));
	}

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = AssetPath.ToString();
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SSelectRetargetFolderDlg::OnPathChange);
	PathPickerConfig.bAddDefaultPath = true;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("SSelectFolderDlg_Title", "Create New Animation Object"))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		//.SizingRule( ESizingRule::Autosized )
		.ClientSize(FVector2D(450, 450))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot() // Add user input block
		.Padding(2)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectPath", "Select Path"))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
		]

	+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(3)
		[
			ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
		]
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(5)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
		.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
		.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.Text(LOCTEXT("OK", "OK"))
		.OnClicked(this, &SSelectRetargetFolderDlg::OnButtonClick, EAppReturnType::Ok)
		]
	+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.Text(LOCTEXT("Cancel", "Cancel"))
		.OnClicked(this, &SSelectRetargetFolderDlg::OnButtonClick, EAppReturnType::Cancel)
		]
		]
		]);
}

void SSelectRetargetFolderDlg::OnPathChange(const FString& NewPath)
{
	AssetPath = FText::FromString(NewPath);
}

FReply SSelectRetargetFolderDlg::OnButtonClick(EAppReturnType::Type ButtonID)
{
	UserResponse = ButtonID;

	RequestDestroyWindow();

	return FReply::Handled();
}

EAppReturnType::Type SSelectRetargetFolderDlg::ShowModal()
{
	GEditor->EditorAddModalWindow(SharedThis(this));
	return UserResponse;
}

FString SSelectRetargetFolderDlg::GetAssetPath()
{
	return AssetPath.ToString();
}

//////////////////////////////////////////////////////////////////////////
// Window for remapping assets when retargetting
TSharedPtr<SWindow> SAnimationRemapAssets::DialogWindow;

void SAnimationRemapAssets::Construct(const FArguments& InArgs)
{
	NewSkeleton = InArgs._NewSkeleton;
	RetargetContext = InArgs._RetargetContext;

	TSharedPtr<SVerticalBox> ListBox;

	TArray<UObject*> Duplicates = RetargetContext->GetAllDuplicates();

	for (UObject* Asset : Duplicates)
	{
		// We don't want to add anim blueprints here, just animation assets
		if (Asset->GetClass() != UAnimBlueprint::StaticClass())
		{
			AssetListInfo.Add(FRetargetSkeletonEntryInfo::Make(Asset, NewSkeleton));
		}
	}

	this->ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.Padding(5.0f)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RemapAssetsDescription", "The assets shown below need to be duplicated or remapped for the new blueprint. Select a new animation to use in the new animation blueprint for each asset or leave blank to duplicate the existing asset."))
		.AutoWrapText(true)
		]
	+ SVerticalBox::Slot()
		.Padding(5.0f)
		.AutoHeight()
		.MaxHeight(500.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SAssignNew(ListWidget, SRemapAssetEntryList)
			.ItemHeight(20.0f)
		.ListItemsSource(&AssetListInfo)
		.OnGenerateRow(this, &SAnimationRemapAssets::OnGenerateMontageReferenceRow)
		.SelectionMode(ESelectionMode::None)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column("AssetName")
			.DefaultLabel(LOCTEXT("ColumnLabel_RemapAssetName", "Asset Name"))
			+ SHeaderRow::Column("AssetType")
			.DefaultLabel(LOCTEXT("ColumnLabel_RemapAssetType", "Asset Type"))
			+ SHeaderRow::Column("AssetRemap")
			.DefaultLabel(LOCTEXT("ColumnLabel_RemapAssetRemap", "Remapped Asset"))
		)
		]
		]
	+ SVerticalBox::Slot()
		.Padding(5.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.FillWidth(0.2f)
		[
			SNew(SButton)
			.ContentPadding(2.0f)
		.OnClicked(this, &SAnimationRemapAssets::OnBestGuessClicked)
		.Content()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BestGuessButton", "Auto-Fill Using Best Guess"))
		]
		]
	+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
		[
			SNew(STextBlock)
			.AutoWrapText(true)
		.Text(LOCTEXT("BestGuessDescription", "Auto-Fill will look at the names of all compatible assets for the new skeleton and look for something similar to use for the remapped asset."))
		]
		]

	+ SVerticalBox::Slot()
		.Padding(5.0f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Content()
		[
			SNew(SButton)
			.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.OnClicked(this, &SAnimationRemapAssets::OnOkClicked)
		.Content()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OkButton", "OK"))
		]
		]
		]
		];
}

void SAnimationRemapAssets::ShowWindow(FAnimationRetargetContext& RetargetContext, USkeleton* RetargetToSkeleton)
{
	if (DialogWindow.IsValid())
	{
		FSlateApplication::Get().DestroyWindowImmediately(DialogWindow.ToSharedRef());
	}

	DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("RemapAssets", "Choose Assets to Remap"))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.HasCloseButton(false)
		.MaxWidth(1024.0f)
		.SizingRule(ESizingRule::Autosized);

	TSharedPtr<class SAnimationRemapAssets> DialogWidget;

	TSharedPtr<SBorder> DialogWrapper =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(DialogWidget, SAnimationRemapAssets)
			.NewSkeleton(RetargetToSkeleton)
		.RetargetContext(&RetargetContext)
		];

	DialogWindow->SetOnWindowClosed(FRequestDestroyWindowOverride::CreateSP(DialogWidget.Get(), &SAnimationRemapAssets::OnDialogClosed));
	DialogWindow->SetContent(DialogWrapper.ToSharedRef());

	FSlateApplication::Get().AddModalWindow(DialogWindow.ToSharedRef(), nullptr);
}

TSharedRef<ITableRow> SAnimationRemapAssets::OnGenerateMontageReferenceRow(TSharedPtr<FRetargetSkeletonEntryInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SRetargetAssetEntryRow, OwnerTable)
		.DisplayedInfo(Item);
}

void SAnimationRemapAssets::OnDialogClosed(const TSharedRef<SWindow>& Window)
{
	DialogWindow = nullptr;
}

FReply SAnimationRemapAssets::OnOkClicked()
{
	for (TSharedPtr<FRetargetSkeletonEntryInfo> AssetInfo : AssetListInfo)
	{
		if (AssetInfo->RemapAsset)
		{
			RetargetContext->AddRemappedAsset(Cast<UAnimationAsset>(AssetInfo->AnimAsset), Cast<UAnimationAsset>(AssetInfo->RemapAsset));
		}
	}

	DialogWindow->RequestDestroyWindow();

	return FReply::Handled();
}

FReply SAnimationRemapAssets::OnBestGuessClicked()
{
	// collect all compatible assets
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FString SkeletonName = FAssetData(NewSkeleton).GetExportTextName();

	TArray<FAssetData> CompatibleAssets;
	TArray<FAssetData> AssetDataList;

	AssetRegistryModule.Get().GetAssetsByClass(UAnimationAsset::StaticClass()->GetFName(), AssetDataList, true);

	for (const FAssetData& Data : AssetDataList)
	{
		if (Data.GetTagValueRef<FString>("Skeleton") == SkeletonName)
		{
			CompatibleAssets.Add(Data);
		}
	}

	if (CompatibleAssets.Num() > 0)
	{
		// Do best guess analysis for the assets based on name.
		for (TSharedPtr<FRetargetSkeletonEntryInfo>& Info : AssetListInfo)
		{
			const FAssetData* BestMatchData = FindBestGuessMatch(FAssetData(Info->AnimAsset), CompatibleAssets);
			Info->RemapAsset = BestMatchData ? BestMatchData->GetAsset() : nullptr;
		}
	}

	ListWidget->RequestListRefresh();

	return FReply::Handled();
}

const FAssetData* SAnimationRemapAssets::FindBestGuessMatch(const FAssetData& AssetData, const TArray<FAssetData>& PossibleAssets) const
{
	int32 LowestScore = MAX_int32;
	int32 FoundIndex = INDEX_NONE;

	for (int32 Idx = 0; Idx < PossibleAssets.Num(); ++Idx)
	{
		const FAssetData& Data = PossibleAssets[Idx];

		if (Data.AssetClass == AssetData.AssetClass)
		{
			int32 Distance = FAnimationRuntime::GetStringDistance(AssetData.AssetName.ToString(), Data.AssetName.ToString());

			if (Distance < LowestScore)
			{
				LowestScore = Distance;
				FoundIndex = Idx;
			}
		}
	}

	if (FoundIndex != INDEX_NONE)
	{
		return &PossibleAssets[FoundIndex];
	}

	return nullptr;
}

void SRetargetAssetEntryRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	check(InArgs._DisplayedInfo.IsValid());
	DisplayedInfo = InArgs._DisplayedInfo;

	SkeletonExportName = FAssetData(DisplayedInfo->NewSkeleton).GetExportTextName();

	SMultiColumnTableRow<TSharedPtr<FRetargetSkeletonEntryInfo>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SRetargetAssetEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == TEXT("AssetName"))
	{
		return SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("AssetNameEntry", "{0}"), FText::FromString(DisplayedInfo->AnimAsset->GetName())));
	}
	else if (ColumnName == TEXT("AssetType"))
	{
		return SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("AssetTypeEntry", "{0}"), FText::FromString(DisplayedInfo->AnimAsset->GetClass()->GetName())));
	}
	else if (ColumnName == TEXT("AssetRemap"))
	{
		return SNew(SBox)
			.Padding(2.0f)
			[
				SNew(SComboButton)
				.ToolTipText(LOCTEXT("AssetRemapTooltip", "Select compatible asset to remap to."))
			.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
			.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
			.OnGetMenuContent(this, &SRetargetAssetEntryRow::GetRemapMenuContent)
			.ContentPadding(2.0f)
			.ButtonContent()
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "PropertyEditor.AssetClass")
			.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
			.Text(this, &SRetargetAssetEntryRow::GetRemapMenuButtonText)
			]
			];
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SRetargetAssetEntryRow::GetRemapMenuContent()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig PickerConfig;
	PickerConfig.SelectionMode = ESelectionMode::Single;
	PickerConfig.Filter.ClassNames.Add(DisplayedInfo->AnimAsset->GetClass()->GetFName());
	PickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SRetargetAssetEntryRow::OnAssetSelected);
	PickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SRetargetAssetEntryRow::OnShouldFilterAsset);
	PickerConfig.bAllowNullSelection = true;

	return SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(768)
		[
			ContentBrowserModule.Get().CreateAssetPicker(PickerConfig)
		];
}

FText SRetargetAssetEntryRow::GetRemapMenuButtonText() const
{
	FText NameText = (DisplayedInfo->RemapAsset) ? FText::FromString(*DisplayedInfo->RemapAsset->GetName()) : LOCTEXT("AssetRemapNone", "None");

	return FText::Format(LOCTEXT("RemapButtonText", "{0}"), NameText);
}

void SRetargetAssetEntryRow::OnAssetSelected(const FAssetData& AssetData)
{
	// Close the asset picker menu
	FSlateApplication::Get().DismissAllMenus();

	RemapAsset = AssetData.GetAsset();
	DisplayedInfo->RemapAsset = RemapAsset.Get();
}

bool SRetargetAssetEntryRow::OnShouldFilterAsset(const FAssetData& AssetData) const
{
	if (AssetData.GetTagValueRef<FString>("Skeleton") == SkeletonExportName)
	{
		return false;
	}

	return true;
}

TSharedRef<FRetargetSkeletonEntryInfo> FRetargetSkeletonEntryInfo::Make(UObject* InAsset, USkeleton* InNewSkeleton)
{
	return MakeShareable(new FRetargetSkeletonEntryInfo(InAsset, InNewSkeleton));
}

FRetargetSkeletonEntryInfo::FRetargetSkeletonEntryInfo(UObject* InAsset, USkeleton* InNewSkeleton)
	: NewSkeleton(InNewSkeleton)
	, AnimAsset(InAsset)
	, RemapAsset(nullptr)
{

}



////////////////////////////////////////////////////////////////////////////////////////////////////
void SSkeletonRetargetPanel::Construct(const FArguments& InArgs, TWeakPtr<FAssetEditorToolkit> InHostingApp)
{
	SkeletonEditorPtr = StaticCastSharedPtr<ISkeletonEditor>(InHostingApp.Pin());
	EditableSkeletonPtr = SkeletonEditorPtr.Pin()->GetPersonaToolkit()->GetEditableSkeleton();
	PreviewScenePtr = SkeletonEditorPtr.Pin()->GetPersonaToolkit()->GetPreviewScene();
	const FString DocLink = TEXT("Shared/Editors/Persona");
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(5, 5)
		.AutoHeight()
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "Persona.RetargetManager.ImportantText")
			.Text(LOCTEXT("RetargetSource_Title", "Manage Retarget Rig"))
		]
		+ SVerticalBox::Slot()
		.Padding(5, 5)
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
		.	ToolTip(IDocumentation::Get()->CreateToolTip(LOCTEXT("RetargetSource_Tooltip", "Add/Delete/Rename Retarget Sources."),
			NULL,
			DocLink,
			TEXT("RetargetSource")))
			.Font(FEditorStyle::GetFontStyle(TEXT("Persona.RetargetManager.FilterFont")))
			.Text(LOCTEXT("RetargetSource_Description", "使用骨架重定向之前需先设置Rig,并正确映射两根需要重定向的骨架（使用相同Rig）,否则会使用默认且不保证正确性"))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(2, 5)
		[
			// construct rig manager window
			SNew(SRigWindow, EditableSkeletonPtr, PreviewScenePtr)
		]
	];
}




//////////////////////////////////////////////////////////////////////////
// SRigWindow
void SRigWindow::Construct(const FArguments& InArgs, const TWeakPtr<class IEditableSkeleton>& InEditableSkeleton, const TWeakPtr<class IPersonaPreviewScene>& InPreviewScene)
{
	EditableSkeletonPtr = InEditableSkeleton;
	PreviewScenePtr = InPreviewScene;
	InEditableSkeleton.Pin()->RefreshRigConfig();

	ChildSlot
	[
		SNew(SVerticalBox)

		// first add rig asset picker
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RigNameLabel", "Select Rig "))
				.Font(FEditorStyle::GetFontStyle("Persona.RetargetManager.BoldFont"))
			]
			+ SHorizontalBox::Slot()
			[
				SAssignNew(AssetComboButton, SComboButton)
				//.ToolTipText( this, &SPropertyEditorAsset::OnGetToolTip )
				.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
				.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
				.OnGetMenuContent(this, &SRigWindow::MakeRigPickerWithMenu)
				.ContentPadding(2.0f)
				.ButtonContent()
				[
				// Show the name of the asset or actor
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "PropertyEditor.AssetClass")
				.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.Text(this, &SRigWindow::GetAssetName)
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2, 5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(2, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SRigWindow::OnAutoMapping))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("AutoMapping_Title", "AutoMap"))
				.ToolTipText(LOCTEXT("AutoMapping_Tooltip", "Automatically map the best matching bones"))
			]

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(2, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SRigWindow::OnClearMapping))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("ClearMapping_Title", "Clear"))
				.ToolTipText(LOCTEXT("ClearMapping_Tooltip", "Clear currently mapping bones"))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(2, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SRigWindow::OnSaveMapping))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("SaveMapping_Title", "Save"))
			.ToolTipText(LOCTEXT("SaveMapping_Tooltip", "Save currently mapping bones"))
			]

		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(2, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SRigWindow::OnLoadMapping))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("LoadMapping_Title", "Load"))
			.ToolTipText(LOCTEXT("LoadMapping_Tooltip", "Load mapping from saved asset."))
			]

		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(2, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SRigWindow::OnToggleAdvanced))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(this, &SRigWindow::GetAdvancedButtonText)
			.ToolTipText(LOCTEXT("ToggleAdvanced_Tooltip", "Toggle Base/Advanced configuration"))
			]
		]

		// now show bone mapping
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(0, 2)
		[
			SAssignNew(BoneMappingWidget, SRigBoneMapping)
			.OnBoneMappingChanged(this, &SRigWindow::OnBoneMappingChanged)
			.OnGetBoneMapping(this, &SRigWindow::GetBoneMapping)
			.OnCreateBoneMapping(this, &SRigWindow::CreateBoneMappingList)
			.OnGetReferenceSkeleton(this, &SRigWindow::GetReferenceSkeleton)
		]
	];
}

void SRigWindow::CreateBoneMappingList(const FString& SearchText, TArray< TSharedPtr<FRigBoneMappingInfo> >& BoneMappingList)
{
	BoneMappingList.Empty();

	const USkeleton& Skeleton = EditableSkeletonPtr.Pin()->GetSkeleton();
	const URig* Rig = Skeleton.GetRig();

	if (Rig)
	{
		bool bDoFiltering = !SearchText.IsEmpty();
		const TArray<FNode>& Nodes = Rig->GetNodes();

		for (const auto& Node : Nodes)
		{
			const FName& Name = Node.Name;
			const FString& DisplayName = Node.DisplayName;
			const FName& BoneName = Skeleton.GetRigBoneMapping(Name);

			if (Node.bAdvanced == bDisplayAdvanced)
			{
				if (bDoFiltering)
				{
					// make sure it doens't fit any of them
					if (!Name.ToString().Contains(SearchText) && !DisplayName.Contains(SearchText) && !BoneName.ToString().Contains(SearchText))
					{
						continue; // Skip items that don't match our filter
					}
				}

				TSharedRef<FRigBoneMappingInfo> Info = FRigBoneMappingInfo::Make(Name, DisplayName);

				BoneMappingList.Add(Info);
			}
		}
	}
}


void SRigWindow::OnAssetSelected(UObject* Object)
{
	AssetComboButton->SetIsOpen(false);

	EditableSkeletonPtr.Pin()->SetRigConfig(Cast<URig>(Object));

	BoneMappingWidget.Get()->RefreshBoneMappingList();

	FAssetNotifications::SkeletonNeedsToBeSaved(&EditableSkeletonPtr.Pin()->GetSkeleton());
}

/** Returns true if the asset shouldn't show  */
bool SRigWindow::ShouldFilterAsset(const struct FAssetData& AssetData)
{
	return (AssetData.GetAsset() == GetRigObject());
}

URig* SRigWindow::GetRigObject() const
{
	const USkeleton& Skeleton = EditableSkeletonPtr.Pin()->GetSkeleton();
	return Skeleton.GetRig();
}

void SRigWindow::OnBoneMappingChanged(FName NodeName, FName BoneName)
{
	EditableSkeletonPtr.Pin()->SetRigBoneMapping(NodeName, BoneName);
}

FName SRigWindow::GetBoneMapping(FName NodeName)
{
	const USkeleton& Skeleton = EditableSkeletonPtr.Pin()->GetSkeleton();
	return Skeleton.GetRigBoneMapping(NodeName);
}

FReply SRigWindow::OnToggleAdvanced()
{
	bDisplayAdvanced = !bDisplayAdvanced;

	BoneMappingWidget.Get()->RefreshBoneMappingList();

	return FReply::Handled();
}

FText SRigWindow::GetAdvancedButtonText() const
{
	if (bDisplayAdvanced)
	{
		return LOCTEXT("ShowBase", "Show Base");
	}

	return LOCTEXT("ShowAdvanced", "Show Advanced");
}

TSharedRef<SWidget> SRigWindow::MakeRigPickerWithMenu()
{
	const USkeleton& Skeleton = EditableSkeletonPtr.Pin()->GetSkeleton();

	//return SNullWidget::NullWidget;
	return
		SNew(SNewRigPicker)
		.InitialObject(Skeleton.GetRig())
		.OnShouldFilterAsset(this, &SRigWindow::ShouldFilterAsset)
		.OnSetReference(this, &SRigWindow::OnAssetSelected)
		.OnClose(this, &SRigWindow::CloseComboButton);
}

void SRigWindow::CloseComboButton()
{
	AssetComboButton->SetIsOpen(false);
}

FText SRigWindow::GetAssetName() const
{
	URig* Rig = GetRigObject();
	if (Rig)
	{
		return FText::FromString(Rig->GetName());
	}

	return LOCTEXT("None", "None");
}

const struct FReferenceSkeleton& SRigWindow::GetReferenceSkeleton() const
{
	return  EditableSkeletonPtr.Pin()->GetSkeleton().GetReferenceSkeleton();
}

bool SRigWindow::OnTargetSkeletonSelected(USkeleton* SelectedSkeleton, URig* Rig) const
{
	if (SelectedSkeleton)
	{
		// make sure the skeleton contains all the rig node names
		const FReferenceSkeleton& RefSkeleton = SelectedSkeleton->GetReferenceSkeleton();

		if (RefSkeleton.GetNum() > 0)
		{
			const TArray<FNode> RigNodes = Rig->GetNodes();
			int32 BoneMatched = 0;

			for (const auto& RigNode : RigNodes)
			{
				if (RefSkeleton.FindBoneIndex(RigNode.Name) != INDEX_NONE)
				{
					++BoneMatched;
				}
			}

			float BoneMatchedPercentage = (float)(BoneMatched) / RefSkeleton.GetNum();
			if (BoneMatchedPercentage > 0.5f)
			{
				Rig->SetSourceReferenceSkeleton(RefSkeleton);

				return true;
			}
		}
	}

	return false;
}

bool SRigWindow::SelectSourceReferenceSkeleton(URig* Rig) const
{
	TSharedRef<SWindow> WidgetWindow = SNew(SWindow)
		.Title(LOCTEXT("SelectSourceSkeletonForRig", "Select Source Skeleton for the Rig"))
		.ClientSize(FVector2D(500, 600));

	TSharedRef<SSkeletonSelectorWindow> SkeletonSelectorWindow = SNew(SSkeletonSelectorWindow).WidgetWindow(WidgetWindow);

	WidgetWindow->SetContent(SkeletonSelectorWindow);

	GEditor->EditorAddModalWindow(WidgetWindow);
	USkeleton* RigSkeleton = SkeletonSelectorWindow->GetSelectedSkeleton();
	if (RigSkeleton)
	{
		return OnTargetSkeletonSelected(RigSkeleton, Rig);
	}

	return false;
}


FReply SRigWindow::OnAutoMapping()
{
	URig* Rig = GetRigObject();
	if (Rig)
	{
		if (!Rig->IsSourceReferenceSkeletonAvailable())
		{
			//ask if they want to set up source skeleton
			EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("TheRigNeedsSkeleton", "In order to attempt to auto-map bones, the rig should have the source skeleton. However, the current rig is missing the source skeleton. Would you like to choose one? It's best to select the skeleton this rig is from."));

			if (Response == EAppReturnType::No)
			{
				return FReply::Handled();
			}

			if (!SelectSourceReferenceSkeleton(Rig))
			{
				return FReply::Handled();
			}
		}

		FReferenceSkeleton RigReferenceSkeleton = Rig->GetSourceReferenceSkeleton();
		const USkeleton& Skeleton = EditableSkeletonPtr.Pin()->GetSkeleton();
		FRigBoneMappingHelper Helper(RigReferenceSkeleton, Skeleton.GetReferenceSkeleton());
		TMap<FName, FName> BestMatches;
		Helper.TryMatch(BestMatches);

		EditableSkeletonPtr.Pin()->SetRigBoneMappings(BestMatches);
		// refresh the list
		BoneMappingWidget->RefreshBoneMappingList();
	}

	return FReply::Handled();
}

FReply SRigWindow::OnClearMapping()
{
	URig* Rig = GetRigObject();
	if (Rig)
	{
		const TArray<FNode>& Nodes = Rig->GetNodes();
		TMap<FName, FName> Mappings;
		for (const auto& Node : Nodes)
		{
			Mappings.Add(Node.Name, NAME_None);
		}

		EditableSkeletonPtr.Pin()->SetRigBoneMappings(Mappings);

		// refresh the list
		BoneMappingWidget->RefreshBoneMappingList();
	}
	return FReply::Handled();
}

// save mapping function
FReply SRigWindow::OnSaveMapping()
{
	URig* Rig = GetRigObject();
	if (Rig)
	{
		const USkeleton& Skeleton = EditableSkeletonPtr.Pin()->GetSkeleton();
		const FString DefaultPackageName = Skeleton.GetPathName();
		const FString DefaultPath = FPackageName::GetLongPackagePath(DefaultPackageName);
		const FString DefaultName = TEXT("BoneMapping");

		// Initialize SaveAssetDialog config
		FSaveAssetDialogConfig SaveAssetDialogConfig;
		SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("SaveMappingToAsset", "Save Mapping");
		SaveAssetDialogConfig.DefaultPath = DefaultPath;
		SaveAssetDialogConfig.DefaultAssetName = DefaultName;
		SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
		SaveAssetDialogConfig.AssetClassNames.Add(UNodeMappingContainer::StaticClass()->GetFName());

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
		if (!SaveObjectPath.IsEmpty())
		{
			const FString SavePackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
			const FString SavePackagePath = FPaths::GetPath(SavePackageName);
			const FString SaveAssetName = FPaths::GetBaseFilename(SavePackageName);

			// create package and create object
			UPackage* Package = CreatePackage(*SavePackageName);
			UNodeMappingContainer* MapperClass = NewObject<UNodeMappingContainer>(Package, *SaveAssetName, RF_Public | RF_Standalone);
			USkeletalMeshComponent* PreviewMeshComp = PreviewScenePtr.Pin()->GetPreviewMeshComponent();
			USkeletalMesh* PreviewMesh = PreviewMeshComp->SkeletalMesh;
			if (MapperClass && PreviewMesh)
			{
				// update mapping information on the class
				MapperClass->SetSourceAsset(Rig);
				MapperClass->SetTargetAsset(PreviewMesh);

				const TArray<FNode>& Nodes = Rig->GetNodes();
				for (const auto& Node : Nodes)
				{
					FName MappingName = Skeleton.GetRigBoneMapping(Node.Name);
					if (Node.Name != NAME_None && MappingName != NAME_None)
					{
						MapperClass->AddMapping(Node.Name, MappingName);
					}
				}

				// save mapper class
				FString const PackageName = Package->GetName();
				FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

				//UPackage::SavePackage(Package, NULL, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Standalone;
				SaveArgs.Error = GError;
				SaveArgs.bWarnOfLongFilename = true;
				SaveArgs.SaveFlags = SAVE_NoError;
				UPackage::SavePackage(Package, NULL, *PackageFileName, SaveArgs);

			}
		}
	}
	return FReply::Handled();
}

FReply SRigWindow::OnLoadMapping()
{
	// show list of skeletalmeshes that they can choose from
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UNodeMappingContainer::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SRigWindow::SetSelectedMappingAsset);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;

	TSharedRef<SWidget> Widget = SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(768)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.25f, 0.25f, 0.25f, 1.f))
		.Padding(2)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		]
		]
		];

	FSlateApplication::Get().PushMenu(
		AsShared(),
		FWidgetPath(),
		Widget,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::TopMenu)
	);

	return FReply::Handled();
}

FReply SRigWindow::OnToggleView()
{
	return FReply::Handled();
}

void SRigWindow::SetSelectedMappingAsset(const FAssetData& InAssetData)
{
	UNodeMappingContainer* Container = Cast<UNodeMappingContainer>(InAssetData.GetAsset());
	if (Container)
	{
		const TMap<FName, FName> SourceToTarget = Container->GetNodeMappingTable();
		EditableSkeletonPtr.Pin()->SetRigBoneMappings(SourceToTarget);
	}

	FSlateApplication::Get().DismissAllMenus();
}
#undef LOCTEXT_NAMESPACE 
