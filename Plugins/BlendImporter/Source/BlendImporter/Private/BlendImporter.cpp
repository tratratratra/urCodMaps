// Copyright 2022 nuclearfriend

#include "BlendImporter.h"
#include "BlendImporterSettings.h"
#include "ContentBrowserModule.h"
#include "ISettingsModule.h"
#include "MessageLogInitializationOptions.h"
#include "MessageLogModule.h"

#if ENGINE_MAJOR_VERSION < 5
    #include "EditorStyleSet.h"
#endif

DEFINE_LOG_CATEGORY(LogBlendImporter);

#define LOCTEXT_NAMESPACE "BlendImporterModule"

void FBlendImporterModule::StartupModule()
{
    RegisterSettings();
    RegisterContentBrowserAssetMenuExtender();
    RegisterMessageLog();
}

void FBlendImporterModule::ShutdownModule()
{
    UnregisterSettings();
    UnregisterContentBrowserAssetMenuExtender();
    UnregisterMessageLog();
}

void FBlendImporterModule::RegisterSettings()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "BlendImporter",
			LOCTEXT("RuntimeSettingsName", "Blend File Importer"), LOCTEXT("RuntimeSettingsDescription", "Configure Blend File Importer settings"),
			GetMutableDefault<UBlendImporterSettings>());
	}
}

void FBlendImporterModule::UnregisterSettings()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "BlendImporter");
	}
}

void FBlendImporterModule::RegisterContentBrowserAssetMenuExtender()
{
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
    CBAssetMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FBlendImporterModule::OnExtendContentBrowserAssetSelectionMenu));
    ContentBrowserExtenderDelegateHandle = CBAssetMenuExtenderDelegates.Last().GetHandle();
}

void FBlendImporterModule::UnregisterContentBrowserAssetMenuExtender()
{
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
    CBAssetMenuExtenderDelegates.RemoveAll([this](const FContentBrowserMenuExtender_SelectedAssets& Delegate) { return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle; });
}

void FBlendImporterModule::RegisterMessageLog()
{
    FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	FMessageLogInitializationOptions InitOptions;
	InitOptions.bShowPages = false;
	InitOptions.bShowFilters = false;
	InitOptions.bAllowClear = true;
    #if ENGINE_MAJOR_VERSION >= 5
        InitOptions.bScrollToBottom = true;
    #endif
	MessageLogModule.RegisterLogListing("LogBlendImporter", LOCTEXT("BlendImporter", "Blend Importer"), InitOptions);
}

void FBlendImporterModule::UnregisterMessageLog()
{
	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		// unregister message log
		FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing("LogBlendImporter");
	}
}

TSharedRef<FExtender> FBlendImporterModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddMenuExtension(
		"ImportedAssetActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateStatic(&FBlendImporterModule::AddMenuExtenderBlendAssetImported, SelectedAssets)
	);
	return Extender;
}

bool CheckAssetImportDataIsBlend(UAssetImportData* AssetImportData)
{
    FString FileExtension = FPaths::GetExtension(AssetImportData->GetFirstFilename());
    return FileExtension.Equals(TEXT("blend"), ESearchCase::IgnoreCase);
}

FString GetAssetImportDataAbsoluteFilePath(UAssetImportData* AssetImportData, const UPackage* Outermost)
{
    return UAssetImportData::ResolveImportFilename(AssetImportData->GetFirstFilename(), Outermost);
}

void FBlendImporterModule::AddMenuExtenderBlendAssetImported(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets)
{
    TSet<FString> FilePaths;

    for (const FAssetData& SelectedAsset : SelectedAssets)
    {
        if (SelectedAsset.GetClass()->IsChildOf<UStaticMesh>())
        {
            UStaticMesh* Mesh = Cast<UStaticMesh>(SelectedAsset.GetAsset());
            if (Mesh && CheckAssetImportDataIsBlend(Mesh->AssetImportData))
            {
                FilePaths.Add(GetAssetImportDataAbsoluteFilePath(Mesh->AssetImportData, Mesh->GetOutermost()));
            }
        }

        if (SelectedAsset.GetClass()->IsChildOf<USkeletalMesh>())
        {
            USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(SelectedAsset.GetAsset());
            #if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 26
                if (SkeletalMesh && CheckAssetImportDataIsBlend(SkeletalMesh->AssetImportData))
                {
                    FilePaths.Add(GetAssetImportDataAbsoluteFilePath(SkeletalMesh->AssetImportData, SkeletalMesh->GetOutermost()));
                }
            #else
                if (SkeletalMesh && CheckAssetImportDataIsBlend(SkeletalMesh->GetAssetImportData()))
                {
                    FilePaths.Add(GetAssetImportDataAbsoluteFilePath(SkeletalMesh->GetAssetImportData(), SkeletalMesh->GetOutermost()));
                }
            #endif
        }

        if (SelectedAsset.GetClass()->IsChildOf<UAnimSequence>())
        {
            UAnimSequence* AnimSequence = Cast<UAnimSequence>(SelectedAsset.GetAsset());
            if (AnimSequence && CheckAssetImportDataIsBlend(AnimSequence->AssetImportData))
            {
                FilePaths.Add(GetAssetImportDataAbsoluteFilePath(AnimSequence->AssetImportData, AnimSequence->GetOutermost()));
            }
        }
    }

    if (FilePaths.Num() > 0)
    {
        MenuBuilder.BeginSection("BlenderAsset", FText::FromString("Blender Source"));
        {
            MenuBuilder.AddMenuEntry(
                FText::FromString("Open in Blender"),
                FText::FromString("Opens the source of this Mesh in Blender"),
                #if ENGINE_MAJOR_VERSION < 5
                    FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.OpenInExternalEditor"),
                #else
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.OpenInExternalEditor"),
                #endif
                FUIAction(FExecuteAction::CreateLambda([FilePaths]()
                {
                    OpenFilesInBlender(FilePaths.Array());
                })));
        }
        MenuBuilder.EndSection();
    }
}

void FBlendImporterModule::OpenFilesInBlender(const TArray<FString>& Filenames)
{
    for (auto Filename : Filenames)
    {
        OpenFileInBlender(Filename);
    }
}

void FBlendImporterModule::OpenFileInBlender(const FString& Filename)
{
    UBlendImporterSettings* Settings = GetMutableDefault<UBlendImporterSettings>();
    FFilePath BlenderExePath = Settings->GetBlenderExecutable();
    if (BlenderExePath.FilePath.IsEmpty())
    {
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("Opening \"%s\" in Blender..."), *Filename);
    FString BlenderParms = FString::Printf(TEXT("\"%s\""), *Filename);
    FPlatformProcess::CreateProc(*BlenderExePath.FilePath, *BlenderParms, true, false, false, nullptr, 0, nullptr, nullptr);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlendImporterModule, BlendImporter)