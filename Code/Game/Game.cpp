#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/GameCamera.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/Player.hpp"
#include "Game/Inventory.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Math/CurveUtils.hpp"

#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/implot.h"

//------------------------------------------------------------------------------------------------------------------
SpriteSheet*	g_blockSpriteSheet				= nullptr;
SpriteSheet*	g_itemSpriteSheet				= nullptr;
World*			Game::s_world					= nullptr;
GameCamera*		Game::s_gameCamera				= nullptr;
Clock*			Game::s_gameClock				= nullptr;
bool			Game::s_isDebugMode				= false;
//bool			Game::s_lockGenerationToOrigin	= false;
bool			Game::s_jobInfo					= false;
bool			Game::s_displayEntityDebug		= false;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Game::Game()
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Game::~Game()
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Startup()
{
	PrintControlsOnDevConsole();

	m_isAttractMode = true;

	s_gameClock = new Clock(Clock::GetSystemClock());
	BlockDefinition::InitBlockDefinitions();

	InitializeCameras();
	InitializeDirectionalLight();

	Texture* spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFilePath("Data/Images/SpriteSheet_Faithful_64x.png", 6);
	g_blockSpriteSheet = new SpriteSheet(*spriteSheetTexture, IntVec2(8, 8));

	Texture* itemSheetTexture = g_theRenderer->CreateOrGetTextureFromFilePath("Data/Images/ItemsSpriteSheet_32px.png");
	g_itemSpriteSheet = new SpriteSheet(*itemSheetTexture, IntVec2(8, 8));
	
	s_world = new World(s_gameClock);

	Mat44 transform = Mat44();

	s_gameCamera = new GameCamera();
	m_playerEntity = SpawnPlayer();
	s_gameCamera->FollowEntity(m_playerEntity);

//	DebugAddBasis(transform, -1.f, 2.f, DebugRenderMode::ALWAYS);
	SubscribeEventCallbackFunction("AddGlowstone", Event_AddGlowstone);
	g_eventSystem->FireEvent("AddGlowstone");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Shutdown()
{
	UnsubscribeEventCallbackFunction("AddGlowstone", Event_AddGlowstone);

	if(s_gameClock)
	{
		delete s_gameClock;
		s_gameClock = nullptr;
	}


	delete s_world;
	s_world = nullptr;

	delete s_gameCamera;
	s_gameCamera = nullptr;
	
	FireEvent("Clear");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Update()
{

	PauseAndSlowdown();

	KeyboardControls();
	XboxControls();

	if(!m_isAttractMode)
	{
		s_gameCamera->Update();
		s_world->Update();
	}

	Vec3 basisPosition = s_gameCamera->m_camera.GetPosition() + (s_gameCamera->GetForwardVector() * 50.f);
	Mat44 basisTransform = Mat44();
	basisTransform.SetTranslation3D(basisPosition);
	
	if(s_gameCamera->m_cameraMode == CAMERA_SPECTATOR || s_gameCamera->m_cameraMode == CAMERA_SPECTATOR_XY)
	{
		DebugAddBasis(basisTransform, 0.f, 1.f, DebugRenderMode::ALWAYS);
	}

 	//Vec3 cameraPosition = s_gameCamera->GetPosition();
 	//IntVec3 coord = GetGlobalCoords(cameraPosition);
 
 	//std::string positionString = Stringf("Camera Position: (%0.2f, %0.2f, %0.2f)", cameraPosition.x, cameraPosition.y, cameraPosition.z);
 	//std::string coordString = Stringf("Camera Coord: (%d, %d, %d)", coord.x, coord.y, coord.z);
 
 	//DebugAddScreenText(positionString, AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions()), 12.f, Vec2(0.f, .98f), 0.f);
 	//DebugAddScreenText(coordString, AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions()), 12.f, Vec2(0.f, 0.96f), 0.f);
// 
// 	std::string worldTime = Stringf("World Time: %0.2f", s_world->m_worldClock->GetTotalSeconds());
// 	std::string gameTime = Stringf("Game Time: %0.2f", s_gameClock->GetTotalSeconds());
// 
// 	DebugAddScreenText(gameTime, AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions()), 12.f, Vec2(0.f, .94f), 0.f, Rgba8::GREEN);
// 	DebugAddScreenText(worldTime, AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions()), 12.f, Vec2(0.f, 0.92f), 0.f, Rgba8::ORANGERED);

	IntVec3 playerPos = GetPlayerPosition().GetAsIntVec3();
	IntVec2 playerChunk = GetChunkCoords(playerPos);
	IntVec2 chunkCenter = GetChunkCenter(playerChunk);

	std::string chunkString = Stringf("Player Position: (%d, %d, %d)\nPlayer Chunk: (%d, %d)\nChunk Center: (%d, %d)", playerPos.x, playerPos.y, playerPos.z, 
										playerChunk.x, playerChunk.y, chunkCenter.x, chunkCenter.y);

	DebugAddScreenText(chunkString, AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions()), 12.f, Vec2(0.f, .94f), 0.f, Rgba8::GREEN);


	DebugImguiInfo();
	UpdateCameras();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::DebugImguiInfo()
{

	if(!m_displayImguiInfo)
	{
		return;
	}

	ImGui::Begin("Debug");

	if(ImGui::CollapsingHeader("Terrain Settings"))
	{
		ImGui::Checkbox("Lock world generation to origin", &g_lockTerrainGenerationToOrigin);

		ImGui::SeparatorText("Noise Layers");
		ImGui::Checkbox("Density", &g_computeDensity);
		ImGui::Checkbox("Continentalness", &g_computeContinentalness);
		ImGui::Checkbox("Erosion", &g_computeErosion);
		ImGui::Checkbox("Peaks & Valleys", &g_computePeaksAndValley);
	}

	if(ImGui::CollapsingHeader("Chunk Systems"))
	{
		ImGui::Checkbox("Enable cross-chunk hidden surface removal",
						&g_enableCrossChunkHiddenSurfaceRemoval);

		ImGui::Checkbox("Enable surface block replacement",
						&g_enableSurfaceBlockReplacement);
	}

	if(ImGui::CollapsingHeader("Gameplay / Interaction"))
	{
		ImGui::Checkbox("Enable digging & placing blocks",
						&g_enableDiggingAndPlacing);
	}

	if(ImGui::CollapsingHeader("Rendering"))
	{
		ImGui::Checkbox("Enable lighting", &g_enableLighting);
	}
	if(ImGui::CollapsingHeader("Density Noise"))
	{
		ImGui::SliderFloat("Density Noise Scale", &g_densityNoiseScale, 0.f, DENSITY_NOISE_SCALE * DENSITY_NOISE_SCALE);
		ImGui::SliderFloat("Density Noise Octaves", &g_densityNoiseOctaves, 0.f, 128.f);
		ImGui::SliderFloat("Density Bias", &g_densityBiasPerBlockDelta, 0.f, 1.f);
		ImGui::SliderFloat("Density Terrain Height", &g_densityTerrainHeight, 0.f, CHUNK_SIZE_Z);
	}
	if(ImGui::CollapsingHeader("Continentalness Noise"))
	{
		ImGui::InputFloat("Continentalness Noise Scale", &g_continentalnessNoiseScale);
		ImGui::InputFloat("Continentalness Noise Octaves", &g_continentalnessNoiseOctaves);

		ImGui::Columns(2);

		ImGui::Text("Continentalness Curve Points");
		
		float deepOcean[2] = {g_deepOceanCurve.m_start, g_deepOceanCurve.m_end};
		if(ImGui::InputFloat2("Deep Ocean", deepOcean))
		{
			g_deepOceanCurve.m_start = deepOcean[0];
			g_deepOceanCurve.m_end = deepOcean[1];
		}

		float ocean[2] = {g_oceanCurve.m_start, g_oceanCurve.m_end};
		if(ImGui::InputFloat2("Ocean", ocean))
		{
			g_oceanCurve.m_start = ocean[0];
			g_oceanCurve.m_end = ocean[1];
		}

		float coast[2] = {g_coastCurve.m_start, g_coastCurve.m_end};
		if(ImGui::InputFloat2("Coast", coast))
		{
			g_coastCurve.m_start = coast[0];
			g_coastCurve.m_end = coast[1];
		}

		float nearInland[2] = {g_nearInlandCurve.m_start, g_nearInlandCurve.m_end};
		if(ImGui::InputFloat2("Near Inland", nearInland))
		{
			g_nearInlandCurve.m_start = nearInland[0];
			g_nearInlandCurve.m_end = nearInland[1];
		}

		float midInland[2] = {g_midInlandCurve.m_start, g_midInlandCurve.m_end};
		if(ImGui::InputFloat2("Mid Inland", midInland))
		{
			g_midInlandCurve.m_start = midInland[0];
			g_midInlandCurve.m_end = midInland[1];
		}

		float farInland[2] = {g_farInlandCurve.m_start, g_farInlandCurve.m_end};
		if(ImGui::InputFloat2("Far Inland", farInland))
		{
			g_farInlandCurve.m_start = farInland[0];
			g_farInlandCurve.m_end = farInland[1];
		}

		float maxContinental[2] = {g_maxContinentalnessCurve.m_start, g_maxContinentalnessCurve.m_end};
		if(ImGui::InputFloat2("Max Continental", maxContinental))
		{
			g_maxContinentalnessCurve.m_start = maxContinental[0];
			g_maxContinentalnessCurve.m_end = maxContinental[1];
		}

		ImGui::NextColumn();
		float xPositions[] = {
			g_deepOceanCurve.m_start,
			g_oceanCurve.m_start,
			g_coastCurve.m_start,
			g_nearInlandCurve.m_start,
			g_midInlandCurve.m_start,
			g_farInlandCurve.m_start,
			g_maxContinentalnessCurve.m_start
		};

		float yValues[] = {
			g_deepOceanCurve.m_end,
			g_oceanCurve.m_end,
			g_coastCurve.m_end,
			g_nearInlandCurve.m_end,
			g_midInlandCurve.m_end,
			g_farInlandCurve.m_end,
			g_maxContinentalnessCurve.m_end
		};

		if(ImPlot::BeginPlot("Continentalness Curve", ImVec2(-1, 400)))
		{
			ImPlot::SetupAxesLimits(-1.0, 1.0, -1.0, 1.0, ImGuiCond_Always);
			//ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4.0f, ImVec4(1, 1, 0, 1));
			ImPlot::PlotLine("Continent Curve", xPositions, yValues, 7);
			

			for(int i = 0; i < 7; ++i)
			{
				double x = xPositions[i];
				double y = yValues[i];

				if(ImPlot::DragPoint(i, &x, &y, ImVec4(1, 1, 0, 1), 4.0f))
				{
					xPositions[i] = static_cast<float>(x);
					yValues[i] = static_cast<float>(y);

					// Update globals immediately
					switch(i)
					{
						case 0: 
						{
							g_deepOceanCurve.m_start = xPositions[0]; 
							g_deepOceanCurve.m_end = yValues[0]; 
							break;
						}
						
						case 1: 
						{
							g_oceanCurve.m_start = xPositions[1]; 
							g_oceanCurve.m_end = yValues[1]; 
							break;
						}

						case 2:
						{
							g_coastCurve.m_start = xPositions[2]; 
							g_coastCurve.m_end = yValues[2]; 
							break;
						}

						case 3:
						{
							g_nearInlandCurve.m_start = xPositions[3]; 
							g_nearInlandCurve.m_end = yValues[3]; 
							break;
						}
						case 4: 
						{
							g_midInlandCurve.m_start = xPositions[4]; 
							g_midInlandCurve.m_end = yValues[4]; 
							break;
						}
						case 5: 
						{
							g_farInlandCurve.m_start = xPositions[5]; 
							g_farInlandCurve.m_end = yValues[5]; 
							break;
						}
						case 6: 
						{
							g_maxContinentalnessCurve.m_start = xPositions[6]; 
							g_maxContinentalnessCurve.m_end = yValues[6]; 
							break;
						}
					}
				}
			}
			ImPlot::EndPlot();
		
		}

		ImGui::Columns(1);
	}


	if(ImGui::CollapsingHeader("Erosion Noise"))
	{
		ImGui::InputFloat("Erosion Noise Scale", &g_erosionNoiseScale);
		ImGui::InputFloat("Erosion Noise Octaves", &g_erosionNoiseOctaves);
		ImGui::InputFloat("Erosion Terrain Height", &g_erosionDefaultTerrainHeight);

		ImGui::Columns(2);

		ImGui::Text("Erosion Curve Points");

		float e0[2] = {g_erosionL0.m_start, g_erosionL0.m_end};
		if(ImGui::InputFloat2("Erosion L0", e0))
		{
			g_erosionL0.m_start = e0[0];
			g_erosionL0.m_end = e0[1];
		}

		float e1[2] = {g_erosionL1.m_start, g_erosionL1.m_end};
		if(ImGui::InputFloat2("Erosion L1", e1))
		{
			g_erosionL1.m_start = e1[0];
			g_erosionL1.m_end = e1[1];
		}

		float e2[2] = {g_erosionL2.m_start, g_erosionL2.m_end};
		if(ImGui::InputFloat2("Erosion L2", e2))
		{
			g_erosionL2.m_start = e2[0];
			g_erosionL2.m_end = e2[1];
		}

		float e3[2] = {g_erosionL3.m_start, g_erosionL3.m_end};
		if(ImGui::InputFloat2("Erosion L3", e3))
		{
			g_erosionL3.m_start = e3[0];
			g_erosionL3.m_end = e3[1];
		}

		float e4[2] = {g_erosionL4.m_start, g_erosionL4.m_end};
		if(ImGui::InputFloat2("Erosion L4", e4))
		{
			g_erosionL4.m_start = e4[0];
			g_erosionL4.m_end = e4[1];
		}

		float e5[2] = {g_erosionL5.m_start, g_erosionL5.m_end};
		if(ImGui::InputFloat2("Erosion L5", e5))
		{
			g_erosionL5.m_start = e5[0];
			g_erosionL5.m_end = e5[1];
		}

		float e6[2] = {g_erosionL6.m_start, g_erosionL6.m_end};
		if(ImGui::InputFloat2("Erosion L6", e6))
		{
			g_erosionL6.m_start = e6[0];
			g_erosionL6.m_end = e6[1];
		}

		ImGui::NextColumn();

		float ex[] = {
			g_erosionL0.m_start,
			g_erosionL1.m_start,
			g_erosionL2.m_start,
			g_erosionL3.m_start,
			g_erosionL4.m_start,
			g_erosionL5.m_start,
			g_erosionL6.m_start
		};

		float ey[] = {
			g_erosionL0.m_end,
			g_erosionL1.m_end,
			g_erosionL2.m_end,
			g_erosionL3.m_end,
			g_erosionL4.m_end,
			g_erosionL5.m_end,
			g_erosionL6.m_end
		};

		if(ImPlot::BeginPlot("Erosion Curve", ImVec2(-1, 400)))
		{
			ImPlot::SetupAxesLimits(-1.0, 1.0, -1.0, 1.0, ImGuiCond_Always);
			ImPlot::PlotLine("Erosion", ex, ey, 7);

			for(int i = 0; i < 7; ++i)
			{
				double x = ex[i];
				double y = ey[i];

				if(ImPlot::DragPoint(i, &x, &y, ImVec4(1, 0, 0, 1), 4.0f))
				{
					ex[i] = (float)x;
					ey[i] = (float)y;

					switch(i)
					{
					case 0: g_erosionL0.m_start = ex[0]; g_erosionL0.m_end = ey[0]; break;
					case 1: g_erosionL1.m_start = ex[1]; g_erosionL1.m_end = ey[1]; break;
					case 2: g_erosionL2.m_start = ex[2]; g_erosionL2.m_end = ey[2]; break;
					case 3: g_erosionL3.m_start = ex[3]; g_erosionL3.m_end = ey[3]; break;
					case 4: g_erosionL4.m_start = ex[4]; g_erosionL4.m_end = ey[4]; break;
					case 5: g_erosionL5.m_start = ex[5]; g_erosionL5.m_end = ey[5]; break;
					case 6: g_erosionL6.m_start = ex[6]; g_erosionL6.m_end = ey[6]; break;
					}
				}
			}

			ImPlot::EndPlot();
		}

		ImGui::Columns(1);
	}

	if(ImGui::CollapsingHeader("Weirdness Noise"))
	{
		ImGui::InputFloat("Weirdness Noise Scale", &g_weirdnessNoiseScale);
		ImGui::InputFloat("Weirdness Noise Octaves", &g_weirdnessNoiseOctaves);

		ImGui::Columns(2);

		ImGui::Text("Weirdness Curve Points");

		float w0[2] = {g_weirdnessValleys.m_start, g_weirdnessValleys.m_end};
		if(ImGui::InputFloat2("Weirdness Valleys", w0))
		{
			g_weirdnessValleys.m_start = w0[0];
			g_weirdnessValleys.m_end = w0[1];
		}

		float w1[2] = {g_weirdnessLow.m_start, g_weirdnessLow.m_end};
		if(ImGui::InputFloat2("Weirdness Low", w1))
		{
			g_weirdnessLow.m_start = w1[0];
			g_weirdnessLow.m_end = w1[1];
		}

		float w2[2] = {g_weirdnessMid.m_start, g_weirdnessMid.m_end};
		if(ImGui::InputFloat2("Weirdness Mid", w2))
		{
			g_weirdnessMid.m_start = w2[0];
			g_weirdnessMid.m_end = w2[1];
		}

		float w3[2] = {g_weirdnessHigh.m_start, g_weirdnessHigh.m_end};
		if(ImGui::InputFloat2("Weirdness High", w3))
		{
			g_weirdnessHigh.m_start = w3[0];
			g_weirdnessHigh.m_end = w3[1];
		}

		float w4[2] = {g_weirdnessPeaks.m_start, g_weirdnessPeaks.m_end};
		if(ImGui::InputFloat2("Weirdness Peaks", w4))
		{
			g_weirdnessPeaks.m_start = w4[0];
			g_weirdnessPeaks.m_end = w4[1];
		}

		float w5[2] = {g_weirdnessMax.m_start, g_weirdnessMax.m_end};
		if(ImGui::InputFloat2("Weirdness Max", w5))
		{
			g_weirdnessMax.m_start = w5[0];
			g_weirdnessMax.m_end = w5[1];
		}

		ImGui::NextColumn();

		float wx[] = {
			g_weirdnessValleys.m_start,
			g_weirdnessLow.m_start,
			g_weirdnessMid.m_start,
			g_weirdnessHigh.m_start,
			g_weirdnessPeaks.m_start,
			g_weirdnessMax.m_start
		};

		float wy[] = {
			g_weirdnessValleys.m_end,
			g_weirdnessLow.m_end,
			g_weirdnessMid.m_end,
			g_weirdnessHigh.m_end,
			g_weirdnessPeaks.m_end,
			g_weirdnessMax.m_end
		};

		if(ImPlot::BeginPlot("Weirdness Curve", ImVec2(-1, 400)))
		{
			ImPlot::SetupAxesLimits(-1.0, 1.0, -1.0, 1.0, ImGuiCond_Always);
			ImPlot::PlotLine("Weirdness", wx, wy, 6);

			for(int i = 0; i < 6; ++i)
			{
				double x = wx[i];
				double y = wy[i];

				if(ImPlot::DragPoint(i, &x, &y, ImVec4(0, 0.5, 1, 1), 4.0f))
				{
					wx[i] = (float)x;
					wy[i] = (float)y;

					switch(i)
					{
					case 0: g_weirdnessValleys.m_start = wx[0]; g_weirdnessValleys.m_end = wy[0]; break;
					case 1: g_weirdnessLow.m_start = wx[1]; g_weirdnessLow.m_end = wy[1]; break;
					case 2: g_weirdnessMid.m_start = wx[2]; g_weirdnessMid.m_end = wy[2]; break;
					case 3: g_weirdnessHigh.m_start = wx[3]; g_weirdnessHigh.m_end = wy[3]; break;
					case 4: g_weirdnessPeaks.m_start = wx[4]; g_weirdnessPeaks.m_end = wy[4]; break;
					case 5: g_weirdnessMax.m_start = wx[5]; g_weirdnessMax.m_end = wy[5]; break;
					}
				}
			}

			ImPlot::EndPlot();
		}

		ImGui::Columns(1);
	}


	ImGui::End();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_theRenderer->BeginCamera(s_gameCamera->m_camera);

	if(!m_isAttractMode)
	{
		RenderGame();
	}

	DebugRenderWorld(s_gameCamera->m_camera);

	g_theRenderer->EndCamera(s_gameCamera->m_camera);
	
	g_theRenderer->BeginCamera(m_screenCamera);

	if(m_isAttractMode)
	{
		g_theRenderer->ClearScreen(Rgba8(150, 150, 150, 255));
		RenderAttractMode();
	}
	
	if(!m_isAttractMode)
	{
		if(m_playerEntity)
		{
			m_playerEntity->RenderInventory();
		}
		RenderDebugText();
		DebugRenderScreen(m_screenCamera);
	}

	g_theRenderer->EndCamera(m_screenCamera);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateCameras()
{
	m_screenCamera.m_mode = Camera::eMode_Orthographic;

	Vec2 orthoMax = Vec2(g_theWindow->GetClientDimensions());

	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), orthoMax);

	s_gameCamera->m_camera.m_mode = Camera::eMode_Perspective;

	Mat44 cameraToRenderMatrix;
	
	cameraToRenderMatrix.SetIJKT3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 0.f));

	s_gameCamera->m_camera.SetCameraToRenderTransform(cameraToRenderMatrix);
	s_gameCamera->m_camera.SetPerspectiveView(g_theWindow->GetConfig().m_aspectRatio, 60.f, 0.01f, 10000.f);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderAttractMode() const
{
	std::vector<Vertex_PCU> attractVerts;
	
	std::string gameTitle = "Simple Miner";
	std::string gameSubtitle = "GOLD";
	std::string startInst = "Press SPACE to START";

	g_gameFont->AddVertsForTextInBox2D(attractVerts, gameTitle, m_textBox, 80.f, Rgba8::WHITE, 0.75f, Vec2(0.5f, 0.8f));
	g_gameFont->AddVertsForTextInBox2D(attractVerts, gameSubtitle, m_textBox, 32.f, Rgba8::WHITE, 0.75f, Vec2(0.5f, 0.7f));
	g_gameFont->AddVertsForTextInBox2D(attractVerts, startInst, m_textBox, 24.f, Rgba8::WHITE, 0.75f, Vec2(0.5f, 0.1f));

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(attractVerts);
}

//------------------------------------------------------------------------------------------------------------------
void Game::RenderGame()
{
// 	g_theRenderer->SetDebugConstants(s_gameClock->GetDeltaSeconds(), m_debugInt);
// 
// 	std::vector<Light> allLight;
// 
// 	g_theRenderer->SetLightConstants(m_sunLight, allLight, s_player->m_playerCamera.m_position, 0.4f);
	
	s_world->Render();
	s_gameCamera->Render();
}

//------------------------------------------------------------------------------------------------------------------
bool Game::IsAttractMode() const
{
	return m_isAttractMode;
}

//------------------------------------------------------------------------------------------------------------------
void Game::SetAttractMode(bool attractModeStatus)
{
	m_isAttractMode = attractModeStatus;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Game::GetDebugRenderText() const
{
	switch(m_debugInt)
	{
		case 0:		return Stringf("Debug Render Mode: Lit (0)");
		case 1:		return Stringf("Debug Render Mode: Texture Color (1)");
		case 2:		return Stringf("Debug Render Mode: Vertex Color (2)");
		case 3:		return Stringf("Debug Render Mode: Model Tint (3)");
		case 4:		return Stringf("Debug Render Mode: Vertex Normals[Raw](4)");
		case 5:		return Stringf("Debug Render Mode: Vertex Tangents[Raw](5)");
		case 6:		return Stringf("Debug Render Mode: Vertex Bitangents[Raw](6)");
		case 7:		return Stringf("Debug Render Mode: Vertex Normals[World Transformed](7)");
		case 8:		return Stringf("Debug Render Mode: Vertex Tangents[World Transformed](8)");
		case 9:		return Stringf("Debug Render Mode: Vertex Bitangents[World Transformed](9)");
		case 10:	return Stringf("Debug Render Mode: Texture Coordinates (UV) (10)");
		case 11:	return Stringf("Debug Render Mode: Normal Texel (11)");
		case 12:	return Stringf("Debug Render Mode: SGE Texel (12)");
		case 13:	return Stringf("Debug Render Mode: Specular Highlight Only (13)");
		case 14:	return Stringf("Debug Render Mode: Diffuse Lighting Only (14)");
		default:    return Stringf("Debug Render Mode: Does not exist (%d)", m_debugInt);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Player* Game::SpawnPlayer()
{
	Vec3		position	= g_gameConfigBlackboard.GetValue("playerSpawnPosition", Vec3(0.f));
	EulerAngles orientation = g_gameConfigBlackboard.GetValue("playerSpawnOrientation", EulerAngles(0.f, 0.f, 0.f));

	return dynamic_cast<Player*>(s_world->SpawnEntityOfType(EntityType::ENTITIY_TYPE_PLAYER, position, orientation));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderDebugText() const
{
	float fps = 1.f / s_gameClock->GetDeltaSeconds();
	std::string fpsText = Stringf("FPS: %0.2f", fps);
	DebugAddScreenText(fpsText, m_textBox, 14.f, Vec2(0.99f, 0.99f), 0.f, Rgba8(255, 255, 255, 255));	
}

//------------------------------------------------------------------------------------------------------------------
void Game::InitializeCameras()
{
	m_screenCamera.m_viewportBounds.m_mins = Vec2::ZERO;
	m_screenCamera.m_viewportBounds.m_maxs = Vec2(static_cast<float>(g_theWindow->GetClientDimensions().x), static_cast<float>(g_theWindow->GetClientDimensions().y));

	m_textBox = m_screenCamera.m_viewportBounds;
}

//------------------------------------------------------------------------------------------------------------------
void Game::InitializeDirectionalLight()
{
	Vec3 refPosition = Vec3(3.f, 10.f, 2.f);
	Vec3 lightToSurfaceDirection = (-refPosition).GetNormalized();

	m_sunLight = Light::CreateDirectionalLight(lightToSurfaceDirection, 0.8f, Rgba8::WHITE);
	m_sunLight.SetPosition(refPosition);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::PrintControlsOnDevConsole()
{
	g_devConsole->AddLine(DevConsole::INTRO_TEXT, "Simple Miner");
	g_devConsole->AddLine(DevConsole::INTRO_TEXT, "==============================================");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Game Controls");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Move - W/S/A/D");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Move Up and Down - E/Q");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Increase Move Speed - Left Shift");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Toggle ImGUI Menu - F1");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Toggle Debug Draw - F2");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Toggle Job System Info - F3");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Toggle Entity Debug Render - F4");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Reset Game - F8");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Dig - LMB");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Place Selected Block - RMB");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Lock/Unlock Raycast - R");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Open/Close Inventory Menu - Tab");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Access items on Hotbar - 1 - 10 / Mouse Wheel");
	g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Move items in inventory - Left Click (Select or place entire stack) / Right Click (Select half the stack, place 1 item)");
	g_devConsole->AddLine(DevConsole::INTRO_TEXT, "----------------------------------------------------------");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::KeyboardControls()
{

	if(m_isAttractMode)
	{
		if(g_inputSystem->WasKeyJustPressed(' '))
		{
			m_isAttractMode = false;
		}


		if(g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theApp->RequestQuit();
		}
	}
	
	if(!m_isAttractMode)
	{
		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F2))
		{
			s_isDebugMode = !s_isDebugMode;
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F3))
		{
			s_jobInfo = !s_jobInfo;
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F4))
		{
			s_displayEntityDebug = !s_displayEntityDebug;
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F1))
		{
			m_displayImguiInfo = !m_displayImguiInfo;
		}
	
		if(g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
		{
			m_isAttractMode = true;
		}

		ShaderDebugInputs();

	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::ShaderDebugInputs()
{
	if(g_inputSystem->WasKeyJustPressed('1'))
	{
		m_debugInt = 1;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 11;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('2'))
	{
		m_debugInt = 2;

		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 12;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('3'))
	{
		m_debugInt = 3;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 13;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('4'))
	{
		m_debugInt = 4;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 14;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('5'))
	{
		m_debugInt = 5;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 15;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('6'))
	{
		m_debugInt = 6;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 16;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('7'))
	{
		m_debugInt = 7;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 17;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('8'))
	{
		m_debugInt = 8;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 18;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('9'))
	{
		m_debugInt = 9;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 19;
		}
	}
	else if(g_inputSystem->WasKeyJustPressed('0'))
	{
		m_debugInt = 0;
		if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
		{
			m_debugInt = 10;
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::XboxControls()
{
	XboxController xboxController = g_inputSystem->GetController(0);

	if(m_isAttractMode)
	{
		if(xboxController.WasButtonJustPressed(XBOX_BUTTON_START))
		{
			SoundID toGame = g_theAudioSystem->CreateOrGetSound("Data/Audio/ToGame.mp3");
			g_theAudioSystem->StartSound(toGame);

			m_isAttractMode = false;
		}

		// if(xboxController.WasButtonJustPressed(XBOX_BUTTON_B))
		// {
		// 	g_theApp->RequestQuit();
		// }
	}

	if(!m_isAttractMode)
	{
		if(xboxController.WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			s_isDebugMode = !s_isDebugMode;
		}

		if(xboxController.WasButtonJustPressed(XBOX_BUTTON_B))
		{
			SoundID toAttract = g_theAudioSystem->CreateOrGetSound("Data/Audio/ToAttract.mp3");
			g_theAudioSystem->StartSound(toAttract);

			m_isAttractMode = true;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------
void Game::PauseAndSlowdown()
{

	if(g_inputSystem->WasKeyJustPressed('T'))
	{
		if(s_gameClock->GetTimeScale() < 1.)
		{
			s_gameClock->SetTimeScale(1.);
		}
		else
		{
			s_gameClock->SetTimeScale(0.1);
		}
	}

	if(g_inputSystem->WasKeyJustPressed('P'))
	{
		s_gameClock->TogglePause();
	}

	if(g_inputSystem->WasKeyJustPressed('O'))
	{		
		s_gameClock->StepSingleFrame();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RegenerateWorld()
{
//	s_player->m_playerCamera.m_position = Vec3(-45.f, -45.f, 280.f);
	delete s_world;
	s_world = new World(s_gameClock);
	
	m_playerEntity = nullptr;
	m_playerEntity = SpawnPlayer();
	s_gameCamera->FollowEntity(m_playerEntity);

}


// Static Helpers
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
World* Game::GetWorld()
{
	return s_world;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Game::GetDeltaSeconds()
{
	return s_gameClock->GetDeltaSeconds();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::IsDebugMode()
{
	return s_isDebugMode;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::ShouldDisplayJobInfo()
{
	return s_jobInfo;
}

//------------------------------------------------------------------------------------------------------------------
Vec3 Game::GetPlayerPosition()
{
	if(g_lockTerrainGenerationToOrigin)
	{
		return Vec3::ZERO;
	}

	return s_gameCamera->GetPosition();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::ShowEntityDebugs()
{
	return s_displayEntityDebug;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::Event_AddGlowstone(EventArgs& args)
{
	int itemCount = args.GetValue("count", 64);

	unsigned char glowstoneType = BlockDefinition::GetBlockDefinitionIndex("Glowstone");
	g_theApp->m_game->m_playerEntity->m_inventory->AddItemToInventory(glowstoneType, itemCount);
	
	return false;
}
