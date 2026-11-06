#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrassInteractionDemo.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

UCLASS()
class GRASSINTERACTION_API AGrassInteractionDemo : public AActor
{
    GENERATED_BODY()

public:
    AGrassInteractionDemo();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category="Grass")
    TObjectPtr<UMaterialInterface> GrassMaterial;

    UPROPERTY(EditAnywhere, Category="Grass")
    TObjectPtr<UMaterialInterface> DebugMaterial;

    UPROPERTY(EditAnywhere, Category="Grass", meta=(ClampMin="200", ClampMax="4000"))
    float FieldSize = 1200.f;

    UPROPERTY(EditAnywhere, Category="Grass", meta=(ClampMin="8", ClampMax="100"))
    int32 GrassPerSide = 64;

    UPROPERTY(EditAnywhere, Category="Spring", meta=(ClampMin="16", ClampMax="512"))
    int32 Resolution = 128;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spring", meta=(ClampMin="1", ClampMax="100"))
    float Stiffness = 14.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spring", meta=(ClampMin="0.1", ClampMax="20"))
    float Damping = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0", ClampMax="3000"))
    float ForceStrength = 1100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="10", ClampMax="300"))
    float BrushRadius = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    bool bAutoMove = true;

    // Optional: drag a character/actor here to use its position instead of the automatic sphere.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    TObjectPtr<AActor> Interactor;

    UPROPERTY(EditAnywhere, Category="Demo")
    bool bUseDemoCamera = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Render Targets")
    TObjectPtr<UTextureRenderTarget2D> StateA;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Render Targets")
    TObjectPtr<UTextureRenderTarget2D> StateB;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Render Targets")
    TObjectPtr<UTextureRenderTarget2D> CurrentState;

    UFUNCTION(BlueprintCallable, Category="Grass Demo")
    void ResetSimulation();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Grass;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Brush;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PositionPreview;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VelocityPreview;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GrassMID;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PositionMID;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VelocityMID;
    float Accumulator = 0.f;
    float SimulationTime = 0.f;

    void BuildGrass();
    void BindCurrentState();
};
