#include "WFCSquareSampleLibrary.h"
#include "WFCMarchingSquares.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
	bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, Asset,
			*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}
}

UWFCSquareModuleSet* UWFCSquareSampleLibrary::CreateSquareSamples(UMaterialInterface* Material, bool bRebuildSamples)
{
	const FString Root = TEXT("/Game/Demos/WFC/MarchingSquares/");
	const FString SetPath = Root + TEXT("DA_SquareSamples");
	UWFCSquareModuleSet* Set = LoadObject<UWFCSquareModuleSet>(nullptr, *(SetPath + TEXT(".DA_SquareSamples")));
	if (Set && !bRebuildSamples) return Set;
	const bool bNewSet = !Set;
	if (!Set) Set = NewObject<UWFCSquareModuleSet>(CreatePackage(*SetPath), TEXT("DA_SquareSamples"), RF_Public | RF_Standalone);
	for (int32 Mask = 1; Mask < 16; ++Mask)
	{
		const FString Name = FString::Printf(TEXT("SM_Square_%02d"), Mask);
		const FString Path = Root + TEXT("Models/") + Name;
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *(Path + TEXT(".") + Name));
		const bool bNewMesh = !Mesh;
		if (!Mesh || bRebuildSamples)
		{
			if (!Mesh) Mesh = NewObject<UStaticMesh>(CreatePackage(*Path), *Name, RF_Public | RF_Standalone);
			Mesh->bAllowCPUAccess = true;
			Mesh->GetStaticMaterials().Reset();
			Mesh->GetStaticMaterials().Add(FStaticMaterial(Material));
			FMeshDescription Description;
			FStaticMeshAttributes Attributes(Description);
			Attributes.Register();
			auto Positions = Attributes.GetVertexPositions();
			auto Normals = Attributes.GetVertexInstanceNormals();
			auto Colors = Attributes.GetVertexInstanceColors();
			auto UVs = Attributes.GetVertexInstanceUVs();
			UVs.SetNumChannels(1);
			const FPolygonGroupID Group = Description.CreatePolygonGroup();
			Attributes.GetPolygonGroupMaterialSlotNames()[Group] = FName(TEXT("Surface"));
			const FWFCTownMeshData Sample = FWFCMarchingSquares::BuildSample(Mask);
			for (int32 I = 0; I < Sample.Indices.Num(); I += 3)
			{
				TArray<FVertexInstanceID> Instances;
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const int32 Source = Sample.Indices[I + Corner];
					const FVertexID Vertex = Description.CreateVertex();
					Positions[Vertex] = FVector3f(Sample.Positions[Source]);
					const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
					Normals[Instance] = FVector3f(Sample.Normals[Source]);
					Colors[Instance] = FVector4f(Sample.Colors[Source]);
					UVs.Set(Instance, 0, FVector2f(Sample.UVs[Source]));
					Instances.Add(Instance);
				}
				Description.CreatePolygon(Group, Instances);
			}
			if (Mesh->GetNumSourceModels() == 0) Mesh->AddSourceModel();
			FStaticMeshSourceModel& SourceModel = Mesh->GetSourceModel(0);
			SourceModel.BuildSettings.bRecomputeNormals = false;
			SourceModel.BuildSettings.bRecomputeTangents = true;
			SourceModel.BuildSettings.bGenerateLightmapUVs = false;
			Mesh->CreateMeshDescription(0, MoveTemp(Description));
			Mesh->CommitMeshDescription(0);
			Mesh->Build(false);
			if (bNewMesh) FAssetRegistryModule::AssetCreated(Mesh);
			if (!SaveAsset(Mesh)) return nullptr;
		}
		Set->Modules[Mask] = Mesh;
	}
	if (bNewSet) FAssetRegistryModule::AssetCreated(Set);
	return SaveAsset(Set) ? Set : nullptr;
}
