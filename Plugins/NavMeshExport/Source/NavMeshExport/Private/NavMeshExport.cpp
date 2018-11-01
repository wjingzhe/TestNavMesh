// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NavMeshExport.h"
#include "NavMeshExportStyle.h"
#include "NavMeshExportCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "LevelEditor.h"

#include "Runtime/Navmesh/Public/Detour/DetourNavMesh.h"
#include "Runtime/Engine/Classes/AI/Navigation/NavigationTypes.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/NavigationSystem/Public/NavMesh/RecastNavMesh.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "HAL/PlatformFilemanager.h"
/** The editor object. */
extern UNREALED_API class UEditorEngine* GEditor;

static const FName NavMeshExportTabName("NavMeshExport");

#define LOCTEXT_NAMESPACE "FNavMeshExportModule"

void FNavMeshExportModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FNavMeshExportStyle::Initialize();
	FNavMeshExportStyle::ReloadTextures();

	FNavMeshExportCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FNavMeshExportCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FNavMeshExportModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FNavMeshExportModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FNavMeshExportModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
}

void FNavMeshExportModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FNavMeshExportStyle::Shutdown();

	FNavMeshExportCommands::Unregister();
}

struct NavMeshSetHeader
{
	int magicNum;//jingz 用于校对导出库版本，根据原生的NavmeshCast库或Unreal引擎库的数值设定
	int version;
	int numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

void FNavMeshExportModule::PluginButtonClicked()
{
	// Put your "OnButtonClicked" stuff here
	FText DialogText = FText::Format(
							LOCTEXT("PluginButtonDialogText", "Add code to {0} in {1} to override this button's actions"),
							FText::FromString(TEXT("FNavMeshExportModule::PluginButtonClicked()")),
							FText::FromString(TEXT("NavMeshExport.cpp"))
					   );
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);


	UWorld* World = GEditor->GetEditorWorldContext().World();

	auto CurrentNavSys = UNavigationSystemV1::GetCurrent(World);

	if (!CurrentNavSys)
	{
		return;
	}

	const ANavigationData* NavigationData = CurrentNavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);

	if (!NavigationData)
	{
		// 主Level没有对应的NavMeshVolume
		return;
	}

	const ARecastNavMesh* RecastMesh = Cast<const ARecastNavMesh>(NavigationData);
	if (!RecastMesh)
	{
		return;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString ExporBinFileName = FPaths::ProjectSavedDir() + GEditor->GetEditorWorldContext().World()->GetMapName();
	ExporBinFileName += L".NavmeshBin";
	IFileHandle* FileHandle = PlatformFile.OpenWrite(*ExporBinFileName);
	if (nullptr == FileHandle)
	{
		//文件打开失败
		return;
	}


	const dtNavMesh* DetourNavMesh = RecastMesh->GetRecastMesh();

	// Store header.
	NavMeshSetHeader header;
	header.magicNum = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = 0;
	for (int i = 0; i < DetourNavMesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = DetourNavMesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		header.numTiles++;
	}
	memcpy(&header.params, DetourNavMesh->getParams(), sizeof(dtNavMeshParams));
	FileHandle->Write(reinterpret_cast<const uint8_t*>(&header), sizeof(header));

	// Store tiles.
	for (int i = 0; i < DetourNavMesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = DetourNavMesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = DetourNavMesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;

		FileHandle->Write(reinterpret_cast<const uint8_t*>(&tileHeader), sizeof(tileHeader));
		FileHandle->Write(reinterpret_cast<const uint8_t*>(tile->data), tile->dataSize);

	}

	FileHandle->Flush();
	delete FileHandle;

	return;
}

void FNavMeshExportModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FNavMeshExportCommands::Get().PluginAction);
}

void FNavMeshExportModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FNavMeshExportCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNavMeshExportModule, NavMeshExport)