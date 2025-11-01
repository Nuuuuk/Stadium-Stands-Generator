// Fill out your copyright notice in the Description page of Project Settings.


#include "StandsSystem/AGlobalSeatManager.h"

// Sets default values
AAGlobalSeatManager::AAGlobalSeatManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAGlobalSeatManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAGlobalSeatManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

