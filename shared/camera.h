// dcmi.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

uint32_t cameraId();
uint32_t cameraFrame();

bool initCamera (bool continuous);
void startCamera();
void waitCamera();
void showCamera();
