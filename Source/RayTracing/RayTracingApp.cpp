#include "RayTracingApp.h"
#include "D3D12/D3D12App.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "D3D12/Texture.h"
#include "D3D12/Pipeline.h"
#include "Math/Vector.h"
#include "Math/MathUtil.h"
#include "RayTracing/RayTracingScene.h"
#include "RayTracing/RayTracingCamera.h"

namespace {
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
}

constexpr uint32 WINDOW_WIDTH = 800;
constexpr uint32 WINDOW_HEIGHT = 450;

inline void InitializeScene(RayTracingScene* scene) {
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

inline void InitializeScene2(RayTracingScene* scene) {
	auto materialGround = MaterialPtr(new LambertMaterial({ 0.5f, 0.5f, 0.5f, 1.0f }));
	scene->AddSphere({ {0.0f, -1000.0f, 0.0f}, 1000.0f }, MoveTemp(materialGround));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float chooseMaterial = Math::Random01();
			Math::FVector3 center((float)a + 0.9f * Math::Random01(), 0.2f, (float)b + 0.9f * Math::Random01());

			if ((center - Math::FVector3(4.0f, 0.2f, 0.0f)).Length() > 0.9f) {
				if (chooseMaterial < 0.8f) {
					// diffuse
					auto albedo = Math::Random01Vector() * Math::Random01Vector();
					auto mat = MaterialPtr(new LambertMaterial(albedo));
					scene->AddSphere({ center, 0.2f }, MoveTemp(mat));
				}
				else if (chooseMaterial < 0.95f) {
					// metal
					auto albedo = Math::RandomVector(0.5f, 1.0f);
					auto fuzz = Math::Random(0.0f, 0.5f);
					auto mat = MaterialPtr(new MetalMaterial(albedo, fuzz));
					scene->AddSphere({ center, 0.2f }, MoveTemp(mat));
				}
				else {
					// glass
					auto mat = MaterialPtr(new DielectricMaterial(1.5f));
					scene->AddSphere({ center, 0.2f }, MoveTemp(mat));
				}
			}
		}
	}
	scene->AddSphere({ {0.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new DielectricMaterial(1.5f)));
	scene->AddSphere({ {-4.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new LambertMaterial({ 0.4f, 0.2f, 0.1f })));
	scene->AddSphere({ {4.0f, 1.0f, 0.0f}, 1.0f }, MaterialPtr(new MetalMaterial({ 0.7f, 0.6f, 0.5f }, 0.0f)));
}


RayTracingApp::RayTracingApp(HINSTANCE hInstance) {
	// ========== step1. Initialize context ==========
	// Initialize window
	const Math::USize windowSize{ WINDOW_WIDTH, WINDOW_HEIGHT };
	D3D12Window::Initialize(hInstance, windowSize);
	if(!D3D12Window::Instance()->InitMainWindow()) {
		LOG_ERROR("Failed to initialize main window!");
		return;
	}
	// Initialize dx
	D3D12App::Initialize();

	// ========== step2. Scene rendering ==========
	// Initialize camera
	const Math::USize renderSize = windowSize;
	m_Camera.reset(new RayTracingCamera(renderSize));
	m_Camera->SetView({ 13,2,3 }, { 0,0,0 }, { 0,1,0 });
	m_Camera->SetFov(20 * Math::Deg2Rad);
	m_Camera->SetFocus(10.0f, 0.6f * Math::Deg2Rad);
	// Create ray tracing scene
	m_Scene.reset(new RayTracingScene());
	InitializeScene2(m_Scene.get());
	// Render the scene
	{
		ProfilePrintScope s{ "Scene Rendering" };
		m_Camera->Render(m_Scene.get());
	}

	// ========== step3. Display the render texture ==========
	RenderData renderData = m_Camera->GetRenderData();
	// Create PSO
	m_PSO.reset(new PSOCommon());
	// Create texture and upload data
	m_Texture.reset(new Texture2D(renderData.Width, renderData.Height, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM));
	D3D12App::Instance()->ImmediatelyCommit([this, renderData](ID3D12GraphicsCommandList* cmd) {
		m_Texture->UpdateData(cmd, renderData.Data, renderData.Width * renderData.Height * sizeof(Math::Color8));
	});
}

RayTracingApp::~RayTracingApp() {
	D3D12App::Release();
	D3D12Window::Release();
}

void RayTracingApp::Run() {
	while(D3D12Window::Instance()->Tick()) {
		RenderTexture();
	}
}

void RayTracingApp::RenderTexture() {
	D3D12App::Instance()->ExecuteDrawCall([this](ID3D12GraphicsCommandList* cmd) {
		m_PSO->Bind(cmd);
		m_Texture->BindDesc(cmd, 0);
		cmd->DrawInstanced(6, 1, 0, 0);
	});
}
