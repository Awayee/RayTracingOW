#include "RHI/RHIInstance.h"

RHIType* GRenderer;

void InitializeRHI( uint32 WindowWidth, uint32 WindowHeight) {
	GRenderer = new RHIType(WindowWidth, WindowHeight);
}

RHIType* GetRHI() {
	return GRenderer;
}

void ReleaseRHI() {
	delete GRenderer;
}
