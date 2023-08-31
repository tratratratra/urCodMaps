// Copyright 2022 nuclearfriend

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAssetTools.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBlendImporter, Verbose, Verbose);

class FBlendImporterModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterSettings();
	void UnregisterSettings();

	void RegisterContentBrowserAssetMenuExtender();
	void UnregisterContentBrowserAssetMenuExtender();

	void RegisterMessageLog();
	void UnregisterMessageLog();

	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	static void AddMenuExtenderBlendAssetImported(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets);
	static void OpenFilesInBlender(const TArray<FString>& Filenames);
	static void OpenFileInBlender(const FString& Filename);

	FDelegateHandle ContentBrowserExtenderDelegateHandle;
};
