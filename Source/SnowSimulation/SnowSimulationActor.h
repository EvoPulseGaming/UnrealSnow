// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SnowSimulation.h"
#include "GameFramework/Actor.h"
#include "GenericPlatformFile.h"
#include "SimulationDataProviderBase.h"
#include "SimulationBase.h"
#include "DefaultDataProvider.h"
#include "SnowSimulationActor.generated.h"




UCLASS()
class SNOWSIMULATION_API ASnowSimulationActor : public AActor
{
	GENERATED_BODY()
	
public:	

	//@TODO make cell creation algorithm independent of section size
	/** Size of one cell of the simulation, should be divisible by the quad section size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	int CellSize = 9;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Render the simulation grid over the landscape. */
	bool RenderGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Data input for the simulation. */
	USimulationDataProviderBase* Data;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The simulation used. */
	USimulationBase* Simulation;

	TArray<FSimulationCell> Cells;

	ASnowSimulationActor();

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;
	
	/** Called every frame */
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	// Called after a property has changed
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	/** Removes old cells and creates the cells for the simulation. */
	void CreateCells();

private:

	/*
	*
	*/
	FSimulationCell* GetCheckedCell(int Index);

};
