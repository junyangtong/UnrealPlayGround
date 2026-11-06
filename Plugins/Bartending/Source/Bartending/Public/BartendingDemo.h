#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BartendingDemo.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;
class UStaticMesh;

USTRUCT(BlueprintType)
struct BARTENDING_API FCocktailLayer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Color = FLinearColor(0.9f, 0.1f, 0.2f, 0.8f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01", ClampMax="0.5")) float BlendRange = 0.12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="10")) float Bubbles = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) float Warp = 0.12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1")) float NoiseScale = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UTexture2D> Mask = nullptr;
};

/** Upright, fixed-size demonstration. Actor transform moves the whole effect; no spilling solver. */
UCLASS()
class BARTENDING_API ABartendingDemo : public AActor
{
    GENERATED_BODY()
public:
    ABartendingDemo();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cocktail") TArray<FCocktailLayer> Presets;
    UPROPERTY(EditAnywhere, Category="Cocktail", meta=(ClampMin="0", ClampMax="5")) int32 InitialLayers = 5;
    UPROPERTY(EditAnywhere, Category="Cocktail", meta=(ClampMin="0.1")) float PourDuration = 1.3f;
    UPROPERTY(EditAnywhere, Category="Cocktail", meta=(ClampMin="0.1")) float StirDuration = 3.f;
    UPROPERTY(EditAnywhere, Category="Cocktail") bool bUseDemoCamera = true;
    UPROPERTY(EditAnywhere, Category="Original Assets") TObjectPtr<UStaticMesh> CupSourceMesh;
    UPROPERTY(EditAnywhere, Category="Original Assets") TObjectPtr<UStaticMesh> LiquidSourceMesh;
    UPROPERTY(EditAnywhere, Category="Original Assets") TObjectPtr<UStaticMesh> IceSourceMesh;
    UPROPERTY(EditAnywhere, Category="Materials") TObjectPtr<UMaterialInterface> CupBaseMaterial;
    UPROPERTY(EditAnywhere, Category="Materials") TObjectPtr<UMaterialInterface> LiquidMaterial;
    UPROPERTY(EditAnywhere, Category="Materials") TObjectPtr<UMaterialInterface> GlassMaterial;
    UPROPERTY(EditAnywhere, Category="Materials") TObjectPtr<UMaterialInterface> IceMaterial;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State") TArray<FCocktailLayer> LiquidLayers;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State") float Fill = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State") bool bBusy = false;
    UFUNCTION(BlueprintCallable, Category="Cocktail") bool PourLayer(int32 PresetIndex);
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Cocktail") void PourNext();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Cocktail") void Stir();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Cocktail") void ResetDrink();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Cocktail") void ToggleIce();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Cocktail") void PreviewFull();
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ImportedCup;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Liquid;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Glass;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Ice;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> LiquidMID;
    UPROPERTY(Transient) TArray<FCocktailLayer> StirStart;
    UPROPERTY() TObjectPtr<UTexture2D> BlackTexture;
    float Time = 0.f, Elapsed = 0.f, StartFill = 0.f, WaveAmplitude = 0.f, MaskMix = 0.f;
    float MaskWeights[5][5] = {}, StirWeights[5][5] = {};
    float IceZ[5] = {}, IceVelocity[5] = {};
    bool bStirring = false, bShowIce = true;
    void InitializeDrink(int32 Count);
    void UpdateMaterials();
    TArray<FVector> SourcePositions, SourceNormals;
    TArray<FVector2D> SourceUVs;
    TArray<int32> SourceTriangles;
    void BuildGlass();
    void UpdateOriginalLiquidMesh();
    void UpdateLiquidMesh();
    void UpdateIce(float Dt);
    float Wave(float X, float Y) const;
    void Pour0() { PourLayer(0); }
    void Pour1() { PourLayer(1); }
    void Pour2() { PourLayer(2); }
    void Pour3() { PourLayer(3); }
    void Pour4() { PourLayer(4); }
};
