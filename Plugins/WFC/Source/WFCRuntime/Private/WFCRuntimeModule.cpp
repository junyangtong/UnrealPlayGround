// Private/WFCRuntimeModule.cpp
#include "WFCRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FWFCRuntimeModule"

void FWFCRuntimeModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("WFCRuntime Module Started"));
}

void FWFCRuntimeModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("WFCRuntime Module Shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWFCRuntimeModule, WFCRuntime)
