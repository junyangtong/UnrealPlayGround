#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

class FGrassInteractionModule : public IModuleInterface
{
    virtual void StartupModule() override
    {
        AddShaderSourceDirectoryMapping(TEXT("/GrassInteraction"),
            FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("GrassInteraction"))->GetBaseDir(), TEXT("Shaders")));
    }
};

IMPLEMENT_MODULE(FGrassInteractionModule, GrassInteraction)
