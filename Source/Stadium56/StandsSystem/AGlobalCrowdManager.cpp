// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/AGlobalCrowdManager.h"

// Sets default values
AAGlobalCrowdManager::AAGlobalCrowdManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAGlobalCrowdManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAGlobalCrowdManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

