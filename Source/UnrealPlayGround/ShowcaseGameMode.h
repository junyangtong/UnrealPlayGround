#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShowcaseGameMode.generated.h"

/** Demo actors provide their own cameras and input; no default pawn geometry is needed. */
UCLASS()
class UNREALPLAYGROUND_API AShowcaseGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AShowcaseGameMode() { DefaultPawnClass = nullptr; }
};
