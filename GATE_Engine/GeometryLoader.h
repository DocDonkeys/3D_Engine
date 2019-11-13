#ifndef __GEOMETRYLOADER_H__
#define __GEOMETRYLOADER_H__

#include "Globals.h"
#include "Module.h"
#include <vector>
#include <string>

#include "libs/Assimp/include/cimport.h"

struct par_shapes_mesh_s;
struct aiMesh;
struct aiScene;
struct aiNode;
class Mesh;
class Texture;
class GameObject;

enum class PRIMITIVE
{
	PLANE = 0,
	CUBE,
	SPHERE,
	HEMISPHERE,
	CYLINDER,
	CONE,
	TORUS
};

class GeometryLoader : public Module
{
public:
	GeometryLoader(Application* app, const char* name = "null", bool start_enabled = true);
	~GeometryLoader();

public:
	bool Init();
	bool Start();
	update_status Update(float dt);
	bool CleanUp();

public:
	// Load a 3D file such as an FBX, OBJ etc.
	bool Load3DFile(const char* full_path); 

	//Load a node of ASSIMP, this will create a gameobject for the node and manage the parenting and hierarchy of this new gameobject
	GameObject* LoadAssimpNode(const aiScene* scene, const aiNode* node, 
		const char* absolute_path, const char* filename, const char* full_path, const char* objName, const uint counter);

	// Load a generated primitive (par_shape) into a mesh (Mesh)
	void LoadPrimitiveShape(const par_shapes_mesh_s* p_mesh, const char* name); 
	void LoadPrimitiveNormals(Mesh* mesh, const par_shapes_mesh_s* p_mesh);
	Texture* LoadMaterial(const aiScene * scene, const aiMesh* loaded_mesh, const std::string & absolute_path);

	//Generate a primitive. For CUBE (slices,stacks,radius) will be ignored, for anything else except Torus (radius) will be ignored. Remember radius between 0 & 1.0f
	void CreatePrimitive(PRIMITIVE p, int slices = 0, int stacks = 0, float radius = 0.f);

public: // Vars
	//aiLogStream log_stream;
};

//Function for Assimp calllback
void AssimpLOGCallback(const char * msg, char * userData);
#endif //__GEOMETRYLOADER_H__