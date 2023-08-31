// Copyright 2022 nuclearfriend

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"

class UBlendImportOptions;

class SBlendAssetImportDialog : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SBlendAssetImportDialog)
		: _Filename()
		, _Collections()
		, _PreviousOptions()
		, _ShowMaterialWarning()
		{}

	SLATE_ARGUMENT( FText, Filename )
	SLATE_ARGUMENT( TArray<FString>, Collections )
	SLATE_ARGUMENT( UBlendImportOptions*, PreviousOptions )
	SLATE_ARGUMENT( bool, ShowMaterialWarning )

	SLATE_END_ARGS()

	SBlendAssetImportDialog()
		: UserResponse(EAppReturnType::Cancel)
	{}

	void Construct(const FArguments& InArgs);

public:
	/** Displays the dialog in a blocking fashion */
	EAppReturnType::Type ShowModal();

	bool IsUseObjectPivot() const;
	TArray<FString> GetEnabledCollections() const;

protected:
	FReply OnButtonClick(EAppReturnType::Type ButtonID);
	void OnCheckStateCollectionChanged(ECheckBoxState InCheckState, int32 Idx);

private:
    TSharedPtr<STextBlock> ComboBoxTitleBlock;
    TArray<TSharedPtr<FString>> ComboBoxItems;

	EAppReturnType::Type UserResponse;
	TArray<FString> Collections;

	TArray<FString> EnabledCollections;
	bool UseObjectPivot;
};
