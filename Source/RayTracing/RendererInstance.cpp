#include "RendererInstance.h"

RendererType* GRenderer;

void InitializeRenderer( uint32 WindowWidth, uint32 WindowHeight) {
	GRenderer = new RendererType(WindowWidth, WindowHeight);
}

RendererType* GetRenderer() {
	return GRenderer;
}

void ReleaseRenderer() {
	delete GRenderer;
}
