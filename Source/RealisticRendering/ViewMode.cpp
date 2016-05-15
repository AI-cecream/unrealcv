// Fill out your copyright notice in the Description page of Project Settings.

#include "RealisticRendering.h"
#include "ViewMode.h"
#include "BufferVisualizationData.h"
#include "CommandDispatcher.h"
#include "Async.h"

#define REG_VIEW(COMMAND, DESC, DELEGATE) \
		IConsoleManager::Get().RegisterConsoleCommand(TEXT(COMMAND), TEXT(DESC), \
			FConsoleCommandWithWorldDelegate::CreateStatic(DELEGATE), ECVF_Default);

UWorld* FViewMode::World = NULL;
FString FViewMode::CurrentViewMode = "lit"; // "lit" the default mode when the game starts

FViewMode::FViewMode()
{
}

FViewMode::~FViewMode()
{
}


/* Define console commands */
void FViewMode::Depth(UWorld *World)
{
	check(World);

	UGameViewportClient* Viewport = World->GetGameViewport();
	// Viewport->CurrentBufferVisualization
	Viewport->EngineShowFlags.SetVisualizeBuffer(true);
	SetCurrentBufferVisualizationMode(TEXT("SceneDepth"));
	// TODO: Enable post process or not will show interesting result.
	// Viewport->EngineShowFlags.SetPostProcessing(false);
}

void FViewMode::SetCurrentBufferVisualizationMode(FString ViewMode)
{
	// A complete list can be found from Engine/Config/BaseEngine.ini, Engine.BufferVisualizationMaterials
	// TODO: BaseColor is weird, check BUG.
	// No matter in which thread, ICVar needs to be set in the GameThread

	// AsyncTask is from https://answers.unrealengine.com/questions/317218/execute-code-on-gamethread.html
	// lambda is from http://stackoverflow.com/questions/26903602/an-enclosing-function-local-variable-cannot-be-referenced-in-a-lambda-body-unles
	// Make this async is risky, TODO:
	static IConsoleVariable* ICVar = IConsoleManager::Get().FindConsoleVariable(FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
	AsyncTask(ENamedThreads::GameThread, [ViewMode]() { // & means capture by reference
		if (ICVar)
		{
			ICVar->Set(*ViewMode, ECVF_SetByCode);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("The BufferVisualization is not correctly configured."));
		}
	});
}

void FViewMode::Normal(UWorld *World)
{
	check(World);

	UGameViewportClient* Viewport = World->GetGameViewport();
	Viewport->EngineShowFlags.SetVisualizeBuffer(true);
	SetCurrentBufferVisualizationMode(TEXT("WorldNormal"));
}


void FViewMode::Lit(UWorld* World)
{
	check(World);

	auto Viewport = World->GetGameViewport();
	ApplyViewMode(VMI_Lit, true, Viewport->EngineShowFlags);

	Viewport->EngineShowFlags.SetMaterials(true);
}

void FViewMode::Unlit(UWorld* World)
{
	check(World);

	auto Viewport = World->GetGameViewport();
	ApplyViewMode(VMI_Unlit, true, Viewport->EngineShowFlags);
	Viewport->EngineShowFlags.SetMaterials(false);
}

void FViewMode::Object(UWorld* World)
{
	check(World);

	auto Viewport = World->GetGameViewport();
	/* This is from ShowFlags.cpp and not working, give it up.
	Viewport->EngineShowFlags.SetLighting(false);
	Viewport->EngineShowFlags.LightFunctions = 0;
	Viewport->EngineShowFlags.SetPostProcessing(false);
	Viewport->EngineShowFlags.AtmosphericFog = 0;
	Viewport->EngineShowFlags.DynamicShadows = 0;
	*/
	ApplyViewMode(VMI_Lit, true, Viewport->EngineShowFlags);
	// Need to toggle this view mode
	// Viewport->EngineShowFlags.SetMaterials(true);

	Viewport->EngineShowFlags.SetMaterials(false);
	Viewport->EngineShowFlags.SetLighting(false);
	Viewport->EngineShowFlags.SetBSPTriangles(true);
	Viewport->EngineShowFlags.SetVertexColors(true);
	Viewport->EngineShowFlags.SetPostProcessing(false);
	Viewport->EngineShowFlags.SetHMDDistortion(false);

	GVertexColorViewMode = EVertexColorViewMode::Color;
}


FExecStatus FViewMode::SetMode(const TArray<FString>& Args) // Check input arguments
{
	UE_LOG(LogTemp, Warning, TEXT("Run SetMode %s"), *Args[0]);

	// Check args
	if (Args.Num() == 1)
	{
		FString ViewMode = Args[0].ToLower();
		bool bSuccess = true;
		if (ViewMode == "depth")
		{
			FViewMode::Depth(FViewMode::World);
		}
		else if (ViewMode == "normal")
		{
			FViewMode::Normal(FViewMode::World);
		}
		else if (ViewMode == "object")
		{
			FViewMode::Object(FViewMode::World);
		}
		else if (ViewMode == "lit")
		{
			FViewMode::Lit(FViewMode::World);
		}
		else if (ViewMode == "unlit")
		{
			FViewMode::Unlit(FViewMode::World);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unrecognized ViewMode %s"), *ViewMode);
			bSuccess = false;
			return FExecStatus::Error(FString::Printf(TEXT("Can not set ViewMode to %s"), *ViewMode));
		}
		if (bSuccess)
		{
			FViewMode::CurrentViewMode = ViewMode;
			return FExecStatus::OK;
		}
	}
	// Send successful message back
	// The network manager should be blocked and wait for response. So the client needs to be universally accessible?
	return FExecStatus::Error(TEXT("Arguments to SetMode are incorrect"));
}


FExecStatus FViewMode::GetMode(const TArray<FString>& Args) // Check input arguments
{
	UE_LOG(LogTemp, Warning, TEXT("Run GetMode, The mode is %s"), *FViewMode::CurrentViewMode);
	// How to send message back. Need the conversation id.
	// Send message back
	// Complex information will be saved to file
	return FExecStatus(*FViewMode::CurrentViewMode);
}
