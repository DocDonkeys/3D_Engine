#ifndef __MODULESCRIPTING_H__
#define __MODULESCRIPTING_H__

#include "Module.h"
#include <vector>

class lua_State;
class ComponentScript;
struct ScriptFile;
struct ScriptInstance;

class ModuleScripting : public Module
{
public:
	ModuleScripting(Application* app, const char* name = "null", bool start_enabled = true);
	~ModuleScripting();


	void SendScriptToModule(ComponentScript* script_component, std::string full_file_path);
	ScriptFile* AddScriptFile(ComponentScript* script_component, std::string full_file_path);

public:

	bool Init();
	bool Start();
	bool CleanUp();
	update_status Update(float dt);

private:
	// L is our Lua Virtual Machine, it's called L because its the common name it receives, so all programers can understand what this var is
	lua_State *L = nullptr;
	bool start = true;

	std::vector<ScriptFile*> script_files;
	std::vector<ScriptInstance*> class_instances;
};


class Scripting
{
public:
	Scripting();
	~Scripting();

public:

	void LogFromLua(const char* string);
	void TestFunc();
};


#endif // !__MODULESCRIPTING_H__

