#pragma once

#include "CoreMinimal.h"

class UTextureRenderTarget2D;

struct FGrassSpringStep
{
    float DeltaTime;
    float Stiffness;
    float Damping;
    float ForceStrength;
    float FieldSize;
    FVector2f BrushPosition;
    float BrushRadius;
};

// Enqueues a GPU pass. Never reads back pixels during normal gameplay.
void DispatchGrassSpring(UTextureRenderTarget2D* ReadRT, UTextureRenderTarget2D* WriteRT, FGrassSpringStep Step);
