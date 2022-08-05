// Copyright Epic Games, Inc. All Rights Reserved.

#include "SNewRigPicker.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Animation/Rig.h"

#define LOCTEXT_NAMESPACE "NewRigPicker"

static const FVector2D ContentBrowserWindowSize(300.0f, 300.0f);

URig* SNewRigPicker::EngineHumanoidRig = NULL;

void SNewRigPicker::Construct(const FArguments& InArgs)
{
	if (EngineHumanoidRig == NULL)
	{
		EngineHumanoidRig = LoadObject<URig>(nullptr, TEXT("/Engine/EngineMeshes/Humanoid.Humanoid"), nullptr, LOAD_None, nullptr);
	}
	if (EngineHumanoidRig == NULL)
	{
		return;
	}

	CurrentObject = InArgs._InitialObject;
	ShouldFilterAsset = InArgs._OnShouldFilterAsset;
	OnSetReference = InArgs._OnSetReference;
	OnClose = InArgs._OnClose;

	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("EngineAssets", "Default"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearAsset", "Clear"),
			LOCTEXT("ClearAsset_ToolTip", "Clears the asset set on this field"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SNewRigPicker::OnClear))
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		TSharedPtr<SWidget> MenuContent;

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		FAssetPickerConfig AssetPickerConfig;
		AssetPickerConfig.Filter.ClassNames.Add(URig::StaticClass()->GetFName());
		// Allow child classes
		AssetPickerConfig.Filter.bRecursiveClasses = true;
		// Set a delegate for setting the asset from the picker
		AssetPickerConfig.OnAssetSelected = FOnRigAssetSelected::CreateSP(this, &SNewRigPicker::OnAssetSelected);
		// Use the list view by default
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		// The initial selection should be the current value
		AssetPickerConfig.InitialAssetSelection = CurrentObject;
		// We'll do clearing ourselves
		AssetPickerConfig.bAllowNullSelection = false;
		// Focus search box
		AssetPickerConfig.bFocusSearchBoxWhenOpened = true;
		// Apply custom filter
		AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SNewRigPicker::OnShouldFilterAsset);
		// Don't allow dragging
		AssetPickerConfig.bAllowDragging = false;

		MenuContent =
			SNew(SBox)
			.WidthOverride(ContentBrowserWindowSize.X)
			.HeightOverride(ContentBrowserWindowSize.Y)
			[
				ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
			];

		MenuBuilder.AddWidget(MenuContent.ToSharedRef(), FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	ChildSlot
	[
		MenuBuilder.MakeWidget()
	];
}

void SNewRigPicker::OnClear()
{
	SetValue(NULL);
	OnClose.ExecuteIfBound();
}

void SNewRigPicker::OnAssetSelected(const FAssetData& AssetData)
{
	SetValue(AssetData);
	OnClose.ExecuteIfBound();
}

bool SNewRigPicker::OnShouldFilterAsset(const struct FAssetData& AssetData)
{
	if (EngineHumanoidRig && AssetData.ObjectPath == FName(*EngineHumanoidRig->GetPathName()))
	{
		return true;
	}

	if (ShouldFilterAsset.IsBound())
	{
		return ShouldFilterAsset.Execute(AssetData);
	}

	return false;
}
void SNewRigPicker::SetValue(const FAssetData& AssetData)
{
	OnSetReference.ExecuteIfBound(AssetData.GetAsset());
}
#undef LOCTEXT_NAMESPACE
