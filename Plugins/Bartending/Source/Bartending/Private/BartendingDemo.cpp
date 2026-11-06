#include "BartendingDemo.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr int32 Segments = 96;
constexpr float Radius = 95.f, Height = 290.f, CupBase = 30.f;
struct FMesh
{
    TArray<FVector> P, N;
    TArray<FVector2D> UV, HeightUV;
    TArray<int32> T;
    TArray<FLinearColor> C;
    void Vertex(FVector Pos, FVector Normal, FVector2D Tex, FLinearColor Color = FLinearColor::White)
    { P.Add(Pos); N.Add(Normal); UV.Add(Tex); HeightUV.Add(FVector2D((Pos.Z-CupBase)/Height, Normal.Z>.8f ? 1.f : 0.f)); C.Add(Color); }
    void Quad(int32 A, int32 B, int32 C0, int32 D)
    { T.Append({A, B, C0, A, C0, D}); }
    void Apply(UProceduralMeshComponent* Mesh)
    {
        if (Mesh->GetProcMeshSection(0) && Mesh->GetProcMeshSection(0)->ProcVertexBuffer.Num() == P.Num())
            Mesh->UpdateMeshSection_LinearColor(0, P, N, UV, HeightUV, {}, {}, C, {});
        else Mesh->CreateMeshSection_LinearColor(0, P, T, N, UV, HeightUV, {}, {}, C, {}, false);
    }
};
}

ABartendingDemo::ABartendingDemo()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ImportedCup = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OriginalCup"));
    ImportedCup->SetupAttachment(RootComponent);
    ImportedCup->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ImportedCup->SetCastShadow(false);
    ImportedCup->SetTranslucentSortPriority(20);
    BackCup = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackCup"));
    BackCup->SetupAttachment(RootComponent);
    BackCup->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BackCup->SetCastShadow(false);
    BackCup->SetTranslucentSortPriority(0);
    Liquid = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Liquid"));
    Glass = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Glass"));
    for (auto* Mesh : {Liquid.Get(), Glass.Get()})
    {
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
    }
    Liquid->SetTranslucentSortPriority(10);
    Glass->SetTranslucentSortPriority(20);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> Black(TEXT("/Engine/EngineResources/Black.Black"));
    BlackTexture = Black.Object;
    for (int32 I = 0; I < 5; ++I)
    {
        auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Ice%d"), I));
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetRelativeScale3D(FVector(0.17f));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Mesh->SetTranslucentSortPriority(15);
        Ice.Add(Mesh);
        auto* Under = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("SubmergedIce%d"), I));
        Under->SetupAttachment(RootComponent);
        Under->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Under->SetCastShadow(false);
        Under->SetTranslucentSortPriority(5);
        SubmergedIce.Add(Under);
    }
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("DemoCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeLocation(FVector(0, -1650, 475));
    Camera->SetRelativeRotation((FVector(0, 0, 175) - Camera->GetRelativeLocation()).Rotation());
    Camera->FieldOfView = 35.f;
    const FLinearColor Colors[] = {{0.85f,0.025f,0.08f,0.85f},{1.f,0.25f,0.025f,0.78f},
        {0.08f,0.85f,0.42f,0.75f},{0.025f,0.38f,1.f,0.75f},{0.48f,0.06f,0.85f,0.8f}};
    for (int32 I = 0; I < 5; ++I)
    { FCocktailLayer L; L.Color = Colors[I]; L.Warp = 0.05f + I * 0.04f; Presets.Add(L); }
}

void ABartendingDemo::InitializeDrink(int32 Count)
{
    LiquidLayers.Reset();
    for (int32 I=0; I<5; ++I) for (int32 J=0; J<5; ++J) MaskWeights[I][J] = I==J ? 1.f : 0.f;
    for (int32 I = 0; I < FMath::Clamp(Count, 0, 5) && Presets.Num() > 0; ++I) LiquidLayers.Add(Presets[I % Presets.Num()]);
    Fill = LiquidLayers.Num() / 5.f;
    Time = Elapsed = WaveAmplitude = MaskMix = 0.f;
    bBusy = bStirring = false;
    StirStart.Reset();
    LiquidMID = LiquidMaterial ? UMaterialInstanceDynamic::Create(LiquidMaterial, this) : nullptr;
    Liquid->SetMaterial(0, LiquidMID);
    Glass->SetMaterial(0, GlassMaterial);
    SubmergedIceMID = SubmergedIceMaterial ? UMaterialInstanceDynamic::Create(SubmergedIceMaterial,this) : nullptr;
    IceMID = IceMaterial ? UMaterialInstanceDynamic::Create(IceMaterial,this) : nullptr;
    for (int32 I = 0; I < Ice.Num(); ++I)
    { Ice[I]->SetMaterial(0, IceMID); IceZ[I] = CupBase + FMath::Max(9.f, Fill * Height - 3.f); IceVelocity[I] = 0; }
    SourcePositions.Reset(); SourceTriangles.Reset(); SourceNormals.Reset(); SourceUVs.Reset();
    if (LiquidSourceMesh)
    {
        TArray<FProcMeshTangent> Tangents;
        UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(LiquidSourceMesh,0,0,SourcePositions,SourceTriangles,SourceNormals,SourceUVs,Tangents);
    }
    BackCup->SetStaticMesh(CupSourceMesh);
    BackCup->SetMaterial(0,BackGlassMaterial);
    BackCup->SetMaterial(1,InvisibleMaterial);
    BackCup->SetVisibility(CupSourceMesh && BackGlassMaterial);
    ImportedCup->SetStaticMesh(CupSourceMesh);
    ImportedCup->SetMaterial(0,GlassMaterial);
    ImportedCup->SetMaterial(1,CupBaseMaterial);
    ImportedCup->SetVisibility(CupSourceMesh!=nullptr);
    Glass->SetVisibility(CupSourceMesh==nullptr);
    if (IceSourceMesh) for (const auto& Mesh : Ice) { Mesh->SetStaticMesh(IceSourceMesh); Mesh->SetRelativeScale3D(FVector(.6f)); }
    for (const auto& Under : SubmergedIce)
    {
        Under->SetStaticMesh(IceSourceMesh);
        Under->SetRelativeScale3D(FVector(.6f));
        Under->SetMaterial(0,SubmergedIceMID);
    }
    BuildGlass(); UpdateMaterials(); UpdateLiquidMesh(); UpdateIce(0);
}
void ABartendingDemo::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); InitializeDrink(InitialLayers); }
void ABartendingDemo::BeginPlay()
{
    Super::BeginPlay(); InitializeDrink(InitialLayers);
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        EnableInput(PC);
        if (bUseDemoCamera) PC->SetViewTarget(this);
        InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ABartendingDemo::Pour0);
        InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ABartendingDemo::Pour1);
        InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ABartendingDemo::Pour2);
        InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ABartendingDemo::Pour3);
        InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ABartendingDemo::Pour4);
        InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ABartendingDemo::PourNext);
        InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ABartendingDemo::Stir);
        InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ABartendingDemo::ResetDrink);
        InputComponent->BindKey(EKeys::I, IE_Pressed, this, &ABartendingDemo::ToggleIce);
    }
}
bool ABartendingDemo::PourLayer(int32 PresetIndex)
{
    if (bBusy || LiquidLayers.Num() >= 5 || !Presets.IsValidIndex(PresetIndex)) return false;
    LiquidLayers.Add(Presets[PresetIndex]); StartFill = Fill; Elapsed = 0; bBusy = true; bStirring = false;
    UpdateMaterials(); return true;
}
void ABartendingDemo::PourNext() { if (Presets.Num()) PourLayer(LiquidLayers.Num() % Presets.Num()); }
void ABartendingDemo::Stir()
{
    if (bBusy || LiquidLayers.Num() < 2) return;
    for (int32 I=0; I<5; ++I) for (int32 J=0; J<5; ++J) StirWeights[I][J] = MaskWeights[I][J];
    StirStart = LiquidLayers; Elapsed = 0; bBusy = bStirring = true;
}
void ABartendingDemo::ResetDrink() { InitializeDrink(0); }
void ABartendingDemo::PreviewFull() { InitializeDrink(5); }
void ABartendingDemo::ToggleIce() { bShowIce = !bShowIce; UpdateIce(0); }

void ABartendingDemo::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Max(0.f, DeltaSeconds);
    Time += Dt;
    if (bBusy)
    {
        Elapsed += Dt;
        const float A = FMath::Clamp(Elapsed / FMath::Max(0.1f, bStirring ? StirDuration : PourDuration), 0.f, 1.f);
        const float S = A * A * (3.f - 2.f * A);
        WaveAmplitude = FMath::Sin(A * PI) * (bStirring ? 3.f : 2.f);
        if (bStirring)
        {
            FLinearColor Mean(0,0,0,0); float Bubbles = 0;
            for (const auto& L : StirStart) { Mean += L.Color; Bubbles += L.Bubbles; }
            Mean /= StirStart.Num(); Bubbles /= StirStart.Num();
            for (int32 I = 0; I < LiquidLayers.Num(); ++I)
            {
                LiquidLayers[I].Color = FMath::Lerp(StirStart[I].Color, Mean, S);
                LiquidLayers[I].Bubbles = FMath::Lerp(StirStart[I].Bubbles, Bubbles, S);
                LiquidLayers[I].BlendRange = FMath::Lerp(StirStart[I].BlendRange, 0.5f, S);
            }
            for (int32 J=0; J<5; ++J)
            {
                float MeanWeight=0;
                for (int32 I=0; I<LiquidLayers.Num(); ++I) MeanWeight += StirWeights[I][J]/LiquidLayers.Num();
                for (int32 I=0; I<LiquidLayers.Num(); ++I) MaskWeights[I][J] = FMath::Lerp(StirWeights[I][J],MeanWeight,S);
            }
        }
        else Fill = FMath::Lerp(StartFill, LiquidLayers.Num() / 5.f, S);
        if (A >= 1) { bBusy = false; WaveAmplitude = 0; }
    }
    UpdateMaterials(); UpdateLiquidMesh(); UpdateIce(FMath::Min(Dt, 0.1f));
}
float ABartendingDemo::Wave(float X, float Y) const
{
    if (bStirring)
    {
        const float R = FMath::Sqrt(X*X + Y*Y) / Radius;
        return WaveAmplitude * R * FMath::Sin(FMath::Atan2(Y, X)*2.f + Time*6.f);
    }
    return WaveAmplitude * 0.5f * (FMath::Sin(X*.0714f + Time*7.f) + FMath::Sin(Y*.0714f + Time*6.f));
}
void ABartendingDemo::UpdateMaterials()
{
    if (SubmergedIceMID) SubmergedIceMID->SetScalarParameterValue(TEXT("LiquidSurface"),GetActorTransform().TransformPosition(FVector(0,0,CupBase+Fill*Height)).Z);
    if (IceMID) IceMID->SetScalarParameterValue(TEXT("LiquidSurface"),GetActorTransform().TransformPosition(FVector(0,0,CupBase+Fill*Height)).Z);
    if (!LiquidMID) return;
    LiquidMID->SetScalarParameterValue(TEXT("LayerCount"), LiquidLayers.Num());
    LiquidMID->SetScalarParameterValue(TEXT("Fill"), Fill);
    LiquidMID->SetScalarParameterValue(TEXT("Clock"), Time);
    LiquidMID->SetScalarParameterValue(TEXT("MaskMix"), MaskMix);
    LiquidMID->SetScalarParameterValue(TEXT("Swirl"), bBusy && bStirring ? Time*0.15f : 0.f);
    for (int32 I = 0; I < 5; ++I)
    {
        const FCocktailLayer L = LiquidLayers.Num() ? LiquidLayers[FMath::Min(I, LiquidLayers.Num()-1)] : FCocktailLayer();
        LiquidMID->SetVectorParameterValue(*FString::Printf(TEXT("Weight%d"), I), FLinearColor(MaskWeights[I][0],MaskWeights[I][1],MaskWeights[I][2],MaskWeights[I][3]));
        LiquidMID->SetScalarParameterValue(*FString::Printf(TEXT("Tail%d"), I), MaskWeights[I][4]);
        LiquidMID->SetVectorParameterValue(*FString::Printf(TEXT("Color%d"), I), L.Color);
        LiquidMID->SetVectorParameterValue(*FString::Printf(TEXT("Detail%d"), I), FLinearColor(L.BlendRange, L.Bubbles, L.Warp, L.NoiseScale));
        LiquidMID->SetTextureParameterValue(*FString::Printf(TEXT("Mask%d"), I), L.Mask ? L.Mask.Get() : BlackTexture.Get());
    }
}
void ABartendingDemo::UpdateLiquidMesh()
{
    Liquid->SetVisibility(Fill > KINDA_SMALL_NUMBER);
    if (Fill <= KINDA_SMALL_NUMBER) return;
    if (SourcePositions.Num() && SourceTriangles.Num()) { UpdateOriginalLiquidMesh(); return; }
    FMesh M;
    // Wall rings: UV.y always measures absolute normalized cup height, independent of filling.
    for (int32 Row=0; Row<=16; ++Row)
    for (int32 I=0; I<=Segments; ++I)
    {
        const float U = I / float(Segments), A = U * 2.f * PI;
        const float X = Radius * FMath::Cos(A), Y = Radius * FMath::Sin(A);
        const float T = Row/16.f;
        const float Z = FMath::Max(.01f, Fill*Height + Wave(X,Y) + .65f) * T;
        M.Vertex(FVector(X,Y,CupBase+Z), FVector(X/Radius,Y/Radius,0), FVector2D(U,Z/Height));
        if (Row<16 && I<Segments) { const int32 K=Row*(Segments+1)+I; M.Quad(K,K+1,K+Segments+2,K+Segments+1); }
    }
    const int32 Start = M.P.Num();
    // Radial top, with raised meniscus and normals from the same displacement function.
    for (int32 Ring=0; Ring<=12; ++Ring)
    for (int32 I=0; I<=Segments; ++I)
    {
        const float R=Ring/12.f, U=I/float(Segments), A=U*2*PI;
        const float X=Radius*R*FMath::Cos(A), Y=Radius*R*FMath::Sin(A);
        const float Z=FMath::Max(.01f,Fill*Height+Wave(X,Y)+.65f*FMath::Pow(R,12.f));
        const float MeniscusSlope = .65f*12.f/Radius*FMath::Pow(R,11.f);
        const FVector N = FVector(-(Wave(X+.1f,Y)-Wave(X-.1f,Y))/.2f-MeniscusSlope*FMath::Cos(A),
            -(Wave(X,Y+.1f)-Wave(X,Y-.1f))/.2f-MeniscusSlope*FMath::Sin(A),1).GetSafeNormal();
        M.Vertex(FVector(X,Y,CupBase+Z), N, FVector2D(U,Z/Height), FLinearColor(.75f,.9f,1,1));
        if (Ring<12 && I<Segments) { const int32 K=Start+Ring*(Segments+1)+I; M.Quad(K,K+Segments+1,K+Segments+2,K+1); }
    }
    M.Apply(Liquid);
}
void ABartendingDemo::BuildGlass()
{
    FMesh M;
    // Continuous profile: underside, outer wall, lip, inner wall, inner floor.
    const FVector2D Profile[] = {{0,0},{31,0},{33,128},{29.5,128},{28.5,5},{0,5}};
    for (int32 Row=0; Row<6; ++Row)
    for (int32 I=0; I<=Segments; ++I)
    {
        const float U=I/float(Segments), A=U*2*PI;
        FVector N(FMath::Cos(A),FMath::Sin(A),0);
        if (Row>=3) N *= -1;
        M.Vertex(FVector(Profile[Row].X*FMath::Cos(A),Profile[Row].X*FMath::Sin(A),Profile[Row].Y),N,FVector2D(U,Row/5.f));
        if (Row<5 && I<Segments) { const int32 K=Row*(Segments+1)+I; M.Quad(K,K+1,K+Segments+2,K+Segments+1); }
    }
    M.Apply(Glass);
}
void ABartendingDemo::UpdateIce(float Dt)
{
    const int32 Steps = FMath::Max(1,FMath::CeilToInt(Dt*120.f));
    const float H = Dt/Steps;
    for (int32 I=0; I<Ice.Num(); ++I)
    {
        const float Angle = I*2.f*PI/5.f + (bBusy && bStirring ? Time*.7f : 0.f);
        const float X=43.f*FMath::Cos(Angle), Y=43.f*FMath::Sin(Angle);
        const float Surface=CupBase+Fill*Height+Wave(X,Y);
        for (int32 S=0; S<Steps; ++S)
        {
            // Submerged-volume approximation for the scaled original ice mesh.
            const float Submerged=FMath::Clamp((Surface-IceZ[I]+21.f)/42.f,0.f,1.f);
            const float Accel=980.f*(Submerged/.78f-1.f)-IceVelocity[I]*5.f;
            IceVelocity[I]+=Accel*H; IceZ[I]+=IceVelocity[I]*H;
            if (IceZ[I]<CupBase+21.f) { IceZ[I]=CupBase+21.f; IceVelocity[I]=FMath::Max(0.f,IceVelocity[I]); }
        }
        Ice[I]->SetVisibility(bShowIce);
        Ice[I]->SetRelativeLocation(FVector(X,Y,IceZ[I]));
        Ice[I]->SetRelativeRotation(FRotator(8.f*FMath::Sin(Time+I),I*37.f,6.f*FMath::Cos(Time*1.3f+I)));
        SubmergedIce[I]->SetRelativeTransform(Ice[I]->GetRelativeTransform());
        SubmergedIce[I]->SetVisibility(bShowIce && SubmergedIceMaterial!=nullptr);
    }
}


// Clip the ORIGINAL FBX triangles, preserving their side normals and UVs.
// The cap follows the actual intersection contour instead of replacing the vessel with a cylinder.
void ABartendingDemo::UpdateOriginalLiquidMesh()
{
    struct FClipVertex { FVector P,N; FVector2D UV; double D; };
    FMesh M;
    TArray<FVector> Rim;
    const float Level=Fill*Height;
    for (int32 I=0; I+2<SourceTriangles.Num(); I+=3)
    {
        TArray<FClipVertex,TInlineAllocator<4>> Polygon;
        for (int32 K=0; K<3; ++K)
        {
            const int32 Index=SourceTriangles[I+K];
            const FVector P0=SourcePositions[Index];
            Polygon.Add({P0,SourceNormals[Index],SourceUVs[Index],Level+Wave(P0.X,P0.Y)-P0.Z});
        }
        TArray<FClipVertex,TInlineAllocator<4>> Clipped;
        for (int32 K=0; K<3; ++K)
        {
            const auto& A=Polygon[K]; const auto& B=Polygon[(K+1)%3];
            if (A.D>=0) Clipped.Add(A);
            if ((A.D>=0)!=(B.D>=0))
            {
                const float T=A.D/(A.D-B.D);
                const FVector P0=FMath::Lerp(A.P,B.P,T);
                Clipped.Add({P0,FMath::Lerp(A.N,B.N,T).GetSafeNormal(),FMath::Lerp(A.UV,B.UV,T),0});
                Rim.Add(P0);
            }
        }
        if (Clipped.Num()<3) continue;
        const int32 First=M.P.Num();
        for (const auto& V:Clipped) M.Vertex(V.P+FVector(0,0,CupBase),V.N,V.UV);
        for (int32 K=1;K+1<Clipped.Num();++K) M.T.Append({First,First+K,First+K+1});
    }
    Rim.Sort([](const FVector& A,const FVector& B){return FMath::Atan2(A.Y,A.X)<FMath::Atan2(B.Y,B.X);});
    const int32 Center=M.P.Num();
    M.Vertex(FVector(0,0,CupBase+Level+Wave(0,0)),FVector::UpVector,FVector2D(.5,.5));
    for (const FVector& P0:Rim) M.Vertex(P0+FVector(0,0,CupBase),FVector::UpVector,FVector2D(P0.X*.01f,P0.Y*.01f));
    for (int32 I=0;I<Rim.Num();++I)
    {
        const int32 Next=(I+1)%Rim.Num();
        if (!Rim[I].Equals(Rim[Next],.01f)) M.T.Append({Center,Center+1+Next,Center+1+I});
    }
    // Clipping changes triangle connectivity even at the same vertex count.
    Liquid->CreateMeshSection_LinearColor(0,M.P,M.T,M.N,M.UV,M.HeightUV,{}, {},M.C,{},false);
}
