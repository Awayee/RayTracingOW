#include <Windows.h>
#include "RayTracing/RayTracingApp.h"
int WINAPI main(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	RayTracingApp app(hInstance);
	app.Run();
}