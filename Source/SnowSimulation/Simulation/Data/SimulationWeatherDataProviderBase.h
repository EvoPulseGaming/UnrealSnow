// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationWeatherDataProviderBase.generated.h"

class ASnowSimulationActor;

/**
* Precipitation data.
*/
USTRUCT(Blueprintable)
struct FPrecipitation
{
	GENERATED_USTRUCT_BODY()
	// Amount of precipitation in liter/(m^2) = mm.
	float Amount;
};

/**
* Temperature data.
*/
USTRUCT(Blueprintable)
struct FTemperature
{
	GENERATED_USTRUCT_BODY()

	/** Minimum Temperature in degree Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float AverageLow;

	/** Maximum Temperature in degree Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float AverageHigh;

	/** Mean Temperature in degree Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Average;

	/** Resolution of the temperature data in Ticks (100 nanoseconds resolution). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Resolution;

	FTemperature(float AverageLow, float AverageHigh, float Average) : AverageLow(AverageLow), AverageHigh(AverageHigh), Average(Average) {}

	FTemperature() : AverageLow(0), AverageHigh(0), Average(0) {}
};

// @TODO simplify API because most weather data sources only provide monthly or daily average values.
// @TODO use stochastic downscaling for hourly weather data
/**
 * Base class for all data providers for the simulation.
 */
UCLASS(BlueprintType)
class SNOWSIMULATION_API USimulationWeatherDataProviderBase : public UObject
{
	GENERATED_BODY()

public:
	/** 
	* Returns the temperature data at base elevation at the given day of the year and position (2D).
	*/
	virtual FTemperature GetTemperatureData(const FDateTime& Date, const FVector2D& Position, ASnowSimulationActor* Simulation, int64 Resolution)  PURE_VIRTUAL(USimulationDataProviderBase::GetTemperatureData, return FTemperature(););

	/**
	* Returns the precipitation in mm (liter/m^2) at base elevation at the given time and position (2D).
	*/
	virtual float GetPrecipitationAt(const FDateTime& Date, const FVector2D& Position, int64 Resolution) PURE_VIRTUAL(USimulationDataProviderBase::GetPrecipitationAt, return 0.0f;);

	/** Returns the resolution of the data provider. */
	virtual int32 GetResolution() PURE_VIRTUAL(USimulationDataProviderBase::GetResolution, return 0;);
};

