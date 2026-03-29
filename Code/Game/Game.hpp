#pragma once

#include "Game/GameCommon.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Light.hpp"
#include "Engine/Core/Rgba8.hpp"

//------------------------------------------------------------------------------------------------------------------
class Clock;
class GameCamera;
class World;
class ConstantBuffer;
class Entity;
class Player;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class GameState
{
	ATTRACT,
	PLAYING
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Game
{
	friend class App;

private:
					Game();
	virtual			~Game();

	void 			Startup();
	void 			Shutdown();
	void 			Update();
	void 			Render();
		 		
	bool 			IsAttractMode() const;
	void 			SetAttractMode(bool attractModeStatus);
	
	void			UpdateCameras();

	void 			RenderAttractMode() const;
	void 			RenderGame();
	void 			RenderDebugText() const;
	void			DebugImguiInfo();

	void			InitializeCameras();
	void			InitializeDirectionalLight();
	
	void 			PrintControlsOnDevConsole();
	     		
	void 			KeyboardControls();
	void			ShaderDebugInputs();
	void 			XboxControls();
		 		
	void 			PauseAndSlowdown();

	void			RegenerateWorld();

	std::string		GetDebugRenderText() const;

	Player*			SpawnPlayer();

public:
	static World*	GetWorld();
	static float	GetDeltaSeconds();
	static bool		IsDebugMode();
	static bool		ShouldDisplayJobInfo();
	static Vec3     GetPlayerPosition();
	static bool     ShowEntityDebugs();
	static bool		Event_AddGlowstone(EventArgs& args);

//	static bool		s_lockGenerationToOrigin;

private:
	Camera			m_screenCamera;
	
	bool			m_isAttractMode		= true;

	bool			m_displayImguiInfo	= false;								
	
	int				m_debugInt		= 0;
								
	Light			m_sunLight;
	AABB2			m_textBox		= AABB2();
	
	GameState		m_gameState		= GameState::ATTRACT;
	Player*			m_playerEntity  = nullptr;

	static GameCamera*	s_gameCamera;
	static World*		s_world;
	static Clock*		s_gameClock;
	static bool			s_isDebugMode;
	static bool			s_jobInfo;
	static bool			s_displayEntityDebug;								

};
