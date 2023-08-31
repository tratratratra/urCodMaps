// // Copyright 2022 nuclearfriend


#include "SBlendAssetImportDialog.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SComboBox.h"
#include "IStructureDetailsView.h"

#if ENGINE_MAJOR_VERSION >= 5
	#include "SWarningOrErrorBox.h"
#endif

#define LOCTEXT_NAMESPACE "BlendImporterModule"

static const ISlateStyle& GetSlateStyle()
{
	#if ENGINE_MAJOR_VERSION < 5
		return FEditorStyle::Get();
	#else
		return FAppStyle::Get();
	#endif
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlendAssetImportDialog::Construct(const FArguments& InArgs)
{
	Collections = InArgs._Collections;
	UseObjectPivot = false;

	TSharedPtr<SComboBox<TSharedPtr<FString>>> ObjectPivotComboBox;
	TSharedPtr<SWidget> MaterialWarning;
	TSharedPtr<SVerticalBox> FilterCollections;

	ComboBoxItems.Add(MakeShareable(new FString(TEXT("World"))));
	ComboBoxItems.Add(MakeShareable(new FString(TEXT("Object"))));

	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("SBlendAssetImportDialog_Title", "Blend Import Options"))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.ClientSize(FVector2D(400, 300))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 5, 0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.MaxWidth(28.0f)
				[
					SNew(SBox)
					.Padding(6)
					[
						SNew(SImage)
						#if ENGINE_MAJOR_VERSION < 5
							.Image(FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.ImportIcon").GetIcon())
						#else
							.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import").GetIcon())
						#endif
					]
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredHeight(28.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(InArgs._Filename)
						.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
					]
				]
			]
			
			+SVerticalBox::Slot()
			.AutoHeight()
			[				
				SNew(SBorder)
				.BorderImage(GetSlateStyle().GetBrush("DetailsView.CategoryTop"))
				.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(SBox)
					.MinDesiredHeight(16.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("General"))
						.Font(GetSlateStyle().GetFontStyle("DetailsView.CategoryFontStyle"))
						.TextStyle(GetSlateStyle(), "DetailsView.CategoryTextStyle")
					]
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(SHorizontalBox)
				.ToolTipText(FText::FromString("Origin\nImport meshes with either their Object or World origin position."))
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Origin"))
					.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SAssignNew(ObjectPivotComboBox, SComboBox<TSharedPtr<FString>>)
					.ContentPadding(FMargin(4.f, 1.f))
					.OptionsSource(&ComboBoxItems)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{ 
						return SNew(STextBlock).Text(FText::FromString(*Item));
					})
					.OnSelectionChanged_Lambda([this] (TSharedPtr<FString> InSelection, ESelectInfo::Type InSelectInfo) 
					{
						if (InSelection.IsValid() && ComboBoxTitleBlock.IsValid())
						{
							ComboBoxTitleBlock->SetText(FText::FromString(*InSelection));

							UseObjectPivot = *InSelection == TEXT("Object");
						}
 					} )
					[
						SAssignNew(ComboBoxTitleBlock, STextBlock).Text(FText::FromString(*ComboBoxItems[0]))
					]
				]
			]
			
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(GetSlateStyle().GetBrush("DetailsView.CategoryTop"))
				.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(SBox)
					.MinDesiredHeight(16.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Collections"))
						.Font(GetSlateStyle().GetFontStyle("DetailsView.CategoryFontStyle"))
						.TextStyle(GetSlateStyle(), "DetailsView.CategoryTextStyle")
					]
				]
			]

			+SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(5)
			[
				SAssignNew(FilterCollections, SVerticalBox)
			]

			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.VAlign(VAlign_Bottom)
			.Padding(5)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					#if ENGINE_MAJOR_VERSION >= 5
						SAssignNew(MaterialWarning, SWarningOrErrorBox)
						.MessageStyle(EMessageStyle::Warning)
						.Message(FText::FromString("This file has issues which may make the materials import incorrectly. Check the \"Blend Importer\" section of the Message Log for more details."))
					#else
						SAssignNew(MaterialWarning, STextBlock)
						.Text(FText::FromString("This file has issues which may make the materials import incorrectly. Check the \"Blend Importer\" section of the Message Log for more details."))
						.ColorAndOpacity(FLinearColor::Yellow)
						.AutoWrapText(true)
					#endif
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(5)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(GetSlateStyle().GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(GetSlateStyle().GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(GetSlateStyle().GetFloat("StandardDialog.MinDesiredSlotHeight"))
					+SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(GetSlateStyle().GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("OK", "OK"))
						.OnClicked(this, &SBlendAssetImportDialog::OnButtonClick, EAppReturnType::Ok)
					]
					+SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(GetSlateStyle().GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("Cancel", "Cancel"))
						.OnClicked(this, &SBlendAssetImportDialog::OnButtonClick, EAppReturnType::Cancel)
					]
				]
			]
		]);

	MaterialWarning->SetVisibility(InArgs._ShowMaterialWarning ? EVisibility::Visible : EVisibility::Hidden);

	if (InArgs._PreviousOptions->bUseObjectPivot)
	{
		ObjectPivotComboBox->SetSelectedItem(ComboBoxItems[1]);
	}

	if (InArgs._Collections.Num() > 0)
	{
		bool bSimilarCollections = false;
		for (int32 i = 0; i < InArgs._Collections.Num(); i++)
		{
			if (InArgs._PreviousOptions->EnabledCollections.Find(InArgs._Collections[i]) != INDEX_NONE)
			{
				bSimilarCollections = true;
				break;
			}
		}

		for (int32 i = 0; i < InArgs._Collections.Num(); i++)
		{
			TSharedPtr<SCheckBox> CollectionCheckbox;

			FilterCollections->AddSlot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Margin(FMargin(10.f, 5.f, 0.f, 5.f))
					.Text(FText::FromString(InArgs._Collections[i]))
					.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Right)
				[
					SAssignNew(CollectionCheckbox, SCheckBox)
					.OnCheckStateChanged(this, &SBlendAssetImportDialog::OnCheckStateCollectionChanged, i)
				]
			];

			bool IsChecked = false;
			if (bSimilarCollections)
			{
				if (InArgs._PreviousOptions->EnabledCollections.Find(InArgs._Collections[i]) != INDEX_NONE)
				{
					IsChecked = true;
				}
			}
			else
			{
				IsChecked = true;
			}
			if (IsChecked)
			{
				EnabledCollections.Add(InArgs._Collections[i]);
				CollectionCheckbox->SetIsChecked(ECheckBoxState::Checked);
			}
		}
	}
	else
	{
		FilterCollections->AddSlot()
		[
			SNew(STextBlock)
			.Margin(FMargin(10.f, 5.f, 0.f, 5.f))
			.Text(FText::FromString("No collections defined, whole file will be imported"))
			.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
			.ColorAndOpacity(FSlateColor(FSlateColor::UseSubduedForeground()))
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

EAppReturnType::Type SBlendAssetImportDialog::ShowModal()
{
	GEditor->EditorAddModalWindow(SharedThis(this));
	return UserResponse;
}

bool SBlendAssetImportDialog::IsUseObjectPivot() const
{
	return UseObjectPivot;
}

TArray<FString> SBlendAssetImportDialog::GetEnabledCollections() const
{
	return EnabledCollections;
}

FReply SBlendAssetImportDialog::OnButtonClick(EAppReturnType::Type ButtonID)
{
	UserResponse = ButtonID;
	RequestDestroyWindow();

	return FReply::Handled();
}

void SBlendAssetImportDialog::OnCheckStateCollectionChanged(ECheckBoxState InCheckState, int32 Idx)
{
	if (InCheckState == ECheckBoxState::Checked)
	{
		EnabledCollections.Add(Collections[Idx]);
	}
	else
	{
		EnabledCollections.Remove(Collections[Idx]);
	}
}
