// Copyright 2022 nuclearfriend

#pragma once

#include "CoreMinimal.h"
#include "EditorReimportHandler.h"
#include "Factories/Factory.h"
#include "BlendAssetFactory.generated.h"

class UFbxFactory;

UCLASS()
class UBlendImportOptions : public UObject
{
	GENERATED_BODY()

public:
	bool bUseObjectPivot = false;
	TArray<FString> EnabledCollections;

	void SaveMetaData(UObject* Object) const;
	bool LoadMetaData(UObject* Object);

	FString ToString() const;
	bool FromString(const FString& String);
};

UCLASS()
class UBlendAssetFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UBlendAssetFactory(const FObjectInitializer& ObjectInitializer);

	// Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual bool DoesSupportClass(UClass * Class) override;
	virtual void CleanUp() override;
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	// End UFactory Interface

	// Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames)  override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	// End FReimportHandler Interface

private:
	bool RunScriptOnBlendFile(const FString& Filename, const FString& ScriptName, FString& Output);
	bool BlendFileAnalyse(const FString& Filename, TArray<FString>& Collections, FString& MaterialWarnings, bool& IsPacked);
	bool BlendFileExport(const FString& Filename, const bool& Unpack, FString& OutputFilename);
	bool CanReimportBlendAsset(UAssetImportData* AssetImportData, TArray<FString>& OutFilenames);
	EReimportResult::Type ReimportBlendAsset(UObject* Obj, UAssetImportData* AssetImportData);
	
	void AssetAddedEvent(const FAssetData& AssetData);

private:
	UPROPERTY()
	UFbxFactory* FbxFactory;

	FDelegateHandle AssetAddedEventHandle;
	TArray<UAnimSequence*> ImportedAnimations;

    UBlendImportOptions* ImportOptions;

	FString PreviousImportedFilename;
	FString PreviousImportOptionsString;
	FDateTime PreviousImportedTimeStamp;
	FMD5Hash PreviousImportedHash;
};
