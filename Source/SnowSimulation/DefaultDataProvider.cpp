// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "DefaultDataProvider.h"

// @TODO At the beginning create temperature as well as precipitation data from the input.

FTemperature UDefaultSimulationDataProvider::GetTemperatureData(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	FDateTime Time = From;
	FTimespan Timespan = To - From;

	float AverageLow = 0.0f;
	float AverageHigh = 0.0f;
	float Average = 0.0f;

	int TotalTicks = Timespan.GetTicks() / Resolution;

	for (int64 Tick = 0; Tick < Timespan.GetTicks(); Tick += Resolution)
	{
		auto MonthIndex = Time.GetMonth() - 1;
		auto AverageMonthlyTemperature = MonthlyTemperatures[MonthIndex];

		AverageLow += AverageMonthlyTemperature.AverageLow;
		AverageHigh += AverageMonthlyTemperature.AverageHigh;
		Average += AverageMonthlyTemperature.Average;
	}

	float T = FMath::FRandRange(-4, 4);

	return FTemperature(AverageLow / TotalTicks + T, AverageHigh / TotalTicks, Average / TotalTicks + T);
}

float UDefaultSimulationDataProvider::GetPrecipitationAt(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	FDateTime Time = From;
	FTimespan Timespan = To - From;

	float AveragePrecipitation = 0.0f;

	for (int64 Tick = 0; Tick < Timespan.GetTicks(); Tick += Resolution)
	{
		auto MonthIndex = Time.GetMonth() - 1;
		auto Precipitation = MonthlyPrecipitation[MonthIndex];
		int64 TicksPerMonth = Precipitation.AverageNumberOfDays *  ETimespan::TicksPerDay;
		
		// @TODO we only assume precipitation to be at the beginning of the month
		if (Time.GetDay() < Precipitation.AverageNumberOfDays) {
			auto AverageMonthlyPrecipitation = Precipitation.AveragePrecipitation;

			AveragePrecipitation += AverageMonthlyPrecipitation / TicksPerMonth * Resolution;
		}
	}

	return AveragePrecipitation;
}

float UDefaultSimulationDataProvider::GetVegetationDensityAt(const FVector& Position)
{
	return 0.0f;
}
