#include "EditorHierarchy.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleSceneIntro.h"

#ifdef _DEBUG
#ifdef _MMGR_MEM_LEAK
#include "libs/mmgr/mmgr.h"
#endif
#endif

EditorHierarchy::EditorHierarchy(const char* name, bool startEnabled, ImGuiWindowFlags flags) : EditorWindow(name, startEnabled, flags) {};

void EditorHierarchy::Update()
{
	if (open_pop_up)
		DrawPopUpWindow();

	 //| ImGuiTreeNodeFlags_Selected;
	for (int i = 0; i < App->scene_intro->root->children.size(); ++i)
	{

	}
	
	for (int i = 0; i < App->scene_intro->root->children.size(); ++i)
	{
		ManageGameObject(App->scene_intro->root->children[i]);
	}
	
	//for (int i = 0; i < App->scene_intro->game_objects.size(); ++i)
	//{
	//	ImGuiTreeNodeFlags tmp_flags = base_flags;
	//	if (focus_node == i)
	//		tmp_flags = base_flags | ImGuiTreeNodeFlags_Selected;
	//	
	//	if (App->scene_intro->game_objects[i]->children.size() == 0)
	//		tmp_flags = tmp_flags | ImGuiTreeNodeFlags_Leaf;

	//	//Print GameObjects Hierarchy
	//	if (ImGui::TreeNodeEx((void*)(intptr_t)i, tmp_flags, App->scene_intro->game_objects[i]->name.data()))
	//		ImGui::TreePop();
	//	
	//	if (ImGui::IsItemClicked(0))
	//	{
	//		focus_node = i;
	//		App->scene_intro->selected_go = App->scene_intro->game_objects[i];
	//	} else if(ImGui::IsItemClicked(1) && ImGui::IsWindowHovered()){
	//		open_pop_up = true;
	//		focus_node = i;
	//		App->scene_intro->selected_go = App->scene_intro->game_objects[i];
	//	}
	//}

	
	
}

void EditorHierarchy::DrawPopUpWindow()
{
	if (ImGui::IsMouseReleased(0) || ImGui::IsMouseReleased(2))
		open_pop_up = false;
	else if (ImGui::IsMouseClicked(1))
		open_pop_up = false;

	ImGui::OpenPopup("Hierarchy Tools");
		if (ImGui::BeginPopup("Hierarchy Tools"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				if (App->scene_intro->selected_go != nullptr)
				App->scene_intro->DestroyGameObject(App->scene_intro->selected_go);
				open_pop_up = false;
			}

			ImGui::EndPopup();
		}
}

void EditorHierarchy::ManageGameObject(GameObject* go)
{
	ImGuiTreeNodeFlags tmp_flags = base_flags;
	if (go == App->scene_intro->selected_go)
		tmp_flags = base_flags | ImGuiTreeNodeFlags_Selected;

	if (go->children.size() == 0)
		tmp_flags = tmp_flags | ImGuiTreeNodeFlags_Leaf;

	//Print GameObjects Hierarchy
	if (ImGui::TreeNodeEx((void*)(intptr_t)0, tmp_flags, go->name.data()))
	{
		if (go->children.size() > 0)
			for (int i = 0; i < go->children.size(); ++i)
			{
				ManageGameObject(go->children[i]);
			}

		ImGui::TreePop();
	}
		

	if (ImGui::IsItemClicked(0))
	{
		App->scene_intro->selected_go = go;
	}
	else if (ImGui::IsItemClicked(1) && ImGui::IsWindowHovered()) {
		open_pop_up = true;
		App->scene_intro->selected_go = go;
	}
}
