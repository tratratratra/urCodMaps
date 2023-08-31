// Copyright 2022 nuclearfriend

#include "BlendImporterSettings.h"

#define LOCTEXT_NAMESPACE "BlendImporterSettings"

#if PLATFORM_WINDOWS

    #include "Windows/WindowsPlatformMisc.h"
    #include "Windows/AllowWindowsPlatformTypes.h"
    #include <Winreg.h>
    #include "Windows/HideWindowsPlatformTypes.h"

    static bool ReadRegistryValue(const Windows::HKEY RootKey, const FString& KeyName, const FString& ValueName, FString& Value)
    {
        HKEY hKey;
        LONG Result = RegOpenKeyEx(RootKey, *KeyName, 0, KEY_READ, &hKey);
        if (Result != ERROR_SUCCESS)
        { 
            return false;
        }

        TCHAR Buffer[512];
        DWORD BufferSize = sizeof(Buffer);
        HRESULT hResult = RegQueryValueEx(hKey, *ValueName, 0, nullptr, reinterpret_cast<LPBYTE>(Buffer), &BufferSize);
        if (hResult != ERROR_SUCCESS)
        {
            return false;
        }
        
        Value = Buffer;
        return true;
    }

#endif


UBlendImporterSettings::UBlendImporterSettings(const FObjectInitializer& obj)
{
    //
}

FFilePath UBlendImporterSettings::GetBlenderExecutable() const
{
    FText ErrorMessage;
    if (BlenderExecutable.FilePath.IsEmpty())
    {
        ErrorMessage = LOCTEXT("BlenderPathNotSet", "Your path to Blender is not set. Would you like to be taken to the project settings to set it now?");
    }
    else if (!BlenderExecutable.FilePath.EndsWith(TEXT("blender.exe")))
    {
        ErrorMessage = LOCTEXT("BlenderPathNotExe", "You must set a path to a valid Blender executable (blender.exe). Would you like to be taken to your project settings to fix it now?");
    }

    if (!ErrorMessage.IsEmpty())
    {
        // If an error was shown, give an option to jump to the project settings to fix the path
        const FText ErrorTitle = LOCTEXT("BlendImporterError", "Blend Importer Error");
        if (FMessageDialog::Open(EAppMsgType::YesNo, ErrorMessage, &ErrorTitle) == EAppReturnType::Yes)
        {
            if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
            {
                SettingsModule->ShowViewer("Project", "Plugins", "BlendImporter");
            }
        }
        return FFilePath();
    }

    return BlenderExecutable;
}

bool UBlendImporterSettings::IsRunInBackground() const
{
    return bRunInBackground;
}

bool UBlendImporterSettings::IsDebug() const
{
    return bDebug;
}

bool UBlendImporterSettings::IsFactoryStartup() const
{
    return bFactoryStartup;
}

double UBlendImporterSettings::GetUnresponsiveWarningDuration() const
{
    return UnresponsiveWarningDuration;
}

bool UBlendImporterSettings::IsFixMaterials() const
{
    return bFixMaterials;
}

void UBlendImporterSettings::PostInitProperties()
{
    Super::PostInitProperties();

    // On Windows, attempt to use registry to find the default Blender installation. If this fails, user can just set manually later.
    #if PLATFORM_WINDOWS
        if (BlenderExecutable.FilePath.IsEmpty())
        {
            FString BlenderPath;
            if (ReadRegistryValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\blendfile\\shell\\open\\command"), TEXT(""), BlenderPath))
            {
                BlenderExecutable.FilePath = BlenderPath;
            }
        }
    #endif

    SanitizeBlenderPathInline(BlenderExecutable.FilePath);
}

void UBlendImporterSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SanitizeBlenderPathInline(BlenderExecutable.FilePath);
}

void UBlendImporterSettings::SanitizeBlenderPathInline(FString& Path)
{
    int exeIdx = Path.Find(TEXT(".exe"));
    Path.LeftInline(exeIdx + 4);

    Path.TrimStartAndEndInline();
    Path.TrimQuotesInline();
    Path.TrimStartAndEndInline();

    if (Path.EndsWith(TEXT("-launcher.exe")))
    {
        UE_LOG(LogBlendImporter, Log, TEXT("Tried to set executable path to blender-launcher.exe, automatically modifying to blender.exe."));
        Path.ReplaceInline(TEXT("-launcher.exe"), TEXT(".exe"));
    }
}

#undef LOCTEXT_NAMESPACE