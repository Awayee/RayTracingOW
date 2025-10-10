#include "RendererInstance.h"

RendererType* GRenderer;

void InitializeRenderer(AppInstanceHandle AppInstance, uint32 WindowWidth, uint32 WindowHeight) {
	GRenderer = new RendererType(AppInstance, WindowWidth, WindowHeight);
}

RendererType* GetRenderer() {
	return GRenderer;
}

void ReleaseRenderer() {
	delete GRenderer;
}
