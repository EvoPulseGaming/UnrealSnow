// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Simulation/DegreeDay/DegreeDaySimulation.h"
#include "DegreeDayCPUSimulation.generated.h"


/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain". 
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDegreeDayCPUSimulation : public UDegreeDaySimulation
{
	GENERATED_BODY()
private:
	/** The maximum snow amount (mm) of the current time step. */
	float MaxSnow;

	// @TODO Degrees or radians?
	/**
	* Calculates the solar radiation as described in Swifts "Algorithm for Solar Radiation on Mountain Slopes".
	*
	* @param I		The inclination of the slope in radians.
	* @param A		The aspect (compass direction) that the slope faces in radians.
	* @param L0		The latitude of the slope in radians.
	*
	* @return
	*/
	float SolarRadiationIndex(float I, float A, float L0, float J)
	{
		float L1 = FMath::Asin(FMath::Cos(I) * FMath::Sin(L0) + FMath::Sin(I) * FMath::Cos(L0) * FMath::Cos(A));
		float D1 = FMath::Cos(I) * FMath::Cos(L0) - FMath::Sin(I) * FMath::Sin(L0) * FMath::Cos(A);
		float L2 = FMath::Atan((FMath::Sin(I) * FMath::Sin(A)) / (FMath::Cos(I) * FMath::Cos(L0) - FMath::Sin(I) * FMath::Sin(L0) * FMath::Cos(A)));

		float D = 0.007 - 0.4067 * FMath::Cos((J + 10) * 0.0172);
		float E = 1.0 - 0.0167 * FMath::Cos((J - 3) * 0.0172);

		const float R0 = 1.95;
		float R1 = 60 * R0 / (E * E);
		// float R1 = (PI / 3) * R0 / (E * E);

		float T;

		T = Func2(L1, D);
		float T7 = T - L2;
		float T6 = -T - L2;
		T = Func2(L0, D);
		float T1 = T;
		float T0 = -T;
		float T3 = FMath::Min(T7, T1);
		float T2 = FMath::Max(T6, T0);

		float T4 = T2 * (12 / PI);
		float T5 = T3 * (12 / PI);

		//float R4 = Func3(L2, L1, T3, T2, R1, D); // Figure1
		if (T3 < T2) // Figure2
		{
			T2 = T3 = 0;
		}

		T6 = T6 + PI * 2;

		float R4;
		if (T6 < T1)
		{
			float T8 = T6;
			float T9 = T1;
			R4 = Func3(L2, L1, T3, T2, R1, D) + Func3(L2, L1, T9, T8, R1, D);
		} 
		else
		{
			T7 = T7 - PI * 2;

			if (T7 > T0)
			{
				float T8 = T0;
				float T9 = T0;
				R4 = Func3(L2, L1, T3, T2, R1, D) + Func3(L2, L1, T9, T8, R1, D);
			}
			else
			{
				R4 = Func3(L2, L1, T3, T2, R1, D);
			}
		}

		float R3 = Func3(0.0, L0, T1, T0, R1, D);

		return R4 / R3;
	}

	// @TODO check for invalid latitudes (90�)
	float Func2(float L, float D) // sunrise/sunset
	{
		return FMath::Acos(FMath::Clamp(-FMath::Tan(L) * FMath::Tan(D), -1.0f, 1.0f));
	}

	float Func3(float V, float W, float X, float Y, float R1, float D) // radiation
	{
		return R1 * (FMath::Sin(D) * FMath::Sin(W) * (X - Y) * (12 / PI) + 
			FMath::Cos(D) * FMath::Cos(W) * (FMath::Sin(X + V) - FMath::Sin(Y + V)) * (12 / PI));
	}

public:
	virtual FString GetSimulationName() override final;

	virtual void Simulate(ASnowSimulationActor* SimulationActor, USimulationWeatherDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime, int32 TimeStepHours) override final;

	virtual void Initialize(ASnowSimulationActor* SimulationActor, UWorld* World) override final;
};

