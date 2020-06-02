// Copyright(c) 2019 - 2020, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "Utilities/Time/Time.h"
#include "Utilities/GenericStorage/GenericStorage.h"
#include "Core/Interfaces/IEvent.h"
#include "Core/DeveloperConsole/DeveloperConsole.h"
#include "Core/RenderController/RenderController.h"
#include "Core/MxObject/MxObject.h"
#include "Utilities/Counter/Counter.h"
#include "Utilities/FileSystem/File.h"
#include "Core/Scene/Scene.h"

#include "Platform/Window/Window.h"


namespace MxEngine
{
	class LoggerImpl;

	class Application
	{
		struct ModuleManager
		{
			ModuleManager(Application* app);
			~ModuleManager();
		} manager;
	private:
		static inline Application* Current = nullptr;

		ResourceStorage<Scene> scenes;
		UniqueRef<Window> window;
		RenderController renderer;
		AppEventDispatcher dispatcher;
		DeveloperConsole console;
		Counter resourceIdCounter;
		TimeStep timeDelta;
		Scene* currentScene = nullptr;
		int counterFPS;
		Vector4 debugColor = MakeVector4(1.0f, 0.0f, 0.0f, 1.0f);
		bool drawBoxes = false;
		bool drawSpheres = false;
		bool overlayDebug = false;
		bool shouldClose = false;
		bool isRunning = false;
		bool drawLighting = true;
		bool skyboxEnabled = true;

		void AddConsoleEventListener(DeveloperConsole& console);
		void DrawObjects();
		void InvokeUpdate();
		bool VerifyApplicationState();
		void VerifyRendererState();
		void VerifyLightSystem(LightSystem& lights);
	protected:

		Application();

		virtual void OnCreate();
		virtual void OnUpdate();
		virtual void OnDestroy();
	public:
		void ExecuteScript(Script& script);
		void ExecuteScript(const MxString& script);
		void ExecuteScript(const char* script);

		void ToggleDeveloperConsole(bool isVisible);
		void ToggleSkybox(bool state = true);
		void ToggleLighting(bool state = true);
		void ToggleDebugDraw(bool aabb, bool spheres, const Vector4& color,  bool overlay = false);

		AppEventDispatcher& GetEventDispatcher();
		RenderController& GetRenderer();
		LoggerImpl& GetLogger();
		DeveloperConsole& GetConsole();
		Window& GetWindow();
		Scene& GetCurrentScene();
		Scene& GetGlobalScene();
		void LoadScene(const MxString& name);
		Scene& CreateScene(const MxString& name, UniqueRef<Scene> scene);
		Scene& GetScene(const MxString& name);
		bool SceneExists(const MxString& name);
		void DestroyScene(const MxString& name);
		Counter::CounterType GenerateResourceId();
		float GetTimeDelta() const;
		int GetCurrentFPS() const;
		void SetMSAASampling(size_t samples);
		void Run();
		bool IsRunning() const;
		void CloseApplication();
		void CreateContext();
		virtual ~Application();

		static Application* Get();
		static void Set(Application* application);
	};
}