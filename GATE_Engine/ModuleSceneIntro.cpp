#include "Globals.h"
#include "Application.h"
#include "ModuleSceneIntro.h"
#include "ModuleCamera3D.h"
#include "ModuleResources.h"
#include "ModuleInput.h"
#include "TextureLoader.h"
#include "GeometryLoader.h"
#include "ModuleEditor.h"
#include "ModuleFileSystem.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ImporterScene.h"
#include "Tree.h"

#include "libs/SDL/include/SDL_keycode.h"
#include "libs/SDL/include/SDL_mouse.h"
#include "libs/SDL/include/SDL_assert.h"

#include "libs/MathGeoLib/include/Math/MathConstants.h"
#include "libs/MathGeoLib/include/Geometry/LineSegment.h"
#include "libs/MathGeoLib/include/Geometry/Triangle.h"

// Memory Leak Detection
#include "MemLeaks.h"

ModuleSceneIntro::ModuleSceneIntro(Application* app, const char* name, bool start_enabled) : Module(app, name, start_enabled)
{
}

ModuleSceneIntro::~ModuleSceneIntro()
{
}

// Load assets
bool ModuleSceneIntro::Start()
{
	//LOG("Loading Intro assets");
	LOG("Loading Intro assets");
	bool ret = true;

	//Create the Root node for Game Objects
	root = new GameObject();
	ComponentTransform* trans = (ComponentTransform*)root->GetComponent(COMPONENT_TYPE::TRANSFORM);
	trans->needsUpdateGlobal = false;

	//Setup camera
	App->camera->Move(float3(15.f, 15.f, 15.f));
	App->camera->LookAt(float3::zero);

	
	//App->resources->ImportFile("Assets\\3D_Objects\\Baker_house\\BakerHouse.fbx");

	// Scene Tree
	staticTree = new Tree(Tree::TREE_TYPE::OC_TREE, AABB(float3(-200.f, -120.f, -200.f), float3(200.f, 100.f, 200.f)), 2);

	//Load the StreetScene
	std::string scene_path;
	scene_path = ASSETS_FOLDER;
	scene_path += "scene_1.scene";
	bool scene_exists = App->file_system->Exists(scene_path.data());
	//if (scene_exists)
	//{
	//	scene_path = App->file_system->GetPathToGameFolder(true) + scene_path;
	//	scene_ie.LoadScene(scene_path.data(), FileType::SCENE);
	//}
	//else
	//{
	//	App->resources->ImportFile("Assets\\3D_Objects\\street\\Assignment2_street.FBX");
	//}

	std::vector<const GameObject*> sceneObjects;
	GOFunctions::FillArrayWithChildren(sceneObjects, root);

	for (int i = 0; i < sceneObjects.size(); i++)
		if (sceneObjects[i]->staticObj)
			staticTree->Insert(sceneObjects[i]);

	return ret;
}

// Load assets
bool ModuleSceneIntro::CleanUp()
{
	LOG("Unloading Intro scene");

	RELEASE(root);
	RELEASE(staticTree);

	return true;
}

GameObject* ModuleSceneIntro::CreateEmptyGameObject()
{
	GameObject* go = new GameObject();
	GOFunctions::ReParentGameObject(go, root);
	return go;
}

GameObject* ModuleSceneIntro::CreateEmptyGameObject(const char* name)
{
	GameObject* go = new GameObject(name);
	GOFunctions::ReParentGameObject(go, root);
	return go;
}

GameObject* ModuleSceneIntro::CreateEmptyGameObject(COMPONENT_TYPE comp)
{
	GameObject* go = new GameObject();
	go->CreateComponent(comp);

	switch (comp) {
	case COMPONENT_TYPE::CAMERA:
		go->name = "Camera_" + std::to_string(App->scene_intro->numObjects++);
		break;
	default:
		go->name = "GameObject_" + std::to_string(App->scene_intro->numObjects++);
		break;
	}
	
	GOFunctions::ReParentGameObject(go, root);
	return go;
}

void ModuleSceneIntro::CreateEmptyGameObject(int num_of_go)
{
	for (int i = 0; i < num_of_go; ++i)
	{
		CreateEmptyGameObject();
	}
}

void ModuleSceneIntro::DestroyGameObject(GameObject* go)
{
	//Set all possible references to the gaemObject to nullptr or out of the array
	if (selected_go == go)
		selected_go = nullptr;

	GOFunctions::ReParentGameObject(go, nullptr);
	RELEASE(go);
	numObjects--;
}

void ModuleSceneIntro::MarkGameObjectDestruction(GameObject* go)
{
	if (go != nullptr)
		go->mustDestroy = true;
}

bool ModuleSceneIntro::AddTextureToGameObject(GameObject * go, const char* path)
{
	bool has_material_component = false;

	ResourceTexture* texture = App->texture_loader->LoadTextureFile(path); //TODO: Didac Make it work with module resources

	for (int i = 0; i < go->components.size(); ++i)
	{
		if (go->components[i]->type == COMPONENT_TYPE::MATERIAL)
		{
			ComponentMaterial* material = (ComponentMaterial*)go->GetComponent(COMPONENT_TYPE::MATERIAL);
			material->AssignTexture(texture);
			return true;
		}
	}

	if (!has_material_component)
	{
		ComponentMaterial* material = (ComponentMaterial*)go->CreateComponent(COMPONENT_TYPE::MATERIAL);
		material->AssignTexture(texture);
		return true;
	}

	return false;
}

// PreUpdate
update_status ModuleSceneIntro::PreUpdate(float realDT)
{
	root->PreUpdate(realDT);

	return UPDATE_CONTINUE;
}

// Update
update_status ModuleSceneIntro::Update(float realDT)
{
	if (!App->editor->using_menu)
		if (App->IsGamePlaying() || App->input->GetMouseButton(SDL_BUTTON_RIGHT) == KEY_IDLE)	// IMPROVE: Define better the limitations of using tool/camera controls (Like a condition of being in Scene or Editor view)
			toolMode = CheckToolMode();

	root->Update(realDT);

	return UPDATE_CONTINUE;
}

// Game Update
update_status ModuleSceneIntro::GameUpdate(float gameDT)
{
	root->GameUpdate(gameDT);

	return UPDATE_CONTINUE;
}

// PostUpdate
update_status ModuleSceneIntro::PostUpdate(float realDT)
{
	root->PostUpdate(realDT);

	std::vector<GameObject*> sceneObjects;
	GOFunctions::FillArrayWithModifiableChildren(sceneObjects, root);

	for (int i = sceneObjects.size() - 1; i >= 0; --i)
		if (sceneObjects[i]->mustDestroy) {
			GOFunctions::ReParentGameObject(sceneObjects[i], nullptr);
			RELEASE(sceneObjects[i]);
			numObjects--;
		}

	return UPDATE_CONTINUE;
}

GameObject* ModuleSceneIntro::IntersectRay(const LineSegment& segment, float& dist, bool nearest) const
{
	dist = inf;
	GameObject* chosen = nullptr;

	std::map<float, const GameObject*> objCollector;
	std::vector<const GameObject*> rootCollector;
	float nearHit, farHit;

	// Dynamic Intersection
	GOFunctions::FillArrayWithChildren(rootCollector, root);
	for (int i = 0; i < rootCollector.size(); i++)
		if (!rootCollector[i]->staticObj
			&& rootCollector[i]->active
			&& rootCollector[i]->GetComponent(COMPONENT_TYPE::MESH) != nullptr
			&& segment.Intersects(rootCollector[i]->aabb, nearHit, farHit))
			if (nearest)
				objCollector[nearHit] = rootCollector[i];
			else
				objCollector[farHit] = rootCollector[i];

	// Static Intersections
	staticTree->Intersects(objCollector, segment);

	for (std::map<float, const GameObject*>::const_iterator it = objCollector.begin(); it != objCollector.end(); it = next(it))
	{
		const GameObject* go = it->second;
		ComponentTransform* compTrs = (ComponentTransform*)go->GetComponent(COMPONENT_TYPE::TRANSFORM);
		ComponentMesh* compMesh = (ComponentMesh*)go->GetComponent(COMPONENT_TYPE::MESH);

		if (compMesh != nullptr)
		{
			ResourceMesh* mesh = compMesh->mesh;

			LineSegment seg(segment);
			seg.Transform(compTrs->GetGlobalMat().Inverted());

			Triangle tri;
			for (uint i = 0; i < mesh->num_index; i += 3)
			{
				tri.a = mesh->vertex[mesh->index[i]];
				tri.b = mesh->vertex[mesh->index[i + 1]];
				tri.c = mesh->vertex[mesh->index[i + 2]];

				float distance;
				float3 hit_point;
				if (seg.Intersects(tri, &distance, &hit_point))
				{
					if (nearest && distance < dist || !nearest && distance > dist)
					{
						dist = distance;
						chosen = (GameObject*)go;
					}
				}
			}
		}
	}

	return chosen;
}

bool ModuleSceneIntro::ChangeScene(GameObject* new_root)
{
	//Delete the current root and change it by the root of the loaded scene
	root->children.erase(root->children.end() - 1);	// The new root was stored as a child of current root, so we take it out before deleting the original root

	if (new_root->children.size() == 0) {	//CHANGE/FIX: Workaround. If an fbx.meta exists, the highest parents in the new root's heriarchy are pushed as children of the old root, so we reparent them
		uint rootChildrenSize = root->children.size();
		for (int i = rootChildrenSize - 1; i >= rootChildrenSize / 2; i--) {
			if (i >= 0)
			{
				GOFunctions::ReParentGameObject(root->children[i], new_root);
			}
			else
				break;
		}
	}

	RELEASE(root);
	root = new_root;
	root->parent = nullptr;
	App->scene_intro->selected_go = nullptr;

	// Restart tree with new objects
	staticTree->Clear();

	std::vector<const GameObject*> collector;
	GOFunctions::FillArrayWithChildren(collector, root);
	for (int i = 0; i < collector.size(); i++) {
		if (collector[i]->staticObj) {
			staticTree->Insert(collector[i]);
		}
	}

	return false;
}

int ModuleSceneIntro::CheckToolMode()
{
	int ret = toolMode;

	if (App->input->GetKey(SDL_SCANCODE_LALT) == KEY_DOWN && App->input->GetMouseButton(SDL_BUTTON_MIDDLE) == KEY_IDLE
		|| App->input->GetKey(SDL_SCANCODE_LALT) == KEY_IDLE && App->input->GetMouseButton(SDL_BUTTON_MIDDLE) == KEY_DOWN)
	{
		ret = (int)tool_mode::DRAG;
		lastToolMode = toolMode;
	}
	else if (App->input->GetKey(SDL_SCANCODE_LALT) == KEY_UP && App->input->GetMouseButton(SDL_BUTTON_MIDDLE) == KEY_IDLE
		|| App->input->GetKey(SDL_SCANCODE_LALT) == KEY_IDLE && App->input->GetMouseButton(SDL_BUTTON_MIDDLE) == KEY_UP) {
		ret = lastToolMode;
	}
	else if (App->input->GetKey(SDL_SCANCODE_LALT) == KEY_IDLE && App->input->GetMouseButton(SDL_BUTTON_MIDDLE) == KEY_IDLE) {
		if (App->input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN)
			ret = (int)tool_mode::DRAG;
		else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
			ret = (int)tool_mode::TRANSLATE;
		else if (App->input->GetKey(SDL_SCANCODE_E) == KEY_DOWN)
			ret = (int)tool_mode::ROTATE;
		else if (App->input->GetKey(SDL_SCANCODE_R) == KEY_DOWN)
			ret = (int)tool_mode::SCALE;
		/*else if (App->input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
			ret = (int)tool_mode::BOUNDS;*/
	}

	return ret;
}