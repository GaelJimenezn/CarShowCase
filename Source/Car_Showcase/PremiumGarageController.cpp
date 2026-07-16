#include "PremiumGarageController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Camera/CameraComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/SlateBrush.h"

namespace PremiumGarageTags
{
	static const FName Murcielago(TEXT("PremiumVehicle.Murcielago"));
	static const FName Urus(TEXT("PremiumVehicle.Urus"));
	static const FName Fairlady300ZX(TEXT("PremiumVehicle.Fairlady300ZX"));
	static const FName AE86(TEXT("PremiumVehicle.AE86"));
	static const FName PorscheSinger(TEXT("PremiumVehicle.PorscheSinger"));

	static const FName CameraWheels(TEXT("PremiumCamera.Wheels"));
	static const FName CameraSpoiler(TEXT("PremiumCamera.Spoiler"));
	static const FName CameraFront(TEXT("PremiumCamera.Front"));
	static const FName CameraMotorFront(TEXT("PremiumCamera.MotorFront"));
	static const FName CameraMotorRear(TEXT("PremiumCamera.MotorRear"));
	static const FName CameraOrbit(TEXT("PremiumCamera.Orbit"));

	static const FName VehicleTags[] = {
		Murcielago,
		Urus,
		Fairlady300ZX,
		AE86,
		PorscheSinger
	};
}

APremiumGarageController::APremiumGarageController()
{
	PrimaryActorTick.bCanEverTick = true;
	ActiveVehicleTag = PremiumGarageTags::Murcielago;
}

void APremiumGarageController::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	TSubclassOf<UPremiumGarageWidget> WidgetClass = LoadClass<UPremiumGarageWidget>(nullptr, TEXT("/Game/ProductConfig/UMG/UI_Catalogo.UI_Catalogo_C"));
	if (!WidgetClass)
	{
		WidgetClass = UPremiumGarageWidget::StaticClass();
	}
	RuntimeWidget = CreateWidget<UPremiumGarageWidget>(PC, WidgetClass);
	if (RuntimeWidget)
	{
		RuntimeWidget->SetController(this);
		RuntimeWidget->AddToViewport(100);
	}

	PC->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);

	ActiveVehicleTag = PremiumGarageTags::Murcielago;
	CacheOriginalPaintMaterials();
	for (const FName Tag : PremiumGarageTags::VehicleTags)
	{
		SetVehicleVisibility(Tag, Tag == ActiveVehicleTag);
	}
	ShowDetail(EPremiumGarageDetail::Front);
}

void APremiumGarageController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ActiveDetail == EPremiumGarageDetail::Orbit)
	{
		UpdateOrbitCamera(DeltaSeconds);
	}
	UpdateVehicleTransition(DeltaSeconds);
}

void APremiumGarageController::ShowDetail(EPremiumGarageDetail Detail)
{
	ActiveDetail = Detail;
	if (Detail == EPremiumGarageDetail::Orbit)
	{
		UpdateOrbitCamera(0.0f);
		SetCameraByTag(GetCameraTag(Detail));
		if (RuntimeWidget)
		{
			RuntimeWidget->UpdateDetailText(
				TEXT("Camara Orbital Libre"),
				TEXT("Vista 360 estable"),
				TEXT("Modo de rotacion automatica alrededor del vehiculo seleccionado. La ficha queda fija para que el texto no cambie mientras revisas el auto.")
			);
		}
		return;
	}
	SetCameraByTag(GetCameraTag(Detail));

	const FName VehicleTag = GetCurrentVehicleTag();
	const FString VehicleName = GetCurrentVehicleName(VehicleTag);
	const FString DetailName =
		Detail == EPremiumGarageDetail::Wheels ? TEXT("Detalle de Rines") :
		Detail == EPremiumGarageDetail::Spoiler ? TEXT("Aerodinamica Trasera") :
		Detail == EPremiumGarageDetail::Front ? TEXT("Frente Agresivo") :
		Detail == EPremiumGarageDetail::Motor ? TEXT("Vista de Motor") :
		TEXT("Camara Orbital");

	if (RuntimeWidget)
	{
		RuntimeWidget->UpdateDetailText(VehicleName, DetailName, GetDetailText(VehicleTag, Detail));
	}
}

void APremiumGarageController::SelectVehicleByName(const FString& VehicleName)
{
	const FName NewVehicleTag = GetVehicleTagFromName(VehicleName);
	if (NewVehicleTag.IsNone())
	{
		return;
	}

	for (const FName Tag : PremiumGarageTags::VehicleTags)
	{
		SetVehicleVisibility(Tag, Tag == NewVehicleTag);
	}

	ActiveVehicleTag = NewVehicleTag;
	if (ActiveVehicleTag == PremiumGarageTags::Urus)
	{
		ApplyBodyColor(FLinearColor(0.95f, 0.82f, 0.06f, 1.0f));
	}
	OrbitAngleDegrees = 0.0f;
	ShowDetail(ActiveDetail);
}

void APremiumGarageController::ApplyBodyColor(FLinearColor Color)
{
	UMaterialInterface* PremiumPaintParent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ProductAssets/ART/Materials/M_GuitarPaint.M_GuitarPaint"));
	const FName VehicleTag = GetCurrentVehicleTag();
	TArray<AActor*> VehicleActors;
	UGameplayStatics::GetAllActorsWithTag(this, VehicleTag, VehicleActors);

	bool bAppliedAnyPaint = false;
	for (AActor* VehicleActor : VehicleActors)
	{
		if (!VehicleActor)
		{
			continue;
		}

		TArray<UStaticMeshComponent*> MeshComponents;
		VehicleActor->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (!MeshComponent)
			{
				continue;
			}

			for (int32 MaterialSlot = 0; MaterialSlot < MeshComponent->GetNumMaterials(); ++MaterialSlot)
			{
				const FString MaterialKey = GetPaintMaterialKey(VehicleTag, MeshComponent, MaterialSlot);
				if (!PaintMaterialKeys.Contains(MaterialKey))
				{
					continue;
				}

				UMaterialInterface* Material = OriginalPaintMaterials.FindRef(MaterialKey);
				if (!Material)
				{
					Material = MeshComponent->GetMaterial(MaterialSlot);
				}

				UMaterialInstanceDynamic* DynamicPaint = MeshComponent->CreateDynamicMaterialInstance(MaterialSlot, PremiumPaintParent ? PremiumPaintParent : Material);
				if (!DynamicPaint)
				{
					continue;
				}

				DynamicPaint->SetVectorParameterValue(TEXT("PrimaryColor"), Color);
				DynamicPaint->SetVectorParameterValue(TEXT("BaseColor"), Color);
				DynamicPaint->SetVectorParameterValue(TEXT("Base Color"), Color);
				DynamicPaint->SetVectorParameterValue(TEXT("Color"), Color);
				DynamicPaint->SetVectorParameterValue(TEXT("PaintColor"), Color);
				DynamicPaint->SetVectorParameterValue(TEXT("BodyColor"), Color);
				bAppliedAnyPaint = true;
			}
		}
	}

	(void)bAppliedAnyPaint;
}

void APremiumGarageController::ResetBodyColor()
{
	const FName VehicleTag = GetCurrentVehicleTag();
	if (VehicleTag == PremiumGarageTags::Urus)
	{
		ApplyBodyColor(FLinearColor(0.95f, 0.82f, 0.06f, 1.0f));
		return;
	}

	TArray<AActor*> VehicleActors;
	UGameplayStatics::GetAllActorsWithTag(this, VehicleTag, VehicleActors);

	for (AActor* VehicleActor : VehicleActors)
	{
		if (!VehicleActor)
		{
			continue;
		}

		TArray<UStaticMeshComponent*> MeshComponents;
		VehicleActor->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (!MeshComponent)
			{
				continue;
			}

			for (int32 MaterialSlot = 0; MaterialSlot < MeshComponent->GetNumMaterials(); ++MaterialSlot)
			{
				const FString MaterialKey = GetPaintMaterialKey(VehicleTag, MeshComponent, MaterialSlot);
				if (UMaterialInterface* OriginalMaterial = OriginalPaintMaterials.FindRef(MaterialKey))
				{
					MeshComponent->SetMaterial(MaterialSlot, OriginalMaterial);
				}
			}
		}
	}
}

void APremiumGarageController::CycleGarageWallColor()
{
	const FLinearColor WallColors[] = {
		FLinearColor(0.16f, 0.17f, 0.19f, 1.0f),
		FLinearColor(0.06f, 0.11f, 0.22f, 1.0f),
		FLinearColor(0.22f, 0.06f, 0.08f, 1.0f)
	};
	GarageColorIndex = (GarageColorIndex + 1) % UE_ARRAY_COUNT(WallColors);
	ApplyGarageWallColor(WallColors[GarageColorIndex]);
}

void APremiumGarageController::SetGarageWallColor(FLinearColor Color)
{
	ApplyGarageWallColor(Color);
}

FName APremiumGarageController::GetCurrentVehicleTag() const
{
	if (!ActiveVehicleTag.IsNone())
	{
		return ActiveVehicleTag;
	}

	for (const FName Tag : PremiumGarageTags::VehicleTags)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsWithTag(this, Tag, Actors);
		for (AActor* Actor : Actors)
		{
			if (Actor && !Actor->IsHidden())
			{
				return Tag;
			}
		}
	}
	return PremiumGarageTags::AE86;
}

FString APremiumGarageController::GetCurrentVehicleName(FName VehicleTag) const
{
	if (VehicleTag == PremiumGarageTags::Murcielago) return TEXT("Lamborghini Murcielago 2001");
	if (VehicleTag == PremiumGarageTags::Urus) return TEXT("Lamborghini Urus 2018");
	if (VehicleTag == PremiumGarageTags::Fairlady300ZX) return TEXT("Nissan Fairlady 300ZX Z32 1989");
	if (VehicleTag == PremiumGarageTags::AE86) return TEXT("Toyota AE86 Sprinter Trueno Zenki");
	return TEXT("High-Poly Porsche Singer");
}

FString APremiumGarageController::GetDetailText(FName VehicleTag, EPremiumGarageDetail Detail) const
{
	if (Detail == EPremiumGarageDetail::Orbit)
	{
		return TEXT("Vista libre para comparar proporciones, acabado, escala y presencia general del modelo dentro del showroom.");
	}

	if (Detail == EPremiumGarageDetail::Motor)
	{
		if (VehicleTag == PremiumGarageTags::Murcielago) return TEXT("V12 central y tomas laterales amplias: el modelo funciona bien para ensenar proporciones de superdeportivo bajo y ancho.");
		if (VehicleTag == PremiumGarageTags::Urus) return TEXT("SUV de alto rendimiento con volumen grande, cofre largo y detalles frontales que contrastan con los coupes del catalogo.");
		if (VehicleTag == PremiumGarageTags::Fairlady300ZX) return TEXT("El Z32 representa los deportivos japoneses noventeros: frente bajo, cabina compacta y una silueta limpia.");
		if (VehicleTag == PremiumGarageTags::AE86) return TEXT("El Trueno mantiene una carroceria ligera y sencilla, buena para comparar detalle real contra modelos mas agresivos.");
		return TEXT("El Porsche Singer destaca por curvas clasicas reinterpretadas y superficies suaves que se benefician de un mesh de buena calidad.");
	}

	if (VehicleTag == PremiumGarageTags::Murcielago)
	{
		if (Detail == EPremiumGarageDetail::Wheels) return TEXT("Rines grandes y pasos de rueda bajos refuerzan la postura ancha del Murcielago.");
		if (Detail == EPremiumGarageDetail::Spoiler) return TEXT("La zaga baja y ancha ayuda a leer el volumen de superdeportivo desde camaras cercanas.");
		return TEXT("Frente afilado y perfil bajo, ideal para abrir el catalogo con un auto exotico reconocible.");
	}
	if (VehicleTag == PremiumGarageTags::Urus)
	{
		if (Detail == EPremiumGarageDetail::Wheels) return TEXT("Rines grandes y carroceria alta: el Urus rompe la escala del showroom frente a los deportivos bajos.");
		if (Detail == EPremiumGarageDetail::Spoiler) return TEXT("La parte trasera concentra volumen de SUV premium y una postura mas pesada.");
		return TEXT("Frente agresivo de Lamborghini en formato SUV, util para variar la lectura del catalogo.");
	}
	if (VehicleTag == PremiumGarageTags::Fairlady300ZX)
	{
		if (Detail == EPremiumGarageDetail::Wheels) return TEXT("Ruedas y lateral limpio de los noventa, con proporciones largas y bajas.");
		if (Detail == EPremiumGarageDetail::Spoiler) return TEXT("Zaga compacta y lineas horizontales que diferencian al 300ZX dentro del grupo japones.");
		return TEXT("Frente bajo con faros caracteristicos del Fairlady Z32, buen contraste contra los modelos modernos.");
	}
	if (VehicleTag == PremiumGarageTags::AE86)
	{
		if (Detail == EPremiumGarageDetail::Wheels) return TEXT("Rines Watanabe, postura limpia y proporciones ligeras. Es el modelo que mejor esta leyendo el detalle del import.");
		if (Detail == EPremiumGarageDetail::Spoiler) return TEXT("Zaga compacta y silueta de drift clasico. Buen contraste contra el showroom oscuro y el piso brillante.");
		return TEXT("Frente de Trueno con identidad inmediata y geometria suficiente para verse claro en primer plano.");
	}
	if (VehicleTag == PremiumGarageTags::PorscheSinger)
	{
		if (Detail == EPremiumGarageDetail::Wheels) return TEXT("Rines, pasos de rueda y detalles finos justifican usar el asset high-poly en camaras cercanas.");
		if (Detail == EPremiumGarageDetail::Spoiler) return TEXT("La cola tipo Singer se beneficia del modelado denso y de las luces largas del techo.");
		return TEXT("Frente clasico con acabado premium, buena pieza para comparar curvas suaves contra autos mas angulares.");
	}

	if (Detail == EPremiumGarageDetail::Wheels) return TEXT("Detalle lateral para revisar rines, llantas y escala del modelo.");
	if (Detail == EPremiumGarageDetail::Spoiler) return TEXT("Vista trasera para comparar volumen y acabado de la zaga.");
	return TEXT("Vista frontal para revisar identidad, luces y postura del auto.");
}

FName APremiumGarageController::GetCameraTag(EPremiumGarageDetail Detail) const
{
	if (Detail == EPremiumGarageDetail::Wheels) return PremiumGarageTags::CameraWheels;
	if (Detail == EPremiumGarageDetail::Spoiler) return PremiumGarageTags::CameraSpoiler;
	if (Detail == EPremiumGarageDetail::Front) return PremiumGarageTags::CameraFront;
	if (Detail == EPremiumGarageDetail::Motor)
	{
		const FName VehicleTag = GetCurrentVehicleTag();
		return (VehicleTag == PremiumGarageTags::Murcielago || VehicleTag == PremiumGarageTags::PorscheSinger)
			? PremiumGarageTags::CameraMotorRear
			: PremiumGarageTags::CameraMotorFront;
	}
	return PremiumGarageTags::CameraOrbit;
}

FName APremiumGarageController::GetVehicleTagFromName(const FString& VehicleName) const
{
	if (VehicleName.Contains(TEXT("Murcielago"))) return PremiumGarageTags::Murcielago;
	if (VehicleName.Contains(TEXT("Urus"))) return PremiumGarageTags::Urus;
	if (VehicleName.Contains(TEXT("Fairlady")) || VehicleName.Contains(TEXT("300ZX"))) return PremiumGarageTags::Fairlady300ZX;
	if (VehicleName.Contains(TEXT("AE86")) || VehicleName.Contains(TEXT("Trueno"))) return PremiumGarageTags::AE86;
	if (VehicleName.Contains(TEXT("Porsche")) || VehicleName.Contains(TEXT("Singer"))) return PremiumGarageTags::PorscheSinger;
	return NAME_None;
}

FString APremiumGarageController::GetVehicleNameFromTag(FName VehicleTag) const
{
	return GetCurrentVehicleName(VehicleTag);
}

bool APremiumGarageController::IsPaintMaterialName(FName VehicleTag, const UStaticMeshComponent* MeshComponent, const FString& MaterialName) const
{
	const UStaticMesh* StaticMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
	const FString MeshName = StaticMesh ? StaticMesh->GetName() : FString();
	const FString CombinedName = MeshName + TEXT(" ") + MaterialName;

	if (VehicleTag == PremiumGarageTags::Murcielago)
	{
		return MaterialName.Contains(TEXT("Murcielago_2001"), ESearchCase::IgnoreCase);
	}
	if (VehicleTag == PremiumGarageTags::Urus)
	{
		return MeshName.Contains(TEXT("yellow_body"), ESearchCase::IgnoreCase);
	}
	if (VehicleTag == PremiumGarageTags::Fairlady300ZX)
	{
		return MaterialName.Equals(TEXT("Material_001"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_002"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_003"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_004"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_005"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_008"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_010"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_011"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_013"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_015"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_016"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_017"), ESearchCase::IgnoreCase)
			|| MaterialName.Equals(TEXT("Material_018"), ESearchCase::IgnoreCase);
	}
	if (VehicleTag == PremiumGarageTags::AE86)
	{
		return MeshName.Contains(TEXT("CarBody_Primary"), ESearchCase::IgnoreCase)
			|| MeshName.Contains(TEXT("CarBody_Secondary"), ESearchCase::IgnoreCase);
	}

	const FString BlockedTokens[] = {
		TEXT("brake"),
		TEXT("light"),
		TEXT("tail"),
		TEXT("head"),
		TEXT("lamp"),
		TEXT("glass"),
		TEXT("window"),
		TEXT("wheel"),
		TEXT("rim"),
		TEXT("tire"),
		TEXT("tyre"),
		TEXT("disc"),
		TEXT("caliper"),
		TEXT("interior"),
		TEXT("seat"),
		TEXT("exhaust"),
		TEXT("engine"),
		TEXT("plate"),
		TEXT("logo"),
		TEXT("emblem"),
		TEXT("chrome"),
		TEXT("carbon"),
		TEXT("rubber"),
		TEXT("leather"),
		TEXT("grill")
	};
	for (const FString& Token : BlockedTokens)
	{
		if (CombinedName.Contains(Token, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}

	return MaterialName.Contains(TEXT("Paint"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("CarPaint"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("Car_Paint"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("CAR_PAINT"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("Coloured"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("Colour"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("Body"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("Primary"), ESearchCase::IgnoreCase)
		|| MaterialName.Contains(TEXT("Exterior"), ESearchCase::IgnoreCase);
}

FString APremiumGarageController::GetPaintMaterialKey(FName VehicleTag, const UStaticMeshComponent* MeshComponent, int32 MaterialSlot) const
{
	return FString::Printf(TEXT("%s|%s|%d"), *VehicleTag.ToString(), *GetPathNameSafe(MeshComponent), MaterialSlot);
}

int32 APremiumGarageController::GetMainPaintMaterialSlot(FName VehicleTag) const
{
	AActor* VehicleActor = FindFirstActorWithTag(VehicleTag);
	UStaticMeshComponent* MeshComponent = VehicleActor ? VehicleActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!MeshComponent)
	{
		return INDEX_NONE;
	}

	if (VehicleTag == PremiumGarageTags::Murcielago) return MeshComponent->GetNumMaterials() > 1 ? 1 : INDEX_NONE;
	if (VehicleTag == PremiumGarageTags::Urus) return MeshComponent->GetNumMaterials() > 0 ? 0 : INDEX_NONE;
	if (VehicleTag == PremiumGarageTags::Fairlady300ZX) return MeshComponent->GetNumMaterials() > 0 ? 0 : INDEX_NONE;
	if (VehicleTag == PremiumGarageTags::AE86) return MeshComponent->GetNumMaterials() > 10 ? 10 : INDEX_NONE;
	if (VehicleTag == PremiumGarageTags::PorscheSinger) return MeshComponent->GetNumMaterials() > 9 ? 9 : INDEX_NONE;

	for (int32 Index = 0; Index < MeshComponent->GetNumMaterials(); ++Index)
	{
		UMaterialInterface* Material = MeshComponent->GetMaterial(Index);
		if (Material && IsPaintMaterialName(VehicleTag, MeshComponent, Material->GetName()))
		{
			return Index;
		}
	}

	return MeshComponent->GetNumMaterials() > 0 ? 0 : INDEX_NONE;
}

AActor* APremiumGarageController::FindFirstActorWithTag(FName Tag) const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(this, Tag, Actors);
	return Actors.Num() > 0 ? Actors[0] : nullptr;
}

AActor* APremiumGarageController::GetActiveVehicleActor() const
{
	return FindFirstActorWithTag(GetCurrentVehicleTag());
}

void APremiumGarageController::SetCameraByTag(FName CameraTag)
{
	AActor* CameraActor = FindFirstActorWithTag(CameraTag);
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && CameraActor)
	{
		PC->SetViewTargetWithBlend(CameraActor, 0.45f);
	}
}

void APremiumGarageController::SetVehicleVisibility(FName VehicleTag, bool bVisible)
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(this, VehicleTag, Actors);
	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Actor->SetActorHiddenInGame(!bVisible);
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTickEnabled(false);
		TArray<UStaticMeshComponent*> MeshComponents;
		Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent)
			{
				MeshComponent->SetVisibility(bVisible, true);
				MeshComponent->SetHiddenInGame(!bVisible, true);
			}
		}
	}
}

void APremiumGarageController::ApplyGarageWallColor(const FLinearColor& Color)
{
	TArray<AActor*> WallActors;
	UGameplayStatics::GetAllActorsWithTag(this, FName(TEXT("PremiumGarage.ColorWall")), WallActors);
	for (AActor* WallActor : WallActors)
	{
		UStaticMeshComponent* MeshComponent = WallActor ? WallActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
		if (!MeshComponent)
		{
			continue;
		}

		UMaterialInterface* Material = MeshComponent->GetMaterial(0);
		UMaterialInstanceDynamic* DynamicWall = MeshComponent->CreateDynamicMaterialInstance(0, Material);
		if (!DynamicWall)
		{
			continue;
		}

		DynamicWall->SetVectorParameterValue(TEXT("PrimaryColor"), Color);
		DynamicWall->SetVectorParameterValue(TEXT("SecondaryColor"), Color * 0.55f);
		DynamicWall->SetVectorParameterValue(TEXT("BaseColor"), Color);
		DynamicWall->SetVectorParameterValue(TEXT("Base Color"), Color);
		DynamicWall->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void APremiumGarageController::CacheOriginalPaintMaterials()
{
	OriginalPaintMaterials.Empty();
	PaintMaterialKeys.Empty();
	for (const FName Tag : PremiumGarageTags::VehicleTags)
	{
		TArray<AActor*> VehicleActors;
		UGameplayStatics::GetAllActorsWithTag(this, Tag, VehicleActors);
		for (AActor* VehicleActor : VehicleActors)
		{
			if (!VehicleActor)
			{
				continue;
			}

			TArray<UStaticMeshComponent*> MeshComponents;
			VehicleActor->GetComponents<UStaticMeshComponent>(MeshComponents);
			for (UStaticMeshComponent* MeshComponent : MeshComponents)
			{
				if (!MeshComponent)
				{
					continue;
				}

				for (int32 MaterialSlot = 0; MaterialSlot < MeshComponent->GetNumMaterials(); ++MaterialSlot)
				{
					UMaterialInterface* Material = MeshComponent->GetMaterial(MaterialSlot);
					if (Material && IsPaintMaterialName(Tag, MeshComponent, Material->GetName()))
					{
						const FString MaterialKey = GetPaintMaterialKey(Tag, MeshComponent, MaterialSlot);
						OriginalPaintMaterials.Add(MaterialKey, Material);
						PaintMaterialKeys.Add(MaterialKey);
					}
				}
			}
		}
	}
}

void APremiumGarageController::BeginVehicleTransition(AActor* VehicleActor)
{
	TransitionVehicleActor = nullptr;
	TransitionElapsed = 0.0f;
}

void APremiumGarageController::UpdateVehicleTransition(float DeltaSeconds)
{
	TransitionVehicleActor = nullptr;
}

void APremiumGarageController::UpdateOrbitCamera(float DeltaSeconds)
{
	AActor* CameraActor = FindFirstActorWithTag(PremiumGarageTags::CameraOrbit);
	if (!CameraActor)
	{
		return;
	}

	OrbitAngleDegrees = FMath::Fmod(OrbitAngleDegrees + DeltaSeconds * 22.0f, 360.0f);
	const float Radius = 500.0f;
	const float Radians = FMath::DegreesToRadians(OrbitAngleDegrees);
	const FVector Target(0.0f, 0.0f, 78.0f);
	const FVector NewLocation(
		FMath::Cos(Radians) * Radius,
		FMath::Sin(Radians) * Radius,
		170.0f
	);

	CameraActor->SetActorLocation(NewLocation);
	CameraActor->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(NewLocation, Target));
	if (UCameraComponent* CameraComponent = CameraActor->FindComponentByClass<UCameraComponent>())
	{
		CameraComponent->SetFieldOfView(68.0f);
	}
}

void UPremiumGarageWidget::SetController(APremiumGarageController* InController)
{
	Controller = InController;
}

TSharedRef<SWidget> UPremiumGarageWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		return Super::RebuildWidget();
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PremiumRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* LeftPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftControlPanel"));
	LeftPanel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.022f, 0.72f));
	LeftPanel->SetPadding(FMargin(16.f, 14.f, 16.f, 14.f));
	UCanvasPanelSlot* LeftSlot = RootCanvas->AddChildToCanvas(LeftPanel);
	LeftSlot->SetAnchors(FAnchors(0.f, 0.06f, 0.f, 0.06f));
	LeftSlot->SetOffsets(FMargin(24.f, 0.f, 430.f, 610.f));
	LeftSlot->SetAlignment(FVector2D(0.f, 0.f));
	DetailButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailButtonVerticalBox"));
	LeftPanel->AddChild(DetailButtonBox);

	UTextBlock* VehicleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VehicleSelectorLabel"));
	VehicleLabel->SetText(FText::FromString(TEXT("AUTOS")));
	VehicleLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.0f, 1.0f)));
	FSlateFontInfo VehicleLabelFont = VehicleLabel->GetFont();
	VehicleLabelFont.Size = 17;
	VehicleLabel->SetFont(VehicleLabelFont);
	UVerticalBoxSlot* VehicleLabelSlot = DetailButtonBox->AddChildToVerticalBox(VehicleLabel);
	VehicleLabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	VehicleComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("VehicleComboBox"));
	VehicleComboBox->AddOption(TEXT("Lamborghini Murcielago 2001"));
	VehicleComboBox->AddOption(TEXT("Lamborghini Urus 2018"));
	VehicleComboBox->AddOption(TEXT("Nissan Fairlady 300ZX Z32 1989"));
	VehicleComboBox->AddOption(TEXT("Toyota AE86 Sprinter Trueno Zenki"));
	VehicleComboBox->AddOption(TEXT("High-Poly Porsche Singer"));
	VehicleComboBox->SetSelectedOption(TEXT("Lamborghini Murcielago 2001"));
	VehicleComboBox->OnSelectionChanged.AddDynamic(this, &UPremiumGarageWidget::OnVehicleSelectionChanged);
	VehicleComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UPremiumGarageWidget::GenerateVehicleComboItem);
	VehicleComboBox->SetMaxListHeight(220.f);
	FComboBoxStyle ComboStyle = VehicleComboBox->GetWidgetStyle();
	FButtonStyle ComboButtonStyle = ComboStyle.ComboButtonStyle.ButtonStyle;
	ComboButtonStyle.SetNormal(FSlateColorBrush(FLinearColor(0.035f, 0.042f, 0.052f, 0.94f)));
	ComboButtonStyle.SetHovered(FSlateColorBrush(FLinearColor(0.10f, 0.12f, 0.15f, 1.0f)));
	ComboButtonStyle.SetPressed(FSlateColorBrush(FLinearColor(0.14f, 0.16f, 0.20f, 1.0f)));
	ComboStyle.ComboButtonStyle.SetButtonStyle(ComboButtonStyle);
	ComboStyle.SetContentPadding(FMargin(10.f, 5.f));
	ComboStyle.SetMenuRowPadding(FMargin(8.f, 6.f));
	VehicleComboBox->SetWidgetStyle(ComboStyle);
	UVerticalBoxSlot* VehicleComboSlot = DetailButtonBox->AddChildToVerticalBox(VehicleComboBox);
	VehicleComboSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	UTextBlock* ColorLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PaintColorLabel"));
	ColorLabel->SetText(FText::FromString(TEXT("COLOR PRINCIPAL")));
	ColorLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.0f, 1.0f)));
	FSlateFontInfo ColorLabelFont = ColorLabel->GetFont();
	ColorLabelFont.Size = 14;
	ColorLabel->SetFont(ColorLabelFont);
	UVerticalBoxSlot* ColorLabelSlot = DetailButtonBox->AddChildToVerticalBox(ColorLabel);
	ColorLabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	UHorizontalBox* ColorRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PaintColorRow"));
	UButton* OriginalButton = BuildColorButton(FLinearColor(0.16f, 0.17f, 0.18f, 1.f), FText::FromString(TEXT("Original")));
	OriginalButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnOriginalPaintClicked);
	UHorizontalBoxSlot* OriginalSlot = ColorRow->AddChildToHorizontalBox(OriginalButton);
	OriginalSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));

	UButton* WhiteButton = BuildColorButton(FLinearColor(0.92f, 0.90f, 0.84f, 1.f), FText::FromString(TEXT("Blanco")));
	WhiteButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnWhitePaintClicked);
	UHorizontalBoxSlot* WhiteSlot = ColorRow->AddChildToHorizontalBox(WhiteButton);
	WhiteSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));

	UButton* BlackButton = BuildColorButton(FLinearColor(0.006f, 0.006f, 0.007f, 1.f), FText::FromString(TEXT("Negro")));
	BlackButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnBlackPaintClicked);
	UHorizontalBoxSlot* BlackSlot = ColorRow->AddChildToHorizontalBox(BlackButton);
	BlackSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));

	UVerticalBoxSlot* ColorRowSlot = DetailButtonBox->AddChildToVerticalBox(ColorRow);
	ColorRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	UHorizontalBox* ColorRow2 = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PaintColorRowCommon"));
	UButton* SilverButton = BuildColorButton(FLinearColor(0.62f, 0.64f, 0.66f, 1.f), FText::FromString(TEXT("Plata")));
	SilverButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnSilverPaintClicked);
	UHorizontalBoxSlot* SilverSlot = ColorRow2->AddChildToHorizontalBox(SilverButton);
	SilverSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));

	UButton* RedButton = BuildColorButton(FLinearColor(0.72f, 0.02f, 0.015f, 1.f), FText::FromString(TEXT("Rojo")));
	RedButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnRedPaintClicked);
	UHorizontalBoxSlot* RedSlot = ColorRow2->AddChildToHorizontalBox(RedButton);
	RedSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));

	UButton* BlueButton = BuildColorButton(FLinearColor(0.02f, 0.18f, 0.62f, 1.f), FText::FromString(TEXT("Azul")));
	BlueButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnBluePaintClicked);
	UHorizontalBoxSlot* BlueSlot = ColorRow2->AddChildToHorizontalBox(BlueButton);
	BlueSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));

	UVerticalBoxSlot* ColorRow2Slot = DetailButtonBox->AddChildToVerticalBox(ColorRow2);
	ColorRow2Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailHeaderText"));
	Header->SetText(FText::FromString(TEXT("CAMARAS")));
	Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.0f, 1.0f)));
	Header->SetJustification(ETextJustify::Left);
	FSlateFontInfo HeaderFont = Header->GetFont();
	HeaderFont.Size = 17;
	Header->SetFont(HeaderFont);
	UVerticalBoxSlot* HeaderSlot = DetailButtonBox->AddChildToVerticalBox(Header);
	HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	UButton* FrontButton = BuildButton(FText::FromString(TEXT("Frente Agresivo")));
	FrontButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnFrontClicked);
	UVerticalBoxSlot* FrontSlot = DetailButtonBox->AddChildToVerticalBox(FrontButton);
	FrontSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));

	UButton* WheelsButton = BuildButton(FText::FromString(TEXT("Detalle de Rines")));
	WheelsButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnWheelsClicked);
	UVerticalBoxSlot* WheelsSlot = DetailButtonBox->AddChildToVerticalBox(WheelsButton);
	WheelsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));

	UButton* SpoilerButton = BuildButton(FText::FromString(TEXT("Aerodinamica Trasera")));
	SpoilerButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnSpoilerClicked);
	UVerticalBoxSlot* SpoilerSlot = DetailButtonBox->AddChildToVerticalBox(SpoilerButton);
	SpoilerSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));

	UButton* MotorButton = BuildButton(FText::FromString(TEXT("Ver Motor")));
	MotorButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnMotorClicked);
	UVerticalBoxSlot* MotorSlot = DetailButtonBox->AddChildToVerticalBox(MotorButton);
	MotorSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));

	UButton* OrbitButton = BuildButton(FText::FromString(TEXT("Camara Orbital")));
	OrbitButton->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnOrbitClicked);
	UVerticalBoxSlot* OrbitSlot = DetailButtonBox->AddChildToVerticalBox(OrbitButton);
	OrbitSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	UTextBlock* GarageColorLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GarageColorLabel"));
	GarageColorLabel->SetText(FText::FromString(TEXT("COLOR GARAJE")));
	GarageColorLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.0f, 1.0f)));
	FSlateFontInfo GarageColorLabelFont = GarageColorLabel->GetFont();
	GarageColorLabelFont.Size = 14;
	GarageColorLabel->SetFont(GarageColorLabelFont);
	UVerticalBoxSlot* GarageColorLabelSlot = DetailButtonBox->AddChildToVerticalBox(GarageColorLabel);
	GarageColorLabelSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));

	UUniformGridPanel* GaragePalette = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("GarageColorPalette"));
	auto AddSwatchSlot = [](UUniformGridPanel* Grid, UButton* Swatch, int32 Row, int32 Column)
	{
		UUniformGridSlot* Slot = Grid->AddChildToUniformGrid(Swatch, Row, Column);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	};

	UButton* CharcoalSwatch = BuildGarageSwatchButton(FLinearColor(0.11f, 0.12f, 0.135f, 1.0f));
	CharcoalSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageCharcoalClicked);
	AddSwatchSlot(GaragePalette, CharcoalSwatch, 0, 0);

	UButton* ConcreteSwatch = BuildGarageSwatchButton(FLinearColor(0.36f, 0.35f, 0.32f, 1.0f));
	ConcreteSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageConcreteClicked);
	AddSwatchSlot(GaragePalette, ConcreteSwatch, 0, 1);

	UButton* NightBlueSwatch = BuildGarageSwatchButton(FLinearColor(0.035f, 0.075f, 0.16f, 1.0f));
	NightBlueSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageNightBlueClicked);
	AddSwatchSlot(GaragePalette, NightBlueSwatch, 0, 2);

	UButton* WineSwatch = BuildGarageSwatchButton(FLinearColor(0.22f, 0.035f, 0.065f, 1.0f));
	WineSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageWineClicked);
	AddSwatchSlot(GaragePalette, WineSwatch, 1, 0);

	UButton* ForestSwatch = BuildGarageSwatchButton(FLinearColor(0.035f, 0.16f, 0.09f, 1.0f));
	ForestSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageForestClicked);
	AddSwatchSlot(GaragePalette, ForestSwatch, 1, 1);

	UButton* TealSwatch = BuildGarageSwatchButton(FLinearColor(0.025f, 0.18f, 0.20f, 1.0f));
	TealSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageTealClicked);
	AddSwatchSlot(GaragePalette, TealSwatch, 1, 2);

	UButton* PurpleSwatch = BuildGarageSwatchButton(FLinearColor(0.13f, 0.055f, 0.22f, 1.0f));
	PurpleSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGaragePurpleClicked);
	AddSwatchSlot(GaragePalette, PurpleSwatch, 2, 0);

	UButton* AmberSwatch = BuildGarageSwatchButton(FLinearColor(0.40f, 0.22f, 0.06f, 1.0f));
	AmberSwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageAmberClicked);
	AddSwatchSlot(GaragePalette, AmberSwatch, 2, 1);

	UButton* LightGreySwatch = BuildGarageSwatchButton(FLinearColor(0.56f, 0.58f, 0.60f, 1.0f));
	LightGreySwatch->OnClicked.AddDynamic(this, &UPremiumGarageWidget::OnGarageLightGreyClicked);
	AddSwatchSlot(GaragePalette, LightGreySwatch, 2, 2);

	UVerticalBoxSlot* PaletteSlot = DetailButtonBox->AddChildToVerticalBox(GaragePalette);
	PaletteSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));

	UBorder* TextPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightDetailPanel"));
	TextPanel->SetBrushColor(FLinearColor(0.012f, 0.015f, 0.020f, 0.70f));
	TextPanel->SetPadding(FMargin(24.f, 22.f, 24.f, 22.f));
	UCanvasPanelSlot* RightSlot = RootCanvas->AddChildToCanvas(TextPanel);
	RightSlot->SetAnchors(FAnchors(1.f, 0.14f, 1.f, 0.14f));
	RightSlot->SetOffsets(FMargin(-460.f, 0.f, 420.f, 392.f));
	RightSlot->SetAlignment(FVector2D(0.f, 0.f));

	DetailTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailTextBlock"));
	DetailTextBlock->SetAutoWrapText(true);
	DetailTextBlock->SetWrapTextAt(390.f);
	DetailTextBlock->SetMinDesiredWidth(390.f);
	DetailTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.96f, 1.0f, 1.0f)));
	FSlateFontInfo DetailFont = DetailTextBlock->GetFont();
	DetailFont.Size = 18;
	DetailTextBlock->SetFont(DetailFont);
	DetailTextBlock->SetText(FText::FromString(TEXT("Selecciona una camara de detalle.")));
	TextPanel->AddChild(DetailTextBlock);

	return Super::RebuildWidget();
}

void UPremiumGarageWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

UButton* UPremiumGarageWidget::BuildButton(const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(FSlateColorBrush(FLinearColor(0.035f, 0.042f, 0.052f, 0.84f)));
	ButtonStyle.SetHovered(FSlateColorBrush(FLinearColor(0.16f, 0.18f, 0.21f, 0.94f)));
	ButtonStyle.SetPressed(FSlateColorBrush(FLinearColor(0.22f, 0.24f, 0.28f, 1.0f)));
	ButtonStyle.SetNormalPadding(FMargin(10.f, 5.f));
	ButtonStyle.SetPressedPadding(FMargin(10.f, 6.f, 10.f, 4.f));
	Button->SetStyle(ButtonStyle);

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetJustification(ETextJustify::Center);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font = LabelText->GetFont();
	Font.Size = 14;
	LabelText->SetFont(Font);
	Button->AddChild(LabelText);
	return Button;
}

UButton* UPremiumGarageWidget::BuildColorButton(const FLinearColor& Color, const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(FSlateColorBrush(Color));
	ButtonStyle.SetHovered(FSlateColorBrush(Color + FLinearColor(0.08f, 0.08f, 0.08f, 0.f)));
	ButtonStyle.SetPressed(FSlateColorBrush(Color * 0.75f));
	ButtonStyle.SetNormalPadding(FMargin(12.f, 5.f));
	ButtonStyle.SetPressedPadding(FMargin(12.f, 6.f, 12.f, 4.f));
	Button->SetStyle(ButtonStyle);

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetJustification(ETextJustify::Center);
	const float Luminance = Color.R * 0.299f + Color.G * 0.587f + Color.B * 0.114f;
	LabelText->SetColorAndOpacity(FSlateColor(Luminance > 0.48f ? FLinearColor(0.02f, 0.02f, 0.025f, 1.0f) : FLinearColor::White));
	FSlateFontInfo Font = LabelText->GetFont();
	Font.Size = 12;
	LabelText->SetFont(Font);
	Button->AddChild(LabelText);
	return Button;
}

UButton* UPremiumGarageWidget::BuildGarageSwatchButton(const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(FSlateColorBrush(Color));
	ButtonStyle.SetHovered(FSlateColorBrush(Color + FLinearColor(0.10f, 0.10f, 0.10f, 0.0f)));
	ButtonStyle.SetPressed(FSlateColorBrush(Color * 0.72f));
	ButtonStyle.SetNormalPadding(FMargin(12.f, 8.f));
	ButtonStyle.SetPressedPadding(FMargin(12.f, 9.f, 12.f, 7.f));
	Button->SetStyle(ButtonStyle);

	UTextBlock* Filler = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Filler->SetText(FText::FromString(TEXT("      ")));
	FSlateFontInfo Font = Filler->GetFont();
	Font.Size = 13;
	Filler->SetFont(Font);
	Filler->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.f)));
	Button->AddChild(Filler);
	return Button;
}

USlider* UPremiumGarageWidget::BuildRgbSlider(const FLinearColor& BarColor, float InitialValue)
{
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetMinValue(0.0f);
	Slider->SetMaxValue(1.0f);
	Slider->SetStepSize(0.01f);
	Slider->SetValue(InitialValue);
	Slider->SetSliderBarColor(BarColor);
	Slider->SetSliderHandleColor(FLinearColor(0.94f, 0.96f, 1.0f, 1.0f));
	return Slider;
}

void UPremiumGarageWidget::UpdateGarageRgbColor()
{
	if (!Controller || !GarageRedSlider || !GarageGreenSlider || !GarageBlueSlider)
	{
		return;
	}

	Controller->SetGarageWallColor(FLinearColor(
		GarageRedSlider->GetValue(),
		GarageGreenSlider->GetValue(),
		GarageBlueSlider->GetValue(),
		1.0f
	));
}

UWidget* UPremiumGarageWidget::GenerateVehicleComboItem(FString Item)
{
	UTextBlock* ItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ItemText->SetText(FText::FromString(Item));
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font = ItemText->GetFont();
	Font.Size = 13;
	ItemText->SetFont(Font);
	return ItemText;
}

void UPremiumGarageWidget::UpdateDetailText(const FString& VehicleName, const FString& DetailName, const FString& BodyText)
{
	if (DetailTextBlock)
	{
		DetailTextBlock->SetText(FText::FromString(VehicleName + TEXT("\n") + DetailName + TEXT("\n\n") + BodyText));
	}
}

void UPremiumGarageWidget::OnWheelsClicked()
{
	if (Controller) Controller->ShowDetail(EPremiumGarageDetail::Wheels);
}

void UPremiumGarageWidget::OnSpoilerClicked()
{
	if (Controller) Controller->ShowDetail(EPremiumGarageDetail::Spoiler);
}

void UPremiumGarageWidget::OnFrontClicked()
{
	if (Controller) Controller->ShowDetail(EPremiumGarageDetail::Front);
}

void UPremiumGarageWidget::OnOrbitClicked()
{
	if (Controller) Controller->ShowDetail(EPremiumGarageDetail::Orbit);
}

void UPremiumGarageWidget::OnMotorClicked()
{
	if (Controller) Controller->ShowDetail(EPremiumGarageDetail::Motor);
}

void UPremiumGarageWidget::OnGarageColorClicked()
{
	if (Controller) Controller->CycleGarageWallColor();
}

void UPremiumGarageWidget::OnGarageCharcoalClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.11f, 0.12f, 0.135f, 1.0f));
}

void UPremiumGarageWidget::OnGarageConcreteClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.36f, 0.35f, 0.32f, 1.0f));
}

void UPremiumGarageWidget::OnGarageNightBlueClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.035f, 0.075f, 0.16f, 1.0f));
}

void UPremiumGarageWidget::OnGarageWineClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.22f, 0.035f, 0.065f, 1.0f));
}

void UPremiumGarageWidget::OnGarageForestClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.035f, 0.16f, 0.09f, 1.0f));
}

void UPremiumGarageWidget::OnGarageTealClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.025f, 0.18f, 0.20f, 1.0f));
}

void UPremiumGarageWidget::OnGaragePurpleClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.13f, 0.055f, 0.22f, 1.0f));
}

void UPremiumGarageWidget::OnGarageAmberClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.40f, 0.22f, 0.06f, 1.0f));
}

void UPremiumGarageWidget::OnGarageLightGreyClicked()
{
	if (Controller) Controller->SetGarageWallColor(FLinearColor(0.56f, 0.58f, 0.60f, 1.0f));
}

void UPremiumGarageWidget::OnGarageRedChanged(float Value)
{
	UpdateGarageRgbColor();
}

void UPremiumGarageWidget::OnGarageGreenChanged(float Value)
{
	UpdateGarageRgbColor();
}

void UPremiumGarageWidget::OnGarageBlueChanged(float Value)
{
	UpdateGarageRgbColor();
}

void UPremiumGarageWidget::OnVehicleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (Controller && !SelectedItem.IsEmpty())
	{
		Controller->SelectVehicleByName(SelectedItem);
	}
}

void UPremiumGarageWidget::OnOriginalPaintClicked()
{
	if (Controller) Controller->ResetBodyColor();
}

void UPremiumGarageWidget::OnRedPaintClicked()
{
	if (Controller) Controller->ApplyBodyColor(FLinearColor(0.72f, 0.02f, 0.015f, 1.f));
}

void UPremiumGarageWidget::OnBlackPaintClicked()
{
	if (Controller) Controller->ApplyBodyColor(FLinearColor(0.005f, 0.005f, 0.006f, 1.f));
}

void UPremiumGarageWidget::OnSilverPaintClicked()
{
	if (Controller) Controller->ApplyBodyColor(FLinearColor(0.62f, 0.64f, 0.66f, 1.f));
}

void UPremiumGarageWidget::OnWhitePaintClicked()
{
	if (Controller) Controller->ApplyBodyColor(FLinearColor(0.92f, 0.90f, 0.84f, 1.f));
}

void UPremiumGarageWidget::OnBluePaintClicked()
{
	if (Controller) Controller->ApplyBodyColor(FLinearColor(0.02f, 0.18f, 0.62f, 1.f));
}
