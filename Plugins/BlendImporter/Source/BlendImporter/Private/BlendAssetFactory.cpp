// Copyright 2022 nuclearfriend

#include "BlendAssetFactory.h"
#include "BlendImporter.h"
#include "BlendImporterSettings.h"
#include "SBlendAssetImportDialog.h"
#include "AssetRegistryModule.h"
#include "DesktopPlatformModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Factories/FbxFactory.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IAssetRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/MetaData.h"

#define LOCTEXT_NAMESPACE "BlendAssetFactory"

void UBlendImportOptions::SaveMetaData(UObject* Obj) const
{
    FString Data = ToString();
    Obj->GetPackage()->GetMetaData()->SetValue(Obj, TEXT("BLEND_IMPORT"), *Data);
}

bool UBlendImportOptions::LoadMetaData(UObject* Obj)
{
    FString Data = Obj->GetPackage()->GetMetaData()->GetValue(Obj, TEXT("BLEND_IMPORT"));
    return FromString(Data);
}

FString UBlendImportOptions::ToString() const
{
    FString Data;
    Data.Append(bUseObjectPivot ? TEXT("true") : TEXT("false"));
    Data.Append(TEXT(";"));
    Data.Append(*FString::Join(EnabledCollections, TEXT(",")));
    return Data;
}

bool UBlendImportOptions::FromString(const FString& Data)
{
	TArray<FString> Params;
	const int32 nArraySize = Data.ParseIntoArray(Params, TEXT(";"), false);
    if (nArraySize == 2)
    {
        bUseObjectPivot = Params[0].ToBool();
        if (!Params[1].IsEmpty())
        {
            Params[1].ParseIntoArray(EnabledCollections, TEXT(","), true);
        }
        return true;
    }
    else
    {
        // TODO: Throw error, force re-import etc
        return false;
    }
}

UBlendAssetFactory::UBlendAssetFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	Formats.Add("blend;Blend File");
	bCreateNew = false;
	bEditorImport = true;
    FReimportManager::Instance()->RegisterHandler(*this);
	SupportedClass = UStaticMesh::StaticClass(); // TODO: Figure out if this is a problem, when this also supports SkeletalMesh etc?
    FbxFactory = NewObject<UFbxFactory>(UFbxFactory::StaticClass());
    ImportOptions = GetMutableDefault<UBlendImportOptions>();
}

bool UBlendAssetFactory::ConfigureProperties()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	AssetAddedEventHandle = AssetRegistry.OnAssetAdded().AddUObject(this, &UBlendAssetFactory::AssetAddedEvent);

    FbxFactory->ConfigureProperties();

    return true;
}

bool UBlendAssetFactory::DoesSupportClass(UClass * Class)
{
	return (Class == UStaticMesh::StaticClass() || Class == USkeletalMesh::StaticClass() || Class == UAnimSequence::StaticClass());
}

void UBlendAssetFactory::CleanUp()
{
    FbxFactory->CleanUp();

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	AssetRegistry.OnAssetAdded().Remove(AssetAddedEventHandle);

	Super::CleanUp();
}

UObject* UBlendAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UObject *ExistingObject = nullptr;
	if (InParent != nullptr)
	{
		ExistingObject = StaticFindObject(UObject::StaticClass(), InParent, *(InName.ToString()));
    }

    bool bLoadedImportOptions = false;

    if (ExistingObject != nullptr)
    {
        if (ImportOptions->LoadMetaData(ExistingObject))
        {        
            bLoadedImportOptions = true;
        }
        else
        {
            UE_LOG(LogBlendImporter, Warning, TEXT("There was an issue loading the metadata for this asset to allow re-import. Showing options again."));
        }
    }

    bool IsPacked = false;

    TArray<FString> Collections;
    FString MaterialWarnings;
    if (BlendFileAnalyse(Filename, Collections, MaterialWarnings, IsPacked) == false)
    {
        return nullptr;
    }

    auto MessageLog = FMessageLog(FName("LogBlendImporter"));
    if (!MaterialWarnings.IsEmpty())
    {        
        auto WarningText = FText::Format(LOCTEXT("MaterialIssues", "Material issues were detected in '{0}', which may make your materials not display properly in Unreal.\n"
                                                                    "You can find an explanation of these issues in the Blend Importer documentation.\n"
                                                                    "Issues:\n"
                                                                    "{1}"), FText::FromString(Filename), FText::FromString(MaterialWarnings));

        auto TokenizedWarning = FTokenizedMessage::Create(EMessageSeverity::Warning, WarningText);

        auto DocumentationToken = FDocumentationToken::Create("https://www.notion.so/jonmurphy/Blend-File-Importer-Plugin-for-Unreal-897316df448d46528c28126874a8f40e");
        TokenizedWarning->AddToken(DocumentationToken);

        MessageLog.AddMessage(TokenizedWarning);
    }

    if (IsPacked)
    {
        MessageLog.Warning(FText::Format(LOCTEXT("PackedTextures", "Packed textures have been detected in '{0}', which will inflate the size of the FBX interchange data and increase import times.\n"
                                                                    "Consider unpacking those resources from your .blend file before importing."), FText::FromString(Filename)));
    }

    if ((!MaterialWarnings.IsEmpty() || IsPacked))
    {
        // Don't open message log on re-import, to avoid log spam and at this point they're prolly ignoring these warnings anyway
        if (ExistingObject == nullptr)
        {
            MessageLog.Open(EMessageSeverity::Warning, false);
        }
    }
    
    if (bLoadedImportOptions == false)
    {
        TSharedRef<SBlendAssetImportDialog> ImportDialog =
            SNew(SBlendAssetImportDialog)
            .Filename(FText::FromString(*Filename))
            .Collections(Collections)
            .PreviousOptions(ImportOptions)
            .ShowMaterialWarning(!MaterialWarnings.IsEmpty());

        if (ImportDialog->ShowModal() == EAppReturnType::Cancel)
        {
            return nullptr;
        }

        ImportOptions->bUseObjectPivot = ImportDialog->IsUseObjectPivot();
        ImportOptions->EnabledCollections = ImportDialog->GetEnabledCollections();
    }

    FString OutputFilename;
    if (BlendFileExport(Filename, IsPacked, OutputFilename) == false)
    {
        return nullptr;
    }

    UE_LOG(LogBlendImporter, Log, TEXT("Importing FBX..."));

    // HACK: Temporarily disable notification manager so we don't see the "FBX Imported" double notification as well as the ".blend Imported"
    FSlateNotificationManager::Get().SetAllowNotifications(false);
    UObject* MainObject = StaticImportObject(InClass, InParent, InName, Flags, *OutputFilename, nullptr, FbxFactory, Parms, Warn);
    FSlateNotificationManager::Get().SetAllowNotifications(true);

	TArray<UObject*> ImportedObjects;
    if (MainObject)
    {
        ImportedObjects.Add(MainObject);
    }

    for (UObject* AdditionalObject : FbxFactory->GetAdditionalImportedObjects())
    {
        ImportedObjects.Add(AdditionalObject);
    }

    // Update source file metadata for imported meshes
    for (UObject* ImportedObject : ImportedObjects)
    {
        UStaticMesh* Mesh = Cast<UStaticMesh>(ImportedObject);
        if (Mesh)
        {
            Mesh->AssetImportData->Update(UAssetImportData::SanitizeImportFilename(Filename, Mesh->GetOutermost()));

            ImportOptions->SaveMetaData(Mesh);
        }

        USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(ImportedObject);
        if (SkeletalMesh)
        {
            #if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 26
                SkeletalMesh->AssetImportData->Update(UAssetImportData::SanitizeImportFilename(Filename, SkeletalMesh->GetOutermost()));
            #else
                SkeletalMesh->GetAssetImportData()->Update(UAssetImportData::SanitizeImportFilename(Filename, SkeletalMesh->GetOutermost()));
            #endif

            ImportOptions->SaveMetaData(SkeletalMesh);
        }
        
    }

    // Update source file metadata for imported animations
    for (UAnimSequence* AnimSequence : ImportedAnimations)
    {
        AnimSequence->AssetImportData->Update(UAssetImportData::SanitizeImportFilename(Filename, AnimSequence->GetOutermost()));
    }
    ImportedAnimations.Empty();
    
    return MainObject;
}

bool UBlendAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
	if (Mesh)
	{
        return CanReimportBlendAsset(Mesh->AssetImportData, OutFilenames);
	}

    USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Obj);
	if (SkeletalMesh)
	{
        #if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 26
            return CanReimportBlendAsset(SkeletalMesh->AssetImportData, OutFilenames);
        #else
            return CanReimportBlendAsset(SkeletalMesh->GetAssetImportData(), OutFilenames);
        #endif
	}

    UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj);
	if (AnimSequence)
	{
        return CanReimportBlendAsset(AnimSequence->AssetImportData, OutFilenames);
	}

	return false;
}

void UBlendAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
    if (Mesh && ensure(NewReimportPaths.Num() == 1))
    {
        Mesh->Modify();
        Mesh->AssetImportData->Update(UAssetImportData::SanitizeImportFilename(NewReimportPaths[0], Mesh->GetOutermost()));
    }

    USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Obj);
    if (SkeletalMesh && ensure(NewReimportPaths.Num() == 1))
    {
        SkeletalMesh->Modify();
        
        #if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 26
            SkeletalMesh->AssetImportData->Update(UAssetImportData::SanitizeImportFilename(NewReimportPaths[0], SkeletalMesh->GetOutermost()));
        #else
            SkeletalMesh->GetAssetImportData()->Update(UAssetImportData::SanitizeImportFilename(NewReimportPaths[0], SkeletalMesh->GetOutermost()));
        #endif
    }

    UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj);
    if (AnimSequence && ensure(NewReimportPaths.Num() == 1))
    {
        AnimSequence->Modify();
        AnimSequence->AssetImportData->Update(UAssetImportData::SanitizeImportFilename(NewReimportPaths[0], AnimSequence->GetOutermost()));
    }
}

EReimportResult::Type UBlendAssetFactory::Reimport(UObject* Obj)
{
    UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
	if (Mesh)
	{
		return ReimportBlendAsset(Mesh, Mesh->AssetImportData);
	}

    USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Obj);
	if (SkeletalMesh)
	{
        #if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 26
            return ReimportBlendAsset(SkeletalMesh, SkeletalMesh->AssetImportData);
        #else
           return ReimportBlendAsset(SkeletalMesh, SkeletalMesh->GetAssetImportData());
        #endif
	}

    UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj);
	if (AnimSequence)
	{
        // HACK: Reimport doesn't report the asset as "added", so we should just manually add it here for now. Perhaps a better event can be used? (OnAssetUpdated??)
        ImportedAnimations.Add(AnimSequence);
        UE_LOG(LogBlendImporter, Log, TEXT("Animation '%s' was imported."), *AnimSequence->GetName());

		return ReimportBlendAsset(AnimSequence, AnimSequence->AssetImportData);
	}

    return EReimportResult::Failed;
}

int32 UBlendAssetFactory::GetPriority() const
{
	return UFactory::GetDefaultImportPriority() * 2;
}

bool UBlendAssetFactory::RunScriptOnBlendFile(const FString& Filename, const FString& ScriptName, FString& Output)
{
    UBlendImporterSettings* Settings = GetMutableDefault<UBlendImporterSettings>();
    FFilePath BlenderExePath = Settings->GetBlenderExecutable();
    if (BlenderExePath.FilePath.IsEmpty())
    {
        UE_LOG(LogBlendImporter, Error, TEXT("Blender executable has not been set."));
        return false;
    }

    FString FullPathFileName = FPaths::ConvertRelativePathToFull(Filename);

    // Create process to execute Blender
    FString PluginPath = IPluginManager::Get().FindPlugin(TEXT("BlendImporter"))->GetBaseDir();
    FString BlenderScriptPath = PluginPath + FString::Printf(TEXT("/Scripts/%s.py"), *ScriptName);

    FString BlenderParameters = TEXT("-noaudio --python-exit-code 1");

    if (Settings->IsDebug())
    {
        BlenderParameters += TEXT(" -d");
    }

    if (Settings->IsFactoryStartup())
    {
        BlenderParameters += TEXT(" --factory-startup");
    }

    if (Settings->IsRunInBackground())
    {
        BlenderParameters += TEXT(" -b");
    }
    else
    {
        BlenderParameters += TEXT(" -w --no-window-focus");
    }
    
    FString BlenderProcessParms = FString::Printf(TEXT("%s \"%s\" -P \"%s\""), *BlenderParameters, *FullPathFileName, *BlenderScriptPath);
    
    void* ReadPipe = nullptr;
	void* WritePipe = nullptr;
    if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
	{
        UE_LOG(LogBlendImporter, Error, TEXT("Failed to create Pipes for Blender process"));
		return nullptr;
	}
	check(ReadPipe);
	check(WritePipe);

    FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*BlenderExePath.FilePath, *BlenderProcessParms, /* bLaunchDetached = */ false, /* bLaunchHidden = */ true, /* bLaunchReallyHidden = */ true, nullptr, 0, nullptr, WritePipe, ReadPipe);

    bool bResult = false;
    if (ProcessHandle.IsValid())
    {
        UE_LOG(LogBlendImporter, Log, TEXT("Running %s %s"), *BlenderExePath.FilePath, *BlenderProcessParms);

        bool bTerminated = false;

        double UnresponsiveWarningTime = FPlatformTime::Seconds() + Settings->GetUnresponsiveWarningDuration();
        while (FPlatformProcess::IsProcRunning(ProcessHandle))
		{
            if (FPlatformTime::Seconds() > UnresponsiveWarningTime)
            {
                const FText ErrorTitle = LOCTEXT("BlendImporterError", "Blend Importer Error");
                const FText ErrorMessage = LOCTEXT("BlenderSlow", "Blender does not appear to be responding, or is taking a while to process your file.\n\nIf you are working with particularly large files or utilise a lot of Blender addons, you can try increasing the duration before this warning is triggered in the settings.\n\nAlternatively, if your Blender has become unresponsive, there may be a problem with your Blender setup and you should consult the Blend Importer Plugin documentation for solutions.\n\nEither way, would you like to try waiting a bit more for Blender to complete it's task?");
                
                if (FMessageDialog::Open(EAppMsgType::YesNo, ErrorMessage, &ErrorTitle) == EAppReturnType::Yes)
                {
                    UnresponsiveWarningTime = FPlatformTime::Seconds() + Settings->GetUnresponsiveWarningDuration();
                }
                else
                {
                    UE_LOG(LogBlendImporter, Warning, TEXT("Terminating Blender before completion"));
                    bTerminated = true;
                    break;
                }
            }
		    Output += FPlatformProcess::ReadPipe(ReadPipe);
            FPlatformProcess::Sleep(0.1f);
		}

        // Wait for console process to complete as well
        if (bTerminated == false)
        {
            FPlatformProcess::WaitForProc(ProcessHandle);
            Output += FPlatformProcess::ReadPipe(ReadPipe);
        }
        
        int32 ReturnCode = 0;
        FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode);
        
        UE_LOG(LogBlendImporter, Log, TEXT("Blender Output:\n%s\nReturn Code: %d"), *Output, ReturnCode);
        bResult = !bTerminated;
    }
    else
    {
        UE_LOG(LogBlendImporter, Error, TEXT("There was an issue running Blender \"(%s)\". Check the path to the executable in your project settings."), *BlenderExePath.FilePath);
        bResult = false;
    }

    if (bResult)
    {
        UE_LOG(LogBlendImporter, Log, TEXT("Blender Execution - Complete"));
    }
    else
    {
        UE_LOG(LogBlendImporter, Error, TEXT("Blender Execution - Failed"));
    }

	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    FPlatformProcess::TerminateProc(ProcessHandle);
    FPlatformProcess::CloseProc(ProcessHandle);
    return bResult;
}

bool UBlendAssetFactory::BlendFileAnalyse(const FString& Filename, TArray<FString>& Collections, FString& MaterialWarnings, bool& IsPacked)
{
    FString Output;
    if (RunScriptOnBlendFile(Filename, "blender_analyse", Output) == false)
    {
        return false;
    }

	TArray<FString> OutputLines;
	const int32 nArraySize = Output.ParseIntoArray(OutputLines, TEXT("\n"), true);
    for (FString Line : OutputLines)
    {
	    TArray<FString> Params;
	    Line.ParseIntoArray(Params, TEXT("|"), true);
        if (Params.Num() <= 1)
            continue;

        if (Params[0].Len() != 1)
            continue;
        
        switch ((*Params[0])[0])
        {
            case 'C':
                Params[1].LeftChopInline(2);
	            Params[1].ParseIntoArray(Collections, TEXT(","), true);
                break;

            case 'P':
                IsPacked = true;
                break;
            
            case 'M':
                Params[2].LeftChopInline(2);
                Params[2].ReplaceInline(TEXT(","), TEXT(", "));
                MaterialWarnings += FString::Printf(TEXT("\t%s: %s\n"), *Params[1], *Params[2]); 
                break;
        }
    }

    return true;
}

bool UBlendAssetFactory::BlendFileExport(const FString& Filename, const bool& Unpack, FString& OutputFilename)
{
    UE_LOG(LogBlendImporter, Log, TEXT("Exporting FBX from Blender..."));

	auto UserTempDir = FPaths::ConvertRelativePathToFull(FDesktopPlatformModule::Get()->GetUserTempPath());
    OutputFilename = UserTempDir + FPaths::GetBaseFilename(Filename) + TEXT(".fbx");

    // HACK: We cache the last file, timestamp and hash, to prevent Blender from exporting the same
    //  file multiple times when processing a re-import for a modified file. Might be a better way to work around this..
    FDateTime TimeStamp = IFileManager::Get().GetTimeStamp(*Filename);
    FMD5Hash Hash = FMD5Hash::HashFile(*Filename);
    FString ImportOptionsString = ImportOptions->ToString();
    if (Filename == PreviousImportedFilename)
    {
        if (TimeStamp == PreviousImportedTimeStamp)
        {
            if (Hash == PreviousImportedHash)
            {
                if (ImportOptionsString == PreviousImportOptionsString)
                {
                    UE_LOG(LogBlendImporter, Log, TEXT("No source file changes detected, skipping export of FBX"));
                }
                return true;
            }
        }
    }

    PreviousImportedFilename = Filename;
    PreviousImportedTimeStamp = TimeStamp;
    PreviousImportedHash = Hash;
    PreviousImportOptionsString = ImportOptionsString;

    UBlendImporterSettings* Settings = GetMutableDefault<UBlendImporterSettings>();
    
    // Set envvar for export python script
    FPlatformMisc::SetEnvironmentVar(TEXT("UNREAL_IMPORTER_OUTPUT_FILE"), *OutputFilename);
    FPlatformMisc::SetEnvironmentVar(TEXT("UNREAL_IMPORTER_EXPORT_OBJECT_PIVOT"), ImportOptions->bUseObjectPivot ? TEXT("true") : TEXT("false"));
    FPlatformMisc::SetEnvironmentVar(TEXT("UNREAL_IMPORTER_FIX_MATERIALS"), Settings->IsFixMaterials() ? TEXT("true") : TEXT("false"));
    FPlatformMisc::SetEnvironmentVar(TEXT("UNREAL_IMPORTER_ENABLED_COLLECTIONS"), *FString::Join(ImportOptions->EnabledCollections, TEXT(",")));
    FPlatformMisc::SetEnvironmentVar(TEXT("UNREAL_IMPORTER_UNPACK"), Unpack ? TEXT("true") : TEXT("false"));

    FString Output;
    if (RunScriptOnBlendFile(Filename, "blender_export", Output) == false)
    {
        return false;
    }

    if (FPaths::FileExists(*OutputFilename) == false)
    {
        UE_LOG(LogBlendImporter, Error, TEXT("There was an issue while exporting the FBX from Blender."));
        return false;
    }

    return true;
}

bool UBlendAssetFactory::CanReimportBlendAsset(UAssetImportData* AssetImportData, TArray<FString>& OutFilenames)
{
    if (AssetImportData)
    {
        FString FileExtension = FPaths::GetExtension(AssetImportData->GetFirstFilename());
        const bool bIsValidFile = FileExtension.Equals(TEXT("blend"), ESearchCase::IgnoreCase);
        if (!bIsValidFile)
        {
            return false;
        }
        AssetImportData->ExtractFilenames(OutFilenames);
    }
    return true;
}

EReimportResult::Type UBlendAssetFactory::ReimportBlendAsset(UObject* Obj, UAssetImportData* AssetImportData)
{
    UE_LOG(LogBlendImporter, Log, TEXT("Re-importing Blend Asset: %s (%s)"), *Obj->GetName(), *Obj->GetClass()->GetName());

    if (!AssetImportData)
    {
        return EReimportResult::Failed;
    }

	bool OutCanceled = false;
    if (ImportObject(Obj->GetClass(), Obj->GetOuter(), *Obj->GetName(), RF_Public | RF_Standalone, *AssetImportData->GetFirstFilename(), *AssetImportData->GetFirstFilename(), OutCanceled) != nullptr)
	{
        Obj->MarkPackageDirty();
		return EReimportResult::Succeeded;
    }
    else if (OutCanceled)
	{
		UE_LOG(LogTemp, Warning, TEXT("-- Import canceled"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("-- Import failed"));
	}
	return EReimportResult::Failed;
}

void UBlendAssetFactory::AssetAddedEvent(const FAssetData& AssetData)
{
    if (AssetData.IsAssetLoaded())
    {
        if (UObject* AddedAsset = AssetData.GetAsset())
        {
            UAnimSequence* AnimSequence = Cast<UAnimSequence>(AddedAsset);
            if (AnimSequence)
            {
                UE_LOG(LogBlendImporter, Log, TEXT("Animation '%s' was imported."), *AnimSequence->GetName());
                ImportedAnimations.Add(AnimSequence);
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE