#include "Application.h"
#include "EditorConsole.h"

// Modules
#include "Module.h"
#include "ModuleWindow.h"
#include "ModuleInput.h"
#include "ModuleSceneIntro.h"
#include "ModuleRenderer3D.h"
#include "ModuleCamera3D.h"
#include "ModuleEditor.h"
#include "ModulePhysics.h"
#include "ModuleResources.h"
#include "GeometryLoader.h"
#include "TextureLoader.h"
#include "ModuleFileSystem.h"
#include "ModuleScripting.h"
#include "ModuleThreading.h"

// ----------------	//CHANGE/FIX: This shouldn't be here! Upgrade Save&Load so it's done on every module for their data.
// Windows
#include "EditorConfiguration.h"
#include "EditorConsole.h"
#include "EditorGame.h"
#include "EditorHierarchy.h"
#include "EditorInspector.h"
#include "EditorProject.h"
#include "EditorScene.h"
#include "EditorToolbar.h"

// Elements
#include "EditorMenuBar.h"

// Brofiler
#include "libs/Brofiler/Brofiler.h"

// Memory Leak Detection
#include "MemLeaks.h"

Application::Application()
{
	threading = new ModuleThreading(this, "Threading");
	window = new ModuleWindow(this, "Window");
	input = new ModuleInput(this, "Input");
	geometry_loader = new GeometryLoader(this, "Geometry_Loader");
	texture_loader = new TextureLoader(this, "Texture_Loader");
	file_system = new ModuleFileSystem(this, "File_System",ASSETS_FOLDER);
	scene_intro = new ModuleSceneIntro(this, "Scene");
	renderer3D = new ModuleRenderer3D(this, "Renderer");
	camera = new ModuleCamera3D(this, "Camera");
	editor = new ModuleEditor(this, "Editor");
	physics = new ModulePhysics(this, "Physics");
	resources = new ModuleResources(this,"Resources");
	scripting = new ModuleScripting(this,"Scripting");

	// The order of calls is very important!
	// Modules will Init() Start() and Update in this order
	// They will CleanUp() in reverse order

	// Main Modules
	AddModule(threading);
	AddModule(window);
	AddModule(input);
	AddModule(camera);
	AddModule(file_system);
	AddModule(resources);
	AddModule(physics);
	AddModule(geometry_loader);
	AddModule(texture_loader);

	//"Scene" (Currently the axis + white grid plane)
	AddModule(scene_intro);
	
	//Engine UI
	AddModule(editor);

	//Scripting module at the end just before rendering
	AddModule(scripting);
	// Renderer last!
	AddModule(renderer3D);
}

Application::~Application()
{
	std::list<Module*>::reverse_iterator item = list_modules.rbegin();

	while (item != list_modules.rend())
	{
		RELEASE(*item);
		item++;
	}
}

bool Application::Init()
{
	bool ret = true;

	//Load Settings before initializing modules
	editableConfig.assign(CONFIG_FILENAME);
	defaultConfig.assign(DEFAULT_CONFIG_FILENAME);
	LoadUserConfig();	//CHANGE/FIX: If config is deleted code breaks, add failsave method that creates new config with default values inside settings folder

	//Call Init() in all modules
	std::list<Module*>::iterator item = list_modules.begin();

	while (item != list_modules.end())
	{
		(*item)->Init();
		item++;
	}

	//Collect hardware info
	hardware.CPU_logic_cores = SDL_GetCPUCount();
	hardware.RAM = (float)(SDL_GetSystemRAM() * 1024);	// MB -> KB
	SDL_GetVersion(&hardware.sdl_version);

	//GPU
	hardware.GPU.vendor = (unsigned char*)glGetString(GL_VENDOR);
	hardware.GPU.renderer = (unsigned char*)glGetString(GL_RENDERER);
	hardware.GPU.version = (unsigned char*)glGetString(GL_VERSION);

	// Get all CPU features
	if (SDL_Has3DNow())
		CPU_features.push_back("3DNow");

	if (SDL_HasAVX())
		CPU_features.push_back("AVX");

	if (SDL_HasAVX2())
		CPU_features.push_back("AVX2");

	if (SDL_HasAltiVec())
		CPU_features.push_back("AltiVec");

	if (SDL_HasMMX())
		CPU_features.push_back("MMX");

	if (SDL_HasRDTSC())
		CPU_features.push_back("RDTSC");

	if (SDL_HasSSE())
		CPU_features.push_back("SSE");

	if (SDL_HasSSE2())
		CPU_features.push_back("SSE2");

	if (SDL_HasSSE3())
		CPU_features.push_back("SSE3");

	if (SDL_HasSSE41())
		CPU_features.push_back("SSE41");

	if (SDL_HasSSE42())
		CPU_features.push_back("SSE42");
	// Finished collecting CPU features

	Start();
	return ret;
}

// ---------------------------------------------

bool Application::Start()
{
	bool ret = true;

	std::list<Module*>::iterator item = list_modules.begin();

	//Call Start() in all modules
	ConsoleLOG("Application Start --------------");
	item = list_modules.begin();

	while (item != list_modules.end())
	{
		(*item)->Start();
		item++;
	}
	
	app_framerate.frame_time.Start();

	return ret;
}

bool Application::CleanUp()
{
	bool ret = true;
	std::list<Module*>::reverse_iterator item = list_modules.rbegin();

	while (item != list_modules.rend() && ret == true)
	{
		ret = (*item)->CleanUp();
		item++;
	}
	return ret;
}

// ---------------------------------------------

// Called before each update
void Application::PrepareUpdate()
{
	BROFILER_CATEGORY("App Prepare Update", Profiler::Color::DarkRed);

	// App framerate
	app_framerate.FrameStart();

	// Game framerate
	CheckGameState();

	if (gamePlaying)
		mustRunGame = (!gamePaused || gameTick);
	else
		mustRunGame = false;
		
	if (mustRunGame)
		game_framerate.FrameStart(game_speed, gameTick);

	//Update Hardware info such as VRAM usage
	GLint nTotalMemoryInKB = 0;
	int temp;
	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &temp); // Total Available Memory in KB
	hardware.GPU.VRAM.available = (float)temp;

	glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &temp); // Dedicated VRAM in KB
	hardware.GPU.VRAM.usage = (float)temp;

	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &temp); // Total VRAM Memory in KB
	hardware.GPU.VRAM.budget = (float)temp;
}

// Called each loop iteration
update_status Application::Update()
{
	update_status ret = UPDATE_CONTINUE;
	PrepareUpdate();

	if (ret == UPDATE_CONTINUE)
		ret = PreUpdateModules();

	if (ret == UPDATE_CONTINUE)
		ret = UpdateModules();

	if (ret == UPDATE_CONTINUE)
		ret = PostUpdateModules();

	FinishUpdate();

	return ret;
}

// Called after each update
void Application::FinishUpdate()	//TODO: Separate in functions (Save&Load, Framerate Calculations, Hardware display update)
{
	BROFILER_CATEGORY("App Finish Update", Profiler::Color::IndianRed);

	// CHANGE/FIX: If .FBX meta is loaded instead of created, SaveScene file creation malfunctions, so until we find a fix a file for saving will always be created if there isn't one
	std::string scene_path;
	scene_path = ASSETS_FOLDER;
	scene_path += "scene_1.scene";
	bool scene_exists = App->file_system->Exists(scene_path.data());
	if (!scene_exists)
	{
		std::string scene_name = "scene_1";
		App->scene_intro->scene_ie.SaveScene(App->scene_intro->root, scene_name, FileType::SCENE);
	}

	//Process Save&Load
	CheckFileEditRequests();
	
	//Framerate Calculations
	app_framerate.FrameEnd();

	if (gamePlaying) {
		if (!gamePaused || gameTick) {
			game_framerate.FrameEnd(gameTick);

			//CHANGE/FIX: For different reasons this doesn't fully work, but there's progress made (longer life on slow-motion kinda works)
			//if (game_speed != 1.f)
			//	game_framerate.AlterateTimeFlow(game_speed);

			gameTick = false;
		}

		gameFirstFrame = false;
	}

	// Update the fps Log
	fps_log.push_back(app_framerate.prev_sec_frame_count);
	if (fps_log.size() > 100)
		fps_log.erase(fps_log.begin());

	// Update the ms Log
	ms_log.push_back(app_framerate.frame_ms);
	if (ms_log.size() > 100)
		ms_log.erase(ms_log.begin());

	// Delay next update
	BROFILER_CATEGORY("App Delay", Profiler::Color::Gray);

	if (max_FPS > 0)
		capped_ms = 1000 / max_FPS;
	else
		capped_ms = -1;

	if (capped_ms > 0 && app_framerate.frame_ms < capped_ms)
		SDL_Delay(capped_ms - app_framerate.frame_ms);

	//CHANGE/FIX: Put corner FPS number
	/*if (map->mapDebugDraw) {
		std::string tmp = std::to_string(prev_last_sec_frame_count);
		gui->fpsText->ChangeString(tmp);
	}*/
}

// ---------------------------------------------

void Application::RequestBrowser(const char* link)
{
	ShellExecuteA(NULL, "open", link, NULL, NULL, SW_SHOWNORMAL);
}

void Application::ConsoleLOG(const char * format, ...)
{
	static char tmp_string[4096];
	static char tmp_string2[4096];
	static va_list  ap;

	// Construct the string from variable arguments
	va_start(ap, format);
	vsprintf_s(tmp_string, 4096, format, ap);
	va_end(ap);
	sprintf_s(tmp_string2, 4096, "%s", tmp_string);
	OutputDebugString(tmp_string2);
	// First we do  a normal LOG to VS using a modified LOG that just returns the LOG message
	const char* str = tmp_string2;
	
	//Now we can pass the string to the vector
	LOG.push_back(str);
	editor->editor_console->AddLog(str);
}

// PreUpdate all modules in App
update_status Application::PreUpdateModules()
{
	BROFILER_CATEGORY("Modules Pre-Update", Profiler::Color::PaleVioletRed);

	update_status ret = UPDATE_CONTINUE;
	Module* pModule = NULL;
	std::list<Module*>::iterator item = list_modules.begin();

	for (; item != list_modules.end() && ret == UPDATE_CONTINUE; item = next(item)) {

		pModule = (*item);
		if (pModule->active == false)
			continue;

		ret = (*item)->PreUpdate(app_framerate.dt);
	}

	return ret;
}

// Update all modules in App
update_status Application::UpdateModules()
{
	BROFILER_CATEGORY("Modules Update", Profiler::Color::Crimson);

	update_status ret = UPDATE_CONTINUE;
	Module* pModule = NULL;

	std::list<Module*>::iterator item = list_modules.begin();
	for (; item != list_modules.end() && ret == UPDATE_CONTINUE; item = next(item)) {

		pModule = *item;
		if (pModule->active == false)
			continue;

		ret = (*item)->Update(app_framerate.dt);

		if (ret == UPDATE_CONTINUE && mustRunGame) {
			ret = (*item)->GameUpdate(game_framerate.dt);
		}
	}

	return ret;
}

// PostUpdate all modules in App
update_status Application::PostUpdateModules()
{
	BROFILER_CATEGORY("Modules Post-Update", Profiler::Color::MediumVioletRed);

	update_status ret = UPDATE_CONTINUE;
	Module* pModule = NULL;
	std::list<Module*>::iterator item = list_modules.begin();

	for (; item != list_modules.end() && ret == UPDATE_CONTINUE; item = next(item)) {

		pModule = *item;
		if (pModule->active == false)
			continue;

		ret = (*item)->PostUpdate(app_framerate.dt);
	}

	return ret;
}

void Application::CheckGameState()
{
	if (App->scene_intro->playing && !gamePlaying) {		// If game needs to start
		sceneBackupPath = file_system->GetPathToGameFolder() + "game" + scene_intro->scene_ie.SaveScene(App->scene_intro->root, std::string("Scene_Backup"), FileType::SCENE);
		game_framerate.Start();
		gameFirstFrame = true;
		LOG("[Info]: Started Game.");
	}
	else if (!App->scene_intro->playing && gamePlaying) {	// If game needs to stop
		scene_intro->scene_ie.LoadScene(sceneBackupPath.c_str(), FileType::SCENE);
		game_framerate.Stop();
		gamePaused = false;
		scripting->Stop();
		LOG("[Info]: Stopped Game.");
	}
	
	gamePlaying = App->scene_intro->playing;

	if (gamePlaying) {	// If game running
		if (App->scene_intro->paused && !gamePaused) {		// If needs to pause
			game_framerate.Stop();
			LOG("[Info]: Paused Game.");
		}
		else if (!App->scene_intro->paused && gamePaused) {	// If needs to unpause
			game_framerate.Resume();
			LOG("[Info]: Resumed Game.");
		}

		gamePaused = App->scene_intro->paused;

		if (App->scene_intro->requestTick) {	//DEBUG
			LOG("[Info]: Advanced game by 1 frame.");
		}

		gameTick = App->scene_intro->requestTick;	// Pass tick request
		App->scene_intro->requestTick = false;
	}
}

// Adding a module
void Application::AddModule(Module* mod)
{
	list_modules.push_back(mod);
}

// Grouped Operations
void Application::CheckFileEditRequests()
{
	//Save
	if (config_save == true) {
		SaveConfig();
		config_save = false;
	}

	//Load
	if (config_load == true) {
		LoadUserConfig();
		config_load = false;
	}

	//Reset
	if (config_reset == true) {
		LoadDefaultConfig();
		config_reset = false;
	}
}

// Get App data
const char* Application::GetTitle() const
{
	return title.c_str();
}

const char* Application::GetOrganization() const
{
	return organization.c_str();
}

const char* Application::GetLicense() const
{
	return license.c_str();
}

const char* Application::GetAuthors() const
{
	return authors.c_str();
}

// Game State
bool Application::IsGameFirstFrame() const
{
	return gameFirstFrame;
}

bool Application::IsGamePlaying() const
{
	return gamePlaying;
}

bool Application::IsGamePaused() const
{
	return gamePaused;
}

bool Application::IsGameTick() const
{
	return gameTick;
}

// Time
float Application::GetRealDT() const
{
	return app_framerate.dt;
}

float Application::GetGameDT() const
{
	return game_framerate.dt;
}

uint Application::GetRealTime() const
{
	return app_framerate.time_since_start.Read();
}

uint Application::GetGameTime() const
{
	return game_framerate.time_since_start.Read();
}

// Save / Load Requests
void Application::RequestLoad(const char* file)
{
	// we should be checking if that file actually exist
	// from the "GetSaveGames" list
	projFilename.assign(file);
	want_to_load = true;
}

void Application::RequestSave(const char* file) const
{
	// we should be checking if that file actually exist
	// from the "GetSaveGames" list ... should we overwrite ?
	projFilename.assign(file);
	want_to_save = true;
}

void Application::RequestConfigSave() const
{
	config_save = true;
}

void Application::RequestConfigLoad()
{
	config_load = true;
}

void Application::RequestConfigReset()
{
	config_reset = true;
}

// Save / Load Operations
bool Application::LoadProject(const char* file)
{
	bool ret = true;

	return ret;
}

bool Application::SaveProject(const char* file) const
{
	bool ret = true;

	return ret;
}

std::string  Application::SubtractString(std::string & str, const char * chars_to_find, bool reading_backwards, bool subtract_until_char, bool include_char)	//IMPROVE: Make it into a helper/tool file
{
	std::string ret = "error substracting string";
	std::size_t found;
	if (reading_backwards) //Reading from END to BEGINNING
		found = str.find_last_of(chars_to_find);
	else  //Reading from BEGINNING to END
		found = str.find_first_of(chars_to_find);

	if (subtract_until_char) // subtract from beginning to char
		if (include_char)
			ret = str.substr(0, found + 1);
		else
			ret = str.substr(0, found);
	else					// subtract from char to beginning
		if (include_char)
			ret = str.substr(found, str.size());
		else
			ret = str.substr(found + 1, str.size());

	return ret;
}

bool Application::LoadDefaultConfig()
{
	json config = jLoad.Load(defaultConfig.c_str());
	return LoadConfig(config);
}

bool Application::LoadUserConfig()
{
	json config = jLoad.Load(editableConfig.c_str());
	return LoadConfig(config);
}

bool Application::LoadConfig(json& obj)	//IMPROVE: Divide the loading in sections, each processed by its correspondant module using a json obj pointer as parameter
{
	bool ret = true;

	//App
	title = obj["App"]["Title"].get<std::string>();
	organization = obj["App"]["Organization"].get<std::string>();
	authors = obj["App"]["Authors"].get<std::string>();
	license = obj["App"]["License"].get<std::string>();
	max_FPS = obj["App"]["Max FPS"].get<int>();

	//Window
	window->window_width = obj["Window"]["Width"].get<int>();
	window->window_max_width = obj["Window"]["Max Width"].get<int>();
	window->window_height = obj["Window"]["Height"].get<int>();
	window->window_max_height = obj["Window"]["Max Height"].get<int>();
	window->window_scale = obj["Window"]["Scale"].get<int>();
	window->window_brightness = obj["Window"]["Brightness"].get<float>();
	window->window_fullscreen = obj["Window"]["Fullscreen"].get<bool>();
	window->window_resizable = obj["Window"]["Resizable"].get<bool>();
	window->window_borderless = obj["Window"]["Borderless"].get<bool>();
	window->window_full_desktop = obj["Window"]["FullscreenDesktop"].get<bool>();

	//Input

	//Camera
	// -> Movement
	camera->camMovSpeed = obj["Camera"]["Movement Speed"].get<float>();
	camera->maxMovSpeed = obj["Camera"]["Max Movement Speed"].get<float>();
	camera->camMovMultiplier = obj["Camera"]["Movement Multiplier"].get<float>();
	camera->maxMovMultiplier = obj["Camera"]["Max Movement Multiplier"].get<float>();

	// -> Rotation
	/*camera->camRotSpeed = obj["Camera"]["Rotation Speed"].get<float>();
	camera->maxRotSpeed = obj["Camera"]["Max Rotation Speed"].get<float>();
	camera->camRotMultiplier = obj["Camera"]["Rotation Multiplier"].get<float>();
	camera->maxRotMultiplier = obj["Camera"]["Max Rotation Multiplier"].get<float>();*/
	
	// -> Mouse Sensibility
	camera->mouseSens = obj["Camera"]["Mouse Sensitivity"].get<float>();
	camera->maxMouseSens = obj["Camera"]["Max Mouse Sensitivity"].get<float>();

	// -> Scroll Sensibility
	camera->scrollSens = obj["Camera"]["Scroll Sensitivity"].get<float>();
	camera->maxScrollSens = obj["Camera"]["Max Scroll Sensitivity"].get<float>();

	//Renderer
	renderer3D->vSync = obj["Renderer3D"]["VSync"].get<bool>();

	// -> GL_Settings Loading (GL Id, mutually exclusive group linking, initial status)
	renderer3D->GL_DepthTest = { GL_DEPTH_TEST, nullptr, obj["Renderer3D"]["GL_Settings"]["DepthTest"].get<bool>() };
	renderer3D->GL_CullFace = { GL_CULL_FACE, nullptr, obj["Renderer3D"]["GL_Settings"]["CullFace"].get<bool>() };
	renderer3D->GL_Lighting = { GL_LIGHTING, nullptr, obj["Renderer3D"]["GL_Settings"]["Lightning"].get<bool>() };
	renderer3D->GL_ColorMaterial = { GL_COLOR_MATERIAL, nullptr, obj["Renderer3D"]["GL_Settings"]["ColorMaterial"].get<bool>() };

	renderer3D->GL_Texture2D = { GL_TEXTURE_2D,  &renderer3D->GL_TextureModes, obj["Renderer3D"]["GL_Settings"]["Texture2D"].get<bool>() };
	renderer3D->GL_TextureModes.push_back(&renderer3D->GL_Texture2D);
	renderer3D->GL_TextureCubeMap = { GL_TEXTURE_CUBE_MAP,  &renderer3D->GL_TextureModes, obj["Renderer3D"]["GL_Settings"]["TextureCubeMap"].get<bool>() };
	renderer3D->GL_TextureModes.push_back(&renderer3D->GL_TextureCubeMap);

	renderer3D->GL_Blend = { GL_BLEND, nullptr, obj["Renderer3D"]["GL_Settings"]["Blend"].get<bool>() };
	renderer3D->GL_Dither = { GL_DITHER, nullptr, obj["Renderer3D"]["GL_Settings"]["Dither"].get<bool>() };
	renderer3D->GL_PointSmooth = { GL_POINT_SMOOTH, nullptr, obj["Renderer3D"]["GL_Settings"]["PointSmooth"].get<bool>() };
	renderer3D->GL_LineSmooth = { GL_LINE_SMOOTH, nullptr, obj["Renderer3D"]["GL_Settings"]["LineSmooth"].get<bool>() };
	renderer3D->GL_LineStipple = { GL_LINE_STIPPLE, nullptr, obj["Renderer3D"]["GL_Settings"]["LineStipple"].get<bool>() };
	renderer3D->GL_PolygonSmooth = { GL_POLYGON_SMOOTH, nullptr, obj["Renderer3D"]["GL_Settings"]["PolygonSmooth"].get<bool>() };
	renderer3D->GL_PolygonStipple = { GL_POLYGON_STIPPLE, nullptr, obj["Renderer3D"]["GL_Settings"]["PolygonStipple"].get<bool>() };
	renderer3D->GL_MinMax = { GL_MINMAX, nullptr, obj["Renderer3D"]["GL_Settings"]["MinMax"].get<bool>() };
	renderer3D->GL_MultiSample = { GL_MULTISAMPLE, nullptr, obj["Renderer3D"]["GL_Settings"]["MultiSample"].get<bool>() };

	//Editor
	// -> Windows
	editor->editor_toolbar->show_window = obj["Editor"]["Windows"]["Toolbar"].get<bool>();
	//editor->editor_configuration->show_window = obj["Editor"]["Windows"]["Configuration"].get<bool>();
	editor->editor_console->show_window = obj["Editor"]["Windows"]["Console"].get<bool>();
	editor->editor_game->show_window = obj["Editor"]["Windows"]["Game"].get<bool>();
	editor->editor_hierarchy->show_window = obj["Editor"]["Windows"]["Hierarchy"].get<bool>();
	editor->editor_inspector->show_window = obj["Editor"]["Windows"]["Inspector"].get<bool>();
	editor->editor_project->show_window = obj["Editor"]["Windows"]["Project"].get<bool>();
	editor->editor_scene->show_window = obj["Editor"]["Windows"]["Scene"].get<bool>();

	// -> RadioButton Modes
	editor->drawMode = obj["Editor"]["Modes"]["Mesh Draw"].get<int>();
	editor->textureMode = obj["Editor"]["Modes"]["Texture"].get<int>();
	editor->byteSizeMode = obj["Editor"]["Modes"]["Byte Size"].get<int>();

	return ret;
}

bool Application::SaveConfig() const	//IMPROVE: Divide the saving in sections, each processed by its correspondant module using a json obj pointer as parameter
{
	bool ret = true;

	//Create JSON obj with imported values
	json config = {
		{"App", {
			{"Title", title.c_str()},
			{"Organization", organization.c_str()},
			{"Authors", authors.c_str()},
			{"License", license.c_str()},
			{"Max FPS", max_FPS}
		}},

		{"Window", {
			{"Width", window->window_width},
			{"Max Width", window->window_max_width},
			{"Height", window->window_height},
			{"Max Height", window->window_max_height},
			{"Scale", window->window_scale},
			{"Brightness", window->window_brightness},
			{"Fullscreen", window->window_fullscreen},
			{"Resizable", window->window_resizable},
			{"Borderless", window->window_borderless},
			{"FullscreenDesktop", window->window_full_desktop}
		}},

		{"Input", {
	
		}},

		{"Camera", {
			{"Movement Speed", camera->camMovSpeed},
			{"Max Movement Speed", camera->maxMovSpeed},
			{"Movement Multiplier", camera->camMovMultiplier},
			{"Max Movement Multiplier", camera->maxMovMultiplier},

			{"Mouse Sensitivity", camera->mouseSens},
			{"Max Mouse Sensitivity", camera->maxMouseSens},

			{"Scroll Sensitivity", camera->scrollSens},
			{"Max Scroll Sensitivity", camera->maxScrollSens}
		}},

		{"Renderer3D", {
			{"VSync", renderer3D->vSync},
			{"GL_Settings", {
				{"DepthTest", renderer3D->GL_DepthTest.status},
				{"CullFace", renderer3D->GL_CullFace.status},
				{"Lightning", renderer3D->GL_Lighting.status},
				{"ColorMaterial", renderer3D->GL_ColorMaterial.status},
				{"Texture2D", renderer3D->GL_Texture2D.status},
				{"TextureCubeMap", renderer3D->GL_TextureCubeMap.status},
				{"Blend", renderer3D->GL_Blend.status},
				{"Dither", renderer3D->GL_Dither.status},
				{"PointSmooth", renderer3D->GL_PointSmooth.status},
				{"LineSmooth", renderer3D->GL_LineSmooth.status},
				{"LineStipple", renderer3D->GL_LineStipple.status},
				{"PolygonSmooth", renderer3D->GL_PolygonSmooth.status},
				{"PolygonStipple", renderer3D->GL_PolygonStipple.status},
				{"MinMax", renderer3D->GL_MinMax.status},
				{"MultiSample", renderer3D->GL_MultiSample.status}
			}}
		}},

		{"Editor", {
			{"Modes", {
				{"Mesh Draw", editor->drawMode},
				{"Texture", editor->textureMode},
				{"Byte Size", editor->byteSizeMode}
			}},
			{"Windows", {
				//{"Configuration", editor->editor_configuration->show_window},
				{"Console", editor->editor_console->show_window},
				{"Game", editor->editor_game->show_window},
				{"Hierarchy", editor->editor_hierarchy->show_window},
				{"Inspector", editor->editor_inspector->show_window},
				{"Project", editor->editor_project->show_window},
				{"Scene", editor->editor_scene->show_window},
				{"Toolbar", editor->editor_toolbar->show_window}
			}},
		}}
	};

	jLoad.Save(config, editableConfig.c_str());

	return ret;
}