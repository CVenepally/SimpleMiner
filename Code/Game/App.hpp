#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"

//------------------------------------------------------------------------------------------------------------------
class NamedStrings;
typedef NamedStrings EventArgs;
class Game;
//------------------------------------------------------------------------------------------------------------------
class App
{

public:

	App();
	~App();
	
	void Startup();
	void Shutdown();
	void RunFrame();

	void SetCursorVisibility();
	void RunMainFrame();
	void RequestQuit();
	void ResetGame();
	
	void LoadConfigFile(char const* configFilePath);
	bool isQuitting() const;

	static bool RequestQuitEvent(EventArgs& args);
private:

	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

public:
	bool	m_isQuitting	= false;
	Game*	m_game			= nullptr;
};