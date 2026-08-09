#include "AK74UFbxFixCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
THIRD_PARTY_INCLUDES_START
#include <fbxsdk.h>
THIRD_PARTY_INCLUDES_END

namespace
{
	bool ContainsSkeleton(FbxNode* Node)
	{
		if(!Node)return false;
		if(FbxNodeAttribute* Attribute=Node->GetNodeAttribute())
			if(Attribute->GetAttributeType()==FbxNodeAttribute::eSkeleton)return true;
		for(int32 ChildIndex=0;ChildIndex<Node->GetChildCount();++ChildIndex)
			if(ContainsSkeleton(Node->GetChild(ChildIndex)))return true;
		return false;
	}

	void LogSkeletonRoots(FbxNode* Node,bool bHasSkeletonParent)
	{
		if(!Node)return;
		const bool bIsSkeleton=Node->GetNodeAttribute()
			&&Node->GetNodeAttribute()->GetAttributeType()==FbxNodeAttribute::eSkeleton;
		if(bIsSkeleton&&!bHasSkeletonParent)
			UE_LOG(LogTemp,Display,TEXT("AK74U source skeleton root: %s"),UTF8_TO_TCHAR(Node->GetName()));
		for(int32 ChildIndex=0;ChildIndex<Node->GetChildCount();++ChildIndex)
			LogSkeletonRoots(Node->GetChild(ChildIndex),bHasSkeletonParent||bIsSkeleton);
	}

	void LogImportantNodes(FbxNode* Node)
	{
		if(!Node)return;
		if(FbxNodeAttribute* Attribute=Node->GetNodeAttribute())
		{
			const FbxNodeAttribute::EType Type=Attribute->GetAttributeType();
			if(Type==FbxNodeAttribute::eMesh||Type==FbxNodeAttribute::eCamera)
			{
				const FbxAMatrix Transform=Node->EvaluateGlobalTransform(FBXSDK_TIME_ZERO);
				const FbxVector4 Location=Transform.GetT();
				const FbxVector4 Rotation=Transform.GetR();
				const FbxVector4 Scale=Transform.GetS();
				UE_LOG(LogTemp,Display,TEXT("AK74U FBX node %s type=%s location=(%.3f,%.3f,%.3f) rotation=(%.3f,%.3f,%.3f) scale=(%.3f,%.3f,%.3f)"),
					UTF8_TO_TCHAR(Node->GetName()),Type==FbxNodeAttribute::eMesh?TEXT("Mesh"):TEXT("Camera"),
					Location[0],Location[1],Location[2],Rotation[0],Rotation[1],Rotation[2],Scale[0],Scale[1],Scale[2]);
			}
		}
		for(int32 ChildIndex=0;ChildIndex<Node->GetChildCount();++ChildIndex)
			LogImportantNodes(Node->GetChild(ChildIndex));
	}

	void LogMeshSkin(FbxNode* Node)
	{
		if(!Node)return;
		if(FbxMesh* Mesh=Node->GetMesh())
		{
			UE_LOG(LogTemp,Display,TEXT("AK74U skin mesh %s: control-points=%d polygons=%d skins=%d"),
				UTF8_TO_TCHAR(Node->GetName()),Mesh->GetControlPointsCount(),Mesh->GetPolygonCount(),
				Mesh->GetDeformerCount(FbxDeformer::eSkin));
			for(int32 SkinIndex=0;SkinIndex<Mesh->GetDeformerCount(FbxDeformer::eSkin);++SkinIndex)
			{
				FbxSkin* Skin=static_cast<FbxSkin*>(Mesh->GetDeformer(SkinIndex,FbxDeformer::eSkin));
				if(!Skin)continue;
				for(int32 ClusterIndex=0;ClusterIndex<Skin->GetClusterCount();++ClusterIndex)
				{
					FbxCluster* Cluster=Skin->GetCluster(ClusterIndex);
					if(!Cluster)continue;
					double TotalWeight=0.;
					for(int32 WeightIndex=0;WeightIndex<Cluster->GetControlPointIndicesCount();++WeightIndex)
						TotalWeight+=Cluster->GetControlPointWeights()[WeightIndex];
					UE_LOG(LogTemp,Display,TEXT("AK74U skin cluster mesh=%s bone=%s points=%d total-weight=%.2f"),
						UTF8_TO_TCHAR(Node->GetName()),UTF8_TO_TCHAR(Cluster->GetLink()?Cluster->GetLink()->GetName():"None"),
						Cluster->GetControlPointIndicesCount(),TotalWeight);
				}
			}
		}
		for(int32 ChildIndex=0;ChildIndex<Node->GetChildCount();++ChildIndex)LogMeshSkin(Node->GetChild(ChildIndex));
	}

	void PruneFirstPersonSleeveShoulders(FbxNode* Node)
	{
		if(!Node)return;
		FbxMesh* Mesh=Node->GetMesh();
		if(Mesh&&FCStringAnsi::Stricmp(Node->GetName(),"Ch08_Body")==0)
		{
			TArray<double> HandWeight;
			HandWeight.Init(0.,Mesh->GetControlPointsCount());
			for(int32 SkinIndex=0;SkinIndex<Mesh->GetDeformerCount(FbxDeformer::eSkin);++SkinIndex)
			{
				FbxSkin* Skin=static_cast<FbxSkin*>(Mesh->GetDeformer(SkinIndex,FbxDeformer::eSkin));
				if(!Skin)continue;
				for(int32 ClusterIndex=0;ClusterIndex<Skin->GetClusterCount();++ClusterIndex)
				{
					FbxCluster* Cluster=Skin->GetCluster(ClusterIndex);
					if(!Cluster||!Cluster->GetLink())continue;
					const FString BoneName=UTF8_TO_TCHAR(Cluster->GetLink()->GetName());
					if(!BoneName.Contains(TEXT("Hand")))continue;
					for(int32 WeightIndex=0;WeightIndex<Cluster->GetControlPointIndicesCount();++WeightIndex)
					{
						const int32 ControlPoint=Cluster->GetControlPointIndices()[WeightIndex];
						if(HandWeight.IsValidIndex(ControlPoint))
							HandWeight[ControlPoint]+=Cluster->GetControlPointWeights()[WeightIndex];
					}
				}
			}
			int32 RemovedPolygons=0;
			for(int32 PolygonIndex=Mesh->GetPolygonCount()-1;PolygonIndex>=0;--PolygonIndex)
			{
				double AverageWeight=0.;
				const int32 PolygonSize=Mesh->GetPolygonSize(PolygonIndex);
				for(int32 VertexIndex=0;VertexIndex<PolygonSize;++VertexIndex)
				{
					const int32 ControlPoint=Mesh->GetPolygonVertex(PolygonIndex,VertexIndex);
					if(HandWeight.IsValidIndex(ControlPoint))AverageWeight+=HandWeight[ControlPoint];
				}
				AverageWeight/=FMath::Max(1,PolygonSize);
				if(AverageWeight<.78)
				{
					Mesh->RemovePolygon(PolygonIndex);
					++RemovedPolygons;
				}
			}
			UE_LOG(LogTemp,Display,TEXT("AK74U cleaned FPS bare arms: removed %d forearm polygons, kept %d hand polygons"),
				RemovedPolygons,Mesh->GetPolygonCount());
		}
		if(Mesh&&FCStringAnsi::Stricmp(Node->GetName(),"Ch08_Hoodie")==0)
		{
			TArray<double> ForearmAndHandWeight;
			ForearmAndHandWeight.Init(0.,Mesh->GetControlPointsCount());
			for(int32 SkinIndex=0;SkinIndex<Mesh->GetDeformerCount(FbxDeformer::eSkin);++SkinIndex)
			{
				FbxSkin* Skin=static_cast<FbxSkin*>(Mesh->GetDeformer(SkinIndex,FbxDeformer::eSkin));
				if(!Skin)continue;
				for(int32 ClusterIndex=0;ClusterIndex<Skin->GetClusterCount();++ClusterIndex)
				{
					FbxCluster* Cluster=Skin->GetCluster(ClusterIndex);
					if(!Cluster||!Cluster->GetLink())continue;
					const FString BoneName=UTF8_TO_TCHAR(Cluster->GetLink()->GetName());
					if(!BoneName.Contains(TEXT("ForeArm"))&&!BoneName.Contains(TEXT("Hand")))continue;
					for(int32 WeightIndex=0;WeightIndex<Cluster->GetControlPointIndicesCount();++WeightIndex)
					{
						const int32 ControlPoint=Cluster->GetControlPointIndices()[WeightIndex];
						if(ForearmAndHandWeight.IsValidIndex(ControlPoint))
							ForearmAndHandWeight[ControlPoint]+=Cluster->GetControlPointWeights()[WeightIndex];
					}
				}
			}

			int32 RemovedPolygons=0;
			for(int32 PolygonIndex=Mesh->GetPolygonCount()-1;PolygonIndex>=0;--PolygonIndex)
			{
				double AverageWeight=0.;
				const int32 PolygonSize=Mesh->GetPolygonSize(PolygonIndex);
				for(int32 VertexIndex=0;VertexIndex<PolygonSize;++VertexIndex)
				{
					const int32 ControlPoint=Mesh->GetPolygonVertex(PolygonIndex,VertexIndex);
					if(ForearmAndHandWeight.IsValidIndex(ControlPoint))AverageWeight+=ForearmAndHandWeight[ControlPoint];
				}
				AverageWeight/=FMath::Max(1,PolygonSize);
				// The source garment continues all the way to the shoulders. Keep only
				// the part predominantly driven by forearm/hand bones, so its open
				// shoulder ends can never enter the first-person camera.
				if(AverageWeight<.88)
				{
					Mesh->RemovePolygon(PolygonIndex);
					++RemovedPolygons;
				}
			}
			UE_LOG(LogTemp,Display,TEXT("AK74U cleaned FPS sleeves: removed %d upper-arm polygons, kept %d"),
				RemovedPolygons,Mesh->GetPolygonCount());
		}
		for(int32 ChildIndex=0;ChildIndex<Node->GetChildCount();++ChildIndex)
			PruneFirstPersonSleeveShoulders(Node->GetChild(ChildIndex));
	}
}
#endif

UAK74UFbxFixCommandlet::UAK74UFbxFixCommandlet()
{
	IsClient=false;
	IsEditor=true;
	IsServer=false;
	LogToConsole=true;
}

int32 UAK74UFbxFixCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
	UE_LOG(LogTemp,Error,TEXT("AK74U FBX fixer is editor-only"));
	return 1;
#else
	const FString InputPath=FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir()/TEXT("SourceArt/AK74UFree/source/AK74U.fbx"));
	const FString OutputPath=FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir()/TEXT("SourceArt/AK74UFree/source/AK74U_UE4_Clean.fbx"));
	if(!IFileManager::Get().FileExists(*InputPath))
	{
		UE_LOG(LogTemp,Error,TEXT("AK74U source FBX not found: %s"),*InputPath);
		return 2;
	}

	FbxManager* Manager=FbxManager::Create();
	if(!Manager)return 3;
	FbxIOSettings* Settings=FbxIOSettings::Create(Manager,IOSROOT);
	Manager->SetIOSettings(Settings);
	FbxImporter* Importer=FbxImporter::Create(Manager,"AK74UImporter");
	const FTCHARToUTF8 InputUtf8(*InputPath);
	if(!Importer->Initialize(InputUtf8.Get(),-1,Manager->GetIOSettings()))
	{
		UE_LOG(LogTemp,Error,TEXT("FBX SDK could not open source: %s"),UTF8_TO_TCHAR(Importer->GetStatus().GetErrorString()));
		Importer->Destroy();
		Manager->Destroy();
		return 4;
	}
	FbxScene* Scene=FbxScene::Create(Manager,"AK74UScene");
	if(!Importer->Import(Scene))
	{
		UE_LOG(LogTemp,Error,TEXT("FBX SDK import failed: %s"),UTF8_TO_TCHAR(Importer->GetStatus().GetErrorString()));
		Importer->Destroy();
		Manager->Destroy();
		return 5;
	}
	Importer->Destroy();

	FbxNode* SceneRoot=Scene->GetRootNode();
	LogImportantNodes(SceneRoot);
	LogSkeletonRoots(SceneRoot,false);
	LogMeshSkin(SceneRoot);
	PruneFirstPersonSleeveShoulders(SceneRoot);
	FbxSkeleton* RootAttribute=FbxSkeleton::Create(Scene,"UE4RootAttribute");
	RootAttribute->SetSkeletonType(FbxSkeleton::eRoot);
	RootAttribute->Size.Set(1.0);
	FbxNode* UnifiedRoot=FbxNode::Create(Scene,"UE4Root");
	UnifiedRoot->SetNodeAttribute(RootAttribute);
	SceneRoot->AddChild(UnifiedRoot);

	TArray<FbxNode*> SkeletonBranches;
	for(int32 ChildIndex=0;ChildIndex<SceneRoot->GetChildCount();++ChildIndex)
	{
		FbxNode* Child=SceneRoot->GetChild(ChildIndex);
		if(Child!=UnifiedRoot&&ContainsSkeleton(Child))SkeletonBranches.Add(Child);
	}
	for(FbxNode* Branch:SkeletonBranches)
	{
		SceneRoot->RemoveChild(Branch);
		UnifiedRoot->AddChild(Branch);
		UE_LOG(LogTemp,Display,TEXT("AK74U reparented skeleton branch: %s"),UTF8_TO_TCHAR(Branch->GetName()));
	}

	FbxExporter* Exporter=FbxExporter::Create(Manager,"AK74UExporter");
	const FTCHARToUTF8 OutputUtf8(*OutputPath);
	if(!Exporter->Initialize(OutputUtf8.Get(),-1,Manager->GetIOSettings())||!Exporter->Export(Scene))
	{
		UE_LOG(LogTemp,Error,TEXT("FBX SDK export failed: %s"),UTF8_TO_TCHAR(Exporter->GetStatus().GetErrorString()));
		Exporter->Destroy();
		Manager->Destroy();
		return 6;
	}
	Exporter->Destroy();
	Manager->Destroy();
	UE_LOG(LogTemp,Display,TEXT("AK74U_CLEAN_UE4_FBX_READY: %s"),*OutputPath);
	return 0;
#endif
}
