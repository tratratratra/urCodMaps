// Copyright 2022 nuclearfriend

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BlendImporterSettings.generated.h"

UCLASS(config = BlendImporterSettings)
class UBlendImporterSettings : public UObject
{
	GENERATED_BODY()

public:
	UBlendImporterSettings(const FObjectInitializer& obj);

	FFilePath GetBlenderExecutable() const;
	bool IsRunInBackground() const;
	bool IsDebug() const;
	bool IsFactoryStartup() const;
	bool IsFixMaterials() const;
	double GetUnresponsiveWarningDuration() const;

	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:
	void SanitizeBlenderPathInline(FString& Path);

private:
	/** Path to your Blender executable. */
	UPROPERTY(Config, EditAnywhere, Category="External Tools", meta=(FilePathFilter = "exe", DisplayName = "Blender Executable Path"))
	FFilePath BlenderExecutable;

	/** Run Blender in background mode (no-UI)? This is preferred but may cause issues with some Blender setups. */
	UPROPERTY(Config, EditAnywhere, Category="Options", meta=(DisplayName = "Run Blender In Background"))
	bool bRunInBackground = true;

	/** Fix some common problems with Blender materials when importing into Unreal */
	UPROPERTY(Config, EditAnywhere, Category="Options", meta=(DisplayName = "Fix Materials"))
	bool bFixMaterials = true;

	/** How long (in seconds) before considering a running Blender process unresponsive? */
	UPROPERTY(Config, EditAnywhere, Category="Options", meta=(DisplayName = "Unresponsive Warning Duration (s)"))
	double UnresponsiveWarningDuration = 15.0;

	/** Runs Blender in debug mode, increasing debug output for problems. Enable this is if you are having issues. */
	UPROPERTY(Config, EditAnywhere, Category="Debug", meta=(DisplayName = "Blender - Debug mode"))
	bool bDebug = false;

	/** Run Blender in factory startup mode, resetting Blender to defaults when running (no addons, no startup scene, etc). Enable this is if you are having issues. */
	UPROPERTY(Config, EditAnywhere, Category="Debug", meta=(DisplayName = "Blender - Factory Startup mode"))
	bool bFactoryStartup = false;
};
