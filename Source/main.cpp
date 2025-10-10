#include <Windows.h>
#include "RayTracing/RayTracingApp.h"
#include <stdlib.h>

/*
class FDerived;

class FBase {
public:
	FBase(FDerived* InObj): Derived(InObj) {
	}
	virtual ~FBase();
	virtual void VFunc() = 0;
private:
	FDerived* Derived;
};

class FDerived : public FBase{
public:
	FDerived(): FBase(this) {
	}
	virtual ~FDerived() override {
		printf("_____~FDerived\n");
	}
	void VFunc() override {
		printf("_____VFunc0_Derived\n");
	}
};

FBase::~FBase() {
	Derived->VFunc();
}

static void PureVirtualCall() {
	printf("In _purecall_handler.\n");
	std::abort();
}
*/

int WINAPI main(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	RayTracingApp app(hInstance);
	app.Run();
}