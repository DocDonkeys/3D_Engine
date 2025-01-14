#ifndef __MODULE_RESOURCES_H__
#define __MODULE_RESOURCES_H__

#include "Globals.h"
#include "Module.h"
#include "Resource.h"
#include "Timer.h"
#include <string>
#include <map>

class Application;
class ImporterScene;
class ImporterMaterial;

struct AbstractDir
{
	AbstractDir* parent_dir = nullptr;
	std::string dir_path;
	std::string dir_name;
	std::vector<AbstractDir*> sub_dirs;
	std::vector<std::string> files;
	std::vector<int64> file_modtimes;

	~AbstractDir();
};

class ModuleResources : public Module
{
public:
	ModuleResources (Application* app, const char* name = "null", bool start_enabled = true);
	~ModuleResources();

	bool CleanUp();

	bool Init();
	update_status PreUpdate(float dt);

public:
	uint32 Find(const char* file_in_assets) const;

	//ImportFile serves the purpose of loading a file that is not on the application, it will decide wether it has to create metadata
	// & own file formats or if it will be using metadata and fileformats directly, calling functions and creating the resources to load the file
	uint32 ImportFile(const char* full_path);
	uint32 ImportInternalFile(const char* new_file_in_assets);
	uint32 GenerateNewUID();
	const Resource* Get(uint32 uid) const;
	Resource* Get(uint32 uid);
	Resource* CreateNewResource(Resource::Type type, uint32 force_uid = 0);
	int64 GetTimestampFromMeta(const char * path, bool game_path);

	//Return all files with this extension, slow recursive function
	void GetAllFilesWithExtension(std::string& extension, std::vector<std::string>& files,AbstractDir* directory);

	void ManageModifiedFile(std::string extension);

	Resource::Type ResourceTypeByPath(const std::string extension);

	uint32 GetUIDFromPath(const char* full_path, Resource::Type type);

public: //Vars
	AbstractDir* assets_dir = nullptr;
	AbstractDir* selected_dir = nullptr;

	ImporterMaterial* ie_material = nullptr;
	ImporterScene*	 ie_scene = nullptr;
private:
	//This function initially fills the directories and files assets tree, with all data found inside the Assets folder
	void InitPopulateAssetsDir(AbstractDir* abs_dir);

	void CheckDirectoryUpdate(AbstractDir* abs_dir);
	void CheckFilesUpdate(AbstractDir* abs_dir);

private: 
	std::map<uint32, Resource*> resources;

	//File monitoring
	Timer timer;
	std::vector<std::string> known_files;
	std::vector<std::string> known_dirs;

	std::vector<std::string> extension_3D_file;
	std::vector<std::string> extension_texture;
	std::string meta = "meta";
};
#endif // !__MODULE_RESOURCES_H__

