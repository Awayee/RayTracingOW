#include "RayTracing/RayTracingApp.h"
#include "RayTracing/RayTracingScene.h"
#include "RayTracing/RayTracingCamera.h"
#include "RayTracing/RayTracingRenderer.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Math/Vector.h"
#include "Math/MathUtil.h"
#include "Math/Geometry.h"
#include "RHI/RHIInstance.h"

static constexpr uint32 WINDOW_WIDTH = 800;
static constexpr uint32 WINDOW_HEIGHT = 450;
static constexpr uint32 NUM_RAYS_PER_PIXEL = 32;
static constexpr uint32 RAY_RECURSIVE_DEPTH = 16;
static constexpr float RENDER_SCALE =
#ifdef _DEBUG
0.5f
#else
1.0f
#endif
;

static void InitializeScene0(RayTracingCamera* Camera, RayTracingScene* scene) {
	auto materialGround = MaterialPtr(new LambertMaterial({0.8f, 0.8f, 0.0f, 1.0f}));
	auto materialMiddle = MaterialPtr(new LambertMaterial({0.1f, 0.2f, 0.5f, 1.0f}));
	auto materialLeft = MaterialPtr(new DielectricMaterial(1.5f));
	auto materialBubble = MaterialPtr(new DielectricMaterial(1.0f / 1.5f));
	auto materialRight = MaterialPtr(new MetalMaterial({ 0.8f, 0.6f, 0.2f, 1.0f }, 1.0f));
	scene->AddSphere({ {0.0f, -100.5f, -1.0f}, 100.0f }, MoveTemp(materialGround));
	scene->AddSphere({ {0.0f, 0.0f, -1.0f}, 0.5f }, MoveTemp(materialMiddle));
	scene->AddSphere({ {-1.0f, 0.0f, -1.0f}, 0.5f }, MoveTemp(materialLeft));
	scene->AddSphere({ {-1.0f, 0.0f, -1.0f}, 0.4f }, MoveTemp(materialBubble));
	scene->AddSphere({ {1.0f, 0.0f, -1.0f}, 0.5f }, MoveTemp(materialRight));

	//auto R = Math::Cos(Math::PI / 4);
	//auto materialLeft = MaterialPtr(new LambertMaterial({ 0.0f, 0.0f, 1.0f, 1.0f }));
	//auto materialRight = MaterialPtr(new LambertMaterial({ 1.0f, 0.0f, 0.0f, 1.0f }));
	//scene->AddSphere({ {-R,0,-1}, R }, MoveTemp(materialLeft));
	//scene->AddSphere({ {R,0,-1}, R }, MoveTemp(materialRight));
}

static void InitializeScene1(RayTracingCamera* Camera, RayTracingScene* Scene) {
	auto materialGround = MaterialPtr(new LambertMaterial(TexturePtr(new CheckerTexture(Math::Color8{0.2f, 0.3f, 0.1f, 1.0f}, Math::Color8{0.9f, 0.9f, 0.8f, 1.0f}, 0.32f))));
	Scene->AddSphere({ {0.0f, -1000.0f, 0.0f}, 1000.0f }, MoveTemp(materialGround));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float chooseMaterial = Math::Random01();
			Math::FVector3 center((float)a + 0.9f * Math::Random01(), 0.2f, (float)b + 0.9f * Math::Random01());

			if ((center - Math::FVector3(4.0f, 0.2f, 0.0f)).Length() > 0.9f) {
				if (chooseMaterial < 0.8f) {
					// diffuse
					auto albedo = Math::Random01Vector() * Math::Random01Vector();
					auto mat = MaterialPtr(new LambertMaterial(albedo));
					auto MoveTarget = center + Math::FVector3{0, Math::Random(0, 0.5f), 0};
					Scene->AddMovableSphere({ center, 0.2f }, MoveTemp(mat), MoveTarget);
				}
				else if (chooseMaterial < 0.95f) {
					// metal
					auto albedo = Math::RandomVector(0.5f, 1.0f);
					auto fuzz = Math::Random(0.0f, 0.5f);
					auto mat = MaterialPtr(new MetalMaterial(albedo, fuzz));
					Scene->AddSphere({ center, 0.2f }, MoveTemp(mat));
				}
				else {
					// glass
					auto mat = MaterialPtr(new DielectricMaterial(1.5f));
					Scene->AddSphere({ center, 0.2f }, MoveTemp(mat));
				}
			}
		}
	}
	Scene->AddSphere({ {0.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new DielectricMaterial(1.5f)));
	Scene->AddSphere({ {-4.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new LambertMaterial({ 0.4f, 0.2f, 0.1f })));
	Scene->AddSphere({ {4.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new MetalMaterial({ 0.7f, 0.6f, 0.5f }, 0.0f)));
}

static void InitializeScene2(RayTracingCamera* Camera, RayTracingScene* Scene) {
	const Math::Color8 ColorEven{0.2f, 0.3f, 0.1f, 1.0f};
	const Math::Color8 ColorOdd{0.9f, 0.9f, 0.8f, 1.0f};
	Scene->AddObject(TUniquePtr<RTSphere>(new RTSphere({{0.0f, 10.0f, 0.0f}, 10.0f}, MaterialPtr(new LambertMaterial(TexturePtr(new CheckerTexture(ColorEven, ColorOdd, 0.32f)))))));
	Scene->AddObject(TUniquePtr<RTSphere>(new RTSphere({ {0.0f, -10.0f, 0.0f}, 10.0f }, MaterialPtr(new LambertMaterial(TexturePtr(new CheckerTexture(ColorEven, ColorOdd, 0.32f)))))));
}

static void InitializeEarth(RayTracingCamera* Camera, RayTracingScene* Scene) {
	MaterialPtr EarthMaterial{new LambertMaterial(TexturePtr{new ImageTexture("earthmap.jpg")})};
	Scene->AddObject(TUniquePtr<RTSphere>(new RTSphere(Math::FSphere{{0.0f, 0.0f, 0.0f}, 2.0f}, std::move(EarthMaterial))));
}

static void InitializePerlinSpheres(RayTracingCamera* Camera, RayTracingScene* Scene) {
	Scene->AddObject(TUniquePtr<RTSphere>(new RTSphere({ {0.0f,-1000.0f,0.0f}, 1000.0f}, MaterialPtr(new LambertMaterial{TexturePtr(new NoiseTexture(4.0f))}))));
	Scene->AddObject(TUniquePtr<RTSphere>(new RTSphere({ {0.0f,2.0f,0.0f}, 2.0f }, MaterialPtr(new LambertMaterial{ TexturePtr(new NoiseTexture(4.0f)) }))));
}

static void InitializeQuads(RayTracingCamera* Camera, RayTracingScene* Scene) {

	// Materials
	MaterialPtr LeftRed     = MaterialPtr(new LambertMaterial(Math::FVector3{1.0f, 0.2f, 0.2f}));
	MaterialPtr BackGreen   = MaterialPtr(new LambertMaterial(Math::FVector3{0.2f, 1.0f, 0.2f}));
	MaterialPtr RightBlue   = MaterialPtr(new LambertMaterial(Math::FVector3{0.2f, 0.2f, 1.0f}));
	MaterialPtr UpperOrange = MaterialPtr(new LambertMaterial(Math::FVector3{1.0f, 0.5f, 0.0f}));
	MaterialPtr LowerTeal   = MaterialPtr(new LambertMaterial(Math::FVector3{0.2f, 0.8f, 0.8f}));

	Scene->AddObject(TUniquePtr<RTQuad>(new RTQuad(Math::FQuad{ {-3.0f, -2.0f, 5.0f}, {0.0f, 0.0f, -4.0f}, {0.0f, 4.0f, 0.0f}}, MoveTemp(LeftRed))));
	Scene->AddObject(TUniquePtr<RTQuad>(new RTQuad(Math::FQuad{ {-2.0f, -2.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 4.0f, 0.0f} }, MoveTemp(BackGreen))));
	Scene->AddObject(TUniquePtr<RTQuad>(new RTQuad(Math::FQuad{ {3.0f, -2.0f, 1.0f}, {0.0f, 0.0f, 4.0f}, {0.0f, 4.0f, 0.0f} }, MoveTemp(RightBlue))));
	Scene->AddObject(TUniquePtr<RTQuad>(new RTQuad(Math::FQuad{ {-2.0f, 3.0f, 1.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 4.0f} }, MoveTemp(UpperOrange))));
	Scene->AddObject(TUniquePtr<RTQuad>(new RTQuad(Math::FQuad{ {-2.0f, -3.0f, 5.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -4.0f} }, MoveTemp(LowerTeal))));

	Camera->SetView({0.0f, 0.0f, 9.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
	Camera->SetFov(80.0f * Math::Deg2Rad);
}

RayTracingApp::RayTracingApp() {

	// print the cost
	class ProfilePrintScope {
	public:
		ProfilePrintScope(std::string&& info) : m_Info(MoveTemp(info)), m_StartTime(NowTimePoint()) {
			m_StartTime = NowTimePoint();
		}
		~ProfilePrintScope() {
			const auto endTime = NowTimePoint();
			const float durationMS = GetDurationMill<float>(m_StartTime, endTime);
			LOG_INFO("Cost of %s is %.3f ms", m_Info.c_str(), durationMS);
		}
	private:
		std::string m_Info;
		TimePoint m_StartTime;
	};

	// ========== step1. Initialize context ==========
	InitializeRHI( WINDOW_WIDTH, WINDOW_HEIGHT);

	// ========== step2. Scene rendering ==========
	// Create ray tracing scene
	Camera.Reset(new RayTracingCamera({ (uint32)(WINDOW_WIDTH * RENDER_SCALE), (uint32)(WINDOW_HEIGHT * RENDER_SCALE) }));
	Scene.Reset(new RayTracingScene());
	InitializeQuads(Camera.Get(), Scene.Get()); // TODO test
	Camera->SetupRayData();
	Scene->BuildHierarchy();

	RayTracingRenderer Renderer{ Camera.Get(), Scene.Get(), NUM_RAYS_PER_PIXEL, RAY_RECURSIVE_DEPTH};
	// Render the scene
	{
		ProfilePrintScope s{ "Scene Rendering" };
		Renderer.Render();
	}

	RenderResult Result = Renderer.GetRenderResult();
	// ========== step3. Display the render texture ==========
	// Create texture and upload data
	RHITexture = GetRHI()->CreateTexture(Result.Width, Result.Height, 1, 1, ERHIFormat::R8G8B8A8_UNorm);
	GetRHI()->UpdateTextureData(RHITexture, Result.Data, (size_t)Result.Width * (size_t)Result.Height * sizeof(Math::Color8));
}

RayTracingApp::~RayTracingApp() {
	GetRHI()->DestroyTexture(RHITexture);
	ReleaseRHI();
}

void RayTracingApp::Run() {
	while(GetRHI()->DrawTexture(RHITexture)){}
}
