// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "SimulationHUD.h"
#include "SnowSimulationGameMode.h"

ASnowSimulationGameMode::ASnowSimulationGameMode()
{
	//DefaultPawnClass = APlayerCharacter::StaticClass();
	HUDClass = ASimulationHUD::StaticClass();
}
