#include "RayTracing/RayTracingApp.h"
#include "RayTracing/RayTracingScene.h"
#include "RayTracing/RayTracingCamera.h"
#include "RayTracing/RayTracingRenderer.h"
#include "RayTracing/RayTracingTransform.h"
#include "RayTracing/RayTracingConstantMedium.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Math/Vector.h"
#include "Math/MathUtil.h"
#include "Math/Geometry.h"
#include "RHI/RHIInstance.h"

static constexpr uint32 WINDOW_WIDTH = 800;
static constexpr uint32 WINDOW_HEIGHT = 450;
static constexpr uint32 RAY_RECURSIVE_DEPTH = 32;
static constexpr uint32 NUM_RAYS_PER_PIXEL =
#ifdef _DEBUG
32
#else
64
#endif
;

static constexpr float RENDER_SCALE =
#ifdef _DEBUG
0.5f
#else
1.0f
#endif
;

#pragma region Utils

inline MaterialPtr SolidColorLambertian(Math::Color8 Color) {
	return MaterialPtr(new LambertMaterial(TexturePtr(new SolidColor(Color))));
}

inline MaterialPtr SolidColorMetal(const Math::FVector4& Color, float Fuzz) {
	return MaterialPtr(new MetalMaterial(Color, Fuzz));
}

inline MaterialPtr EmissiveMaterial(Math::Color8 Color, float Scale) {
	return MaterialPtr(new DiffuseLightMaterial(Color, Scale));
}

inline MaterialPtr ImageLambertian(const char* File) {
	return MaterialPtr(new LambertMaterial(TexturePtr(new ImageTexture(File))));
}

inline MaterialPtr NoiseLambertian(float Scale) {
	return MaterialPtr(new LambertMaterial(TexturePtr(new NoiseTexture(Scale))));
}

inline RTObjectPtr MakeGlassSphere(const Math::FVector3& Center, float Radius, float RefractionIndex) {
	return RTObjectPtr(new RTSphere({Center, Radius}, MaterialPtr(new DielectricMaterial(RefractionIndex))));
}

inline void AddBox(RayTracingScene* Scene, const Math::FVector3& A, const Math::FVector3& B, Math::Color8 Color) {
	// Construct the two opposite vertices with the minimum and maximum coordinates.
	Math::FVector3 Min = Math::FVector3::Min(A, B);
	Math::FVector3 Max = Math::FVector3::Max(A, B);

	Math::FVector3 DX = Math::FVector3{ Max.X - Min.X, 0.0f, 0.0f };
	Math::FVector3 DY = Math::FVector3{ 0.0f, Max.Y - Min.Y, 0.0f };
	Math::FVector3 DZ = Math::FVector3{ 0.0f, 0.0f, Max.Z - Min.Z };

	Scene->AddObject(RTObjectPtr(new RTQuad({ {Min.X, Min.Y, Max.Z}, DX, DY }, SolidColorLambertian(Color)))); // front
	Scene->AddObject(RTObjectPtr(new RTQuad({ {Max.X, Min.Y, Max.Z},-DZ, DY }, SolidColorLambertian(Color)))); // right
	Scene->AddObject(RTObjectPtr(new RTQuad({ {Max.X, Min.Y, Min.Z},-DX, DY }, SolidColorLambertian(Color)))); // back
	Scene->AddObject(RTObjectPtr(new RTQuad({ {Min.X, Min.Y, Min.Z}, DZ, DY }, SolidColorLambertian(Color)))); // left
	Scene->AddObject(RTObjectPtr(new RTQuad({ {Min.X, Max.Y, Max.Z}, DX,-DZ }, SolidColorLambertian(Color)))); // top
	Scene->AddObject(RTObjectPtr(new RTQuad({ {Min.X, Min.Y, Min.Z}, DX, DZ }, SolidColorLambertian(Color)))); // bottom
}

template<class T, class ...Args>
void AddTranslated(RayTracingScene* Scene, const Math::FVector3& Translation, Args...InArgs) {
	Scene->AddObject(RTObjectPtr(new RTTranslated(RTObjectPtr(new T(MoveTemp(InArgs)...)), Translation)));
}

template<class T, class ...Args>
void AddRotatedY(RayTracingScene* Scene, float RotationY, Args...InArgs) {
	Scene->AddObject(RTObjectPtr(new RTRotatedY(RTObjectPtr(new T(MoveTemp(InArgs)...)), RotationY)));
}

template<class T, class...Args>
RTObjectPtr TransformedObject(const Math::FVector3& Translation, float RotationY, Args...InArgs) {
	return RTObjectPtr(new RTTranslated(RTObjectPtr(new RTRotatedY(RTObjectPtr(new T(MoveTemp(InArgs)...)), RotationY)), Translation));
}

#pragma endregion


#pragma region BuildScene

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
	auto materialGround = MaterialPtr(new LambertMaterial(TexturePtr(new CheckerTexture({0.2f, 0.3f, 0.1f, 1.0f}, {0.9f, 0.9f, 0.8f, 1.0f}, 0.32f))));
	Scene->AddSphere({ {0.0f, -1000.0f, 0.0f}, 1000.0f }, MoveTemp(materialGround));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float chooseMaterial = Math::Random01();
			Math::FVector3 center((float)a + 0.9f * Math::Random01(), 0.2f, (float)b + 0.9f * Math::Random01());

			if ((center - Math::FVector3(4.0f, 0.2f, 0.0f)).Length() > 0.9f) {
				if (chooseMaterial < 0.8f) {
					// diffuse
					auto albedo = Math::Random01Vector() * Math::Random01Vector();
					auto mat = MaterialPtr(new LambertMaterial(Math::Color8{albedo}));
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
	Scene->AddSphere({ {-4.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new LambertMaterial(Math::Color8{ 0.4f, 0.2f, 0.1f, 1.0f })));
	Scene->AddSphere({ {4.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new MetalMaterial({ 0.7f, 0.6f, 0.5f }, 0.0f)));
}

static void InitializeScene2(RayTracingCamera* Camera, RayTracingScene* Scene) {
	const Math::Color8 ColorEven{0.2f, 0.3f, 0.1f, 1.0f};
	const Math::Color8 ColorOdd{0.9f, 0.9f, 0.8f, 1.0f};
	Scene->AddObject(RTObjectPtr(new RTSphere({{0.0f, 10.0f, 0.0f}, 10.0f}, MaterialPtr(new LambertMaterial(TexturePtr(new CheckerTexture(ColorEven, ColorOdd, 0.32f)))))));
	Scene->AddObject(RTObjectPtr(new RTSphere({ {0.0f, -10.0f, 0.0f}, 10.0f }, MaterialPtr(new LambertMaterial(TexturePtr(new CheckerTexture(ColorEven, ColorOdd, 0.32f)))))));
}

static void InitializeEarth(RayTracingCamera* Camera, RayTracingScene* Scene) {
	MaterialPtr EarthMaterial{new LambertMaterial(TexturePtr{new ImageTexture("earthmap.jpg")})};
	Scene->AddObject(RTObjectPtr(new RTSphere(Math::FSphere{{0.0f, 0.0f, 0.0f}, 2.0f}, std::move(EarthMaterial))));
}

static void InitializePerlinSpheres(RayTracingCamera* Camera, RayTracingScene* Scene) {
	Scene->AddObject(RTObjectPtr(new RTSphere({ {0.0f,-1000.0f,0.0f}, 1000.0f}, MaterialPtr(new LambertMaterial{TexturePtr(new NoiseTexture(4.0f))}))));
	Scene->AddObject(RTObjectPtr(new RTSphere({ {0.0f,2.0f,0.0f}, 2.0f }, MaterialPtr(new LambertMaterial{ TexturePtr(new NoiseTexture(4.0f)) }))));
}

static void InitializeQuads(RayTracingCamera* Camera, RayTracingScene* Scene) {

	// Materials
	MaterialPtr LeftRed     = MaterialPtr(new LambertMaterial(Math::Color8{1.0f, 0.2f, 0.2f, 1.0f}));
	MaterialPtr BackGreen   = MaterialPtr(new LambertMaterial(Math::Color8{0.2f, 1.0f, 0.2f, 1.0f}));
	MaterialPtr RightBlue   = MaterialPtr(new LambertMaterial(Math::Color8{0.2f, 0.2f, 1.0f, 1.0f}));
	MaterialPtr UpperOrange = MaterialPtr(new LambertMaterial(Math::Color8{1.0f, 0.5f, 0.0f, 1.0f}));
	MaterialPtr LowerTeal   = MaterialPtr(new LambertMaterial(Math::Color8{0.2f, 0.8f, 0.8f, 1.0f}));

	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {-3.0f, -2.0f, 5.0f}, {0.0f, 0.0f, -4.0f}, {0.0f, 4.0f, 0.0f}}, MoveTemp(LeftRed))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {-2.0f, -2.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 4.0f, 0.0f} }, MoveTemp(BackGreen))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {3.0f, -2.0f, 1.0f}, {0.0f, 0.0f, 4.0f}, {0.0f, 4.0f, 0.0f} }, MoveTemp(RightBlue))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {-2.0f, 3.0f, 1.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 4.0f} }, MoveTemp(UpperOrange))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {-2.0f, -3.0f, 5.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -4.0f} }, MoveTemp(LowerTeal))));

	Camera->SetView({0.0f, 0.0f, 9.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
	Camera->SetFov(80.0f * Math::Deg2Rad);
}

static void InitialzeSampleLight(RayTracingCamera* Camera, RayTracingScene* Scene) {
	Scene->AddObject(RTObjectPtr(new RTSphere(Math::FSphere{{0.0f, -1000.0f, 0.0f}, 1000.0f}, MaterialPtr(new LambertMaterial(TexturePtr(new NoiseTexture(4.0f)))))));
	Scene->AddObject(RTObjectPtr(new RTSphere(Math::FSphere{ {0.0f, 2.0f, 0.0f}, 2.0f }, MaterialPtr(new LambertMaterial(TexturePtr(new NoiseTexture(4.0f)))))));
	MaterialPtr QuadLightMaterial{new DiffuseLightMaterial(Math::Color8{1.0f, 1.0f, 1.0f, 1.0f}, 4.0f)};
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{3.0f, 1.0f, -2.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f}}, MoveTemp(QuadLightMaterial))));
	MaterialPtr SphereLightMaterial{ new DiffuseLightMaterial(Math::Color8{1.0f, 1.0f, 1.0f, 1.0f}, 4.0f) };
	Scene->AddObject(RTObjectPtr(new RTSphere(Math::FSphere{ {0.0f, 7.0f, 0.0f}, 2.0f}, MoveTemp(SphereLightMaterial))));
	Scene->SetBackground(TexturePtr(new SolidColor(Math::Color8{0.0f, 0.0f, 0.0f, 1.0f})));

	Camera->SetFov(20.0f * Math::Deg2Rad);
	Camera->SetView({26.0f, 3.0f, 6.0f}, {0.0f, 2.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
	Camera->SetDefocusAngle(0.0f);
}

static void InitializeCornellBox(RayTracingCamera* Camera, RayTracingScene* Scene) {
	const Math::Color8 Red{0.65f, 0.05f, 0.05f, 1.0f};
	const Math::Color8 White{0.73f, 0.73f, 0.73f, 1.0f};
	const Math::Color8 Green{0.12f, 0.45f, 0.15f, 1.0f};
	const Math::Color8 LightColor{1.0f, 1.0f, 1.0f, 1.0f};
	const float LightScale = 15.0f;

	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{555, 0, 0}, {0, 555, 0}, {0, 0, 555}}, SolidColorLambertian(Green))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{0, 0, 0}, {0, 555, 0}, {0, 0, 555}}, SolidColorLambertian(Red))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{343, 554, 332}, {-130, 0, 0}, {0, 0, -105} }, EmissiveMaterial(LightColor, LightScale))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{0, 0, 0}, {555, 0, 0}, {0, 0, 555}}, SolidColorLambertian(White))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{555, 555, 555}, {-555, 0, 0}, {0, 0, -555}}, SolidColorLambertian(White))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{0, 0, 555}, {555, 0, 0}, {0, 555, 0}}, SolidColorLambertian(White))));
	Scene->SetBackground(TexturePtr(new SolidColor({ 0.0f, 0.0f, 0.0f, 1.0f })));

	//AddBox(Scene, {130, 0, 65}, {295, 165, 230}, White);
	//AddBox(Scene, {265, 0, 295}, {430, 330, 460}, White);
	//Scene->AddObject(RTObjectPtr(new RTBox({ 130, 0, 65 }, { 295, 165, 230 }, SolidColorLambertian(White))));
	//Scene->AddObject(RTObjectPtr(new RTBox({ 265, 0, 295 }, { 430, 330, 460 }, SolidColorLambertian(White))));
	Scene->AddObject(TransformedObject<RTBox>({ 265,0,295 }, 15 * Math::Deg2Rad, Math::FVector3{0,0,0}, Math::FVector3{ 165,330,165 }, SolidColorLambertian(White)));
	Scene->AddObject(TransformedObject<RTBox>({ 130,0,65 }, -30 * Math::Deg2Rad, Math::FVector3{ 0,0,0 }, Math::FVector3{ 165,165,165 }, SolidColorLambertian(White)));

	Camera->SetFov(40.0f * Math::Deg2Rad);
	Camera->SetView({278, 278, -800}, {278, 278, 0}, {0, 1, 0});
	Camera->SetDefocusAngle(0.0f);
}

static void InitialzeCornellSmoke(RayTracingCamera* Camera, RayTracingScene* Scene) {
	const Math::Color8 Red{ 0.65f, 0.05f, 0.05f, 1.0f };
	const Math::Color8 White{ 0.73f, 0.73f, 0.73f, 1.0f };
	const Math::Color8 Green{ 0.12f, 0.45f, 0.15f, 1.0f };
	const Math::Color8 LightColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	const float LightScale = 15.0f;

	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {555, 0, 0}, {0, 555, 0}, {0, 0, 555} }, SolidColorLambertian(Green))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {0, 0, 0}, {0, 555, 0}, {0, 0, 555} }, SolidColorLambertian(Red))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {343, 554, 332}, {-130, 0, 0}, {0, 0, -105} }, EmissiveMaterial(LightColor, LightScale))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {0, 0, 0}, {555, 0, 0}, {0, 0, 555} }, SolidColorLambertian(White))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {555, 555, 555}, {-555, 0, 0}, {0, 0, -555} }, SolidColorLambertian(White))));
	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{ {0, 0, 555}, {555, 0, 0}, {0, 555, 0} }, SolidColorLambertian(White))));
	Scene->SetBackground(TexturePtr(new SolidColor({ 0.0f, 0.0f, 0.0f, 1.0f })));

	//AddBox(Scene, {130, 0, 65}, {295, 165, 230}, White);
	//AddBox(Scene, {265, 0, 295}, {430, 330, 460}, White);
	//Scene->AddObject(RTObjectPtr(new RTBox({ 130, 0, 65 }, { 295, 165, 230 }, SolidColorLambertian(White))));
	//Scene->AddObject(RTObjectPtr(new RTBox({ 265, 0, 295 }, { 430, 330, 460 }, SolidColorLambertian(White))));
	RTObjectPtr Box0 = TransformedObject<RTBox>({ 265,0,295 }, 15 * Math::Deg2Rad, Math::FVector3{ 0,0,0 }, Math::FVector3{ 165,330,165 }, SolidColorLambertian(White));
	RTObjectPtr Box1 = TransformedObject<RTBox>({ 130,0,65 }, -30 * Math::Deg2Rad, Math::FVector3{ 0,0,0 }, Math::FVector3{ 165,165,165 }, SolidColorLambertian(White));
	Scene->AddObject(RTObjectPtr(new RTConstantMedium(MoveTemp(Box0), 0.01f, Math::Color8{0.0f, 0.0f, 0.0f, 1.0f})));
	Scene->AddObject(RTObjectPtr(new RTConstantMedium(MoveTemp(Box1), 0.01f, Math::Color8{1.0f, 0.0f, 1.0f, 1.0f})));

	Camera->SetFov(40.0f * Math::Deg2Rad);
	Camera->SetView({ 278, 278, -800 }, { 278, 278, 0 }, { 0, 1, 0 });
	Camera->SetDefocusAngle(0.0f);
}

static void InitializeFinalScene(RayTracingCamera* Camera, RayTracingScene* Scene) {
	Math::Color8 GroundColor{0.48f, 0.83f, 0.53f, 1.0f};
	constexpr int32 BoxPerSide = 20;
	for (int i = 0; i < BoxPerSide; i++) {
		for (int j = 0; j < BoxPerSide; j++) {
			float w = 100.0f;
			float x0 = -1000.0f + (float)i * w;
			float z0 = -1000.0f + (float)j * w;
			float y0 = 0.0;
			float x1 = x0 + w;
			float y1 = Math::Random(1.0f, 101.0f);
			float z1 = z0 + w;
			Scene->AddObject(RTObjectPtr(new RTBox({x0, y0, z0}, {x1, y1, z1}, SolidColorLambertian(GroundColor))));
		}
	}
	Math::Color8 LightColor{1.0f, 1.0f, 1.0f, 1.0f};
	float LightDensity = 7.0f;

	Scene->AddObject(RTObjectPtr(new RTQuad(Math::FQuad{{123, 554, 147}, {300, 0, 0}, {0, 0, 265}}, EmissiveMaterial(LightColor, LightDensity))));

	Math::FVector3 Center1(400, 400, 200);
	Math::FVector3 Center2 = Center1 + Math::FVector3{30, 0, 0};
	Scene->AddObject(RTObjectPtr(new RTMovableSphere({ Center1,50 }, SolidColorLambertian(Math::Color8{ 0.7f, 0.3f, 0.1f, 1.0f}), Center2)));

	Scene->AddObject(MakeGlassSphere({ 260.0f, 150.0f, 45.0f }, 50.0f, 1.5f));
	Scene->AddObject(RTObjectPtr(new RTSphere({{0, 150, 145}, 50}, SolidColorMetal(Math::FVector4{0.8f, 0.8f, 0.9f, 1.0f}, 1.0f))));

	Scene->AddObject(MakeGlassSphere({ 360.0f, 150.0f, 145.0f }, 70.0f, 1.5f));
	Scene->AddObject(RTObjectPtr(new RTConstantMedium(MakeGlassSphere({ 360.0f, 150.0f, 145.0f }, 70.0f, 1.5f), 0.2f, {0.2f, 0.4f, 0.9f, 1.0f})));
	Scene->AddObject(RTObjectPtr(new RTConstantMedium(MakeGlassSphere({ 0.0f, 0.0f, 0.0f }, 5000.0f, 1.5f), 0.0001f, { 1.0f, 1.0f, 1.0f, 1.0f })));

	Scene->AddObject(RTObjectPtr(new RTSphere({{400.0f, 200.0f, 400.0f}, 100.0f}, ImageLambertian("earthmap.jpg"))));
	Scene->AddObject(RTObjectPtr(new RTSphere({{360.0f, 150.0f,145.0f}, 70.0f}, NoiseLambertian(0.2f))));

	Math::Color8 WhiteColor{0.73f, 0.73f, 0.73f, 1.0f};
	int ns = 1000;
	for (int j = 0; j < ns; j++) {
		Scene->AddObject(TransformedObject<RTSphere>({-100.0f, 270.0f, 395.0f}, 15.0f, Math::FSphere{Math::RandomVector(0.0f, 165.0f), 10.0f}, SolidColorLambertian(WhiteColor)));
	}

	Scene->SetBackground(TexturePtr(new SolidColor({0.0f, 0.0f, 0.0f, 1.0f})));
	Camera->SetFov(40.0f * Math::Deg2Rad);
	Camera->SetView({ 478, 278, -600 }, { 278, 278, 0 }, {0, 1,0});
	Camera->SetDefocusAngle(0.0f);
}
#pragma endregion

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
	InitializeFinalScene(Camera.Get(), Scene.Get()); // TODO test
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
