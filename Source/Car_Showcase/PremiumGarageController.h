#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "PremiumGarageController.generated.h"

class UTextBlock;
class UButton;
class UComboBoxString;
class UHorizontalBox;
class USlider;
class UUniformGridPanel;
class UVerticalBox;
class UWidget;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EPremiumGarageDetail : uint8
{
	Wheels,
	Spoiler,
	Front,
	Motor,
	Orbit
};

UCLASS()
class CAR_SHOWCASE_API UPremiumGarageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Premium Garage", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailTextBlock;

	void SetController(class APremiumGarageController* InController);
	void UpdateDetailText(const FString& VehicleName, const FString& DetailName, const FString& BodyText);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<APremiumGarageController> Controller;

	UPROPERTY()
	TObjectPtr<UVerticalBox> DetailButtonBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> VehicleComboBox;

	UPROPERTY()
	TObjectPtr<USlider> GarageRedSlider;

	UPROPERTY()
	TObjectPtr<USlider> GarageGreenSlider;

	UPROPERTY()
	TObjectPtr<USlider> GarageBlueSlider;

	UFUNCTION()
	void OnWheelsClicked();

	UFUNCTION()
	void OnSpoilerClicked();

	UFUNCTION()
	void OnFrontClicked();

	UFUNCTION()
	void OnOrbitClicked();

	UFUNCTION()
	void OnMotorClicked();

	UFUNCTION()
	void OnGarageColorClicked();

	UFUNCTION()
	void OnGarageCharcoalClicked();

	UFUNCTION()
	void OnGarageConcreteClicked();

	UFUNCTION()
	void OnGarageNightBlueClicked();

	UFUNCTION()
	void OnGarageWineClicked();

	UFUNCTION()
	void OnGarageForestClicked();

	UFUNCTION()
	void OnGarageTealClicked();

	UFUNCTION()
	void OnGaragePurpleClicked();

	UFUNCTION()
	void OnGarageAmberClicked();

	UFUNCTION()
	void OnGarageLightGreyClicked();

	UFUNCTION()
	void OnGarageRedChanged(float Value);

	UFUNCTION()
	void OnGarageGreenChanged(float Value);

	UFUNCTION()
	void OnGarageBlueChanged(float Value);

	UFUNCTION()
	void OnVehicleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	UWidget* GenerateVehicleComboItem(FString Item);

	UFUNCTION()
	void OnOriginalPaintClicked();

	UFUNCTION()
	void OnRedPaintClicked();

	UFUNCTION()
	void OnBlackPaintClicked();

	UFUNCTION()
	void OnSilverPaintClicked();

	UFUNCTION()
	void OnWhitePaintClicked();

	UFUNCTION()
	void OnBluePaintClicked();

	UButton* BuildButton(const FText& Label);
	UButton* BuildColorButton(const FLinearColor& Color, const FText& Label);
	UButton* BuildGarageSwatchButton(const FLinearColor& Color);
	USlider* BuildRgbSlider(const FLinearColor& BarColor, float InitialValue);
	void UpdateGarageRgbColor();
};

UCLASS()
class CAR_SHOWCASE_API APremiumGarageController : public AActor
{
	GENERATED_BODY()

public:
	APremiumGarageController();

	UFUNCTION(BlueprintCallable, Category = "Premium Garage")
	void ShowDetail(EPremiumGarageDetail Detail);

	UFUNCTION(BlueprintCallable, Category = "Premium Garage")
	void SelectVehicleByName(const FString& VehicleName);

	UFUNCTION(BlueprintCallable, Category = "Premium Garage")
	void ApplyBodyColor(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Premium Garage")
	void ResetBodyColor();

	UFUNCTION(BlueprintCallable, Category = "Premium Garage")
	void CycleGarageWallColor();

	UFUNCTION(BlueprintCallable, Category = "Premium Garage")
	void SetGarageWallColor(FLinearColor Color);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UPremiumGarageWidget> RuntimeWidget;

	UPROPERTY()
	FName ActiveVehicleTag;

	UPROPERTY()
	TMap<FString, TObjectPtr<UMaterialInterface>> OriginalPaintMaterials;

	UPROPERTY()
	TSet<FString> PaintMaterialKeys;

	UPROPERTY()
	TObjectPtr<AActor> TransitionVehicleActor;

	EPremiumGarageDetail ActiveDetail = EPremiumGarageDetail::Front;
	float OrbitAngleDegrees = 0.0f;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.35f;
	FVector TransitionTargetScale = FVector::OneVector;
	int32 GarageColorIndex = 0;

	FName GetCurrentVehicleTag() const;
	FString GetCurrentVehicleName(FName VehicleTag) const;
	FString GetDetailText(FName VehicleTag, EPremiumGarageDetail Detail) const;
	FName GetCameraTag(EPremiumGarageDetail Detail) const;
	FName GetVehicleTagFromName(const FString& VehicleName) const;
	FString GetVehicleNameFromTag(FName VehicleTag) const;
	bool IsPaintMaterialName(FName VehicleTag, const UStaticMeshComponent* MeshComponent, const FString& MaterialName) const;
	FString GetPaintMaterialKey(FName VehicleTag, const UStaticMeshComponent* MeshComponent, int32 MaterialSlot) const;
	int32 GetMainPaintMaterialSlot(FName VehicleTag) const;
	AActor* FindFirstActorWithTag(FName Tag) const;
	AActor* GetActiveVehicleActor() const;
	void SetCameraByTag(FName CameraTag);
	void SetVehicleVisibility(FName VehicleTag, bool bVisible);
	void ApplyGarageWallColor(const FLinearColor& Color);
	void CacheOriginalPaintMaterials();
	void BeginVehicleTransition(AActor* VehicleActor);
	void UpdateVehicleTransition(float DeltaSeconds);
	void UpdateOrbitCamera(float DeltaSeconds);
};
