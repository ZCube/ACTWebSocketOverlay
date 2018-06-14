/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "Preference.h"

void PreferenceBase::SetRoot(PreferenceBase * root)
{
	this->root = root;
	this->instance = root->instance;
}

void PreferenceBase::Save()
{
	if (root != nullptr)
	{
		root->Save();
	}
}

void PreferenceBase::Load()
{
	if (root != nullptr)
	{
		root->Load();
	}
}

void PreferenceBase::Preferences(PreferenceNode & preference_node, PreferenceStorage & preference_storage)
{
	const char* name = preference_node.name.c_str();
	if (preference_node.map.size())
	{
		if (ImGui::TreeNode(name))
		{
			for (auto i = preference_node.map.begin(); i != preference_node.map.end(); ++i)
			{
				Preferences(i->second, preference_storage);
			}
			ImGui::TreePop();
		}
	}
	else
	{
		switch (preference_node.type)
		{
		case PreferenceNode::Type::Integer:
			if (preference_node.val)
			{
				int* i = reinterpret_cast<int*>(preference_node.val);
				if (ImGui::InputInt(name, i, preference_node.step))
				{
					*i = preference_storage.int_map[name];
					Save();
				}
			}
			else
			{
				if (ImGui::InputInt(name, &preference_storage.int_map[name], preference_node.step))
				{
					Save();
				}
			}
			break;
		case PreferenceNode::Type::Float:
			if (preference_node.val)
			{
				float* i = reinterpret_cast<float*>(preference_node.val);
				if (ImGui::InputFloat(name, i, preference_node.step))
				{
					Save();
				}
			}
			else
			{
				if (ImGui::InputFloat(name, &preference_storage.float_map[name], preference_node.step))
				{
					Save();
				}
			}
			break;
		case PreferenceNode::Type::Boolean:
			if (preference_node.val)
			{
				bool* i = reinterpret_cast<bool*>(preference_node.val);
				if (ImGui::Checkbox(name, i))
				{
					Save();
				}
			}
			else
			{
				if (ImGui::Checkbox(name, &preference_storage.boolean_map[name]))
				{
					Save();
				}
			}
			break;
		case PreferenceNode::Type::Color:
			if (preference_node.val)
			{
				ImVec4* i = reinterpret_cast<ImVec4*>(preference_node.val);
				if (ImGui::ColorEdit3(name, &i->x))
				{
					Save();
				}
			}
			else
			{
				if (ImGui::ColorEdit3(name, &preference_storage.color_map[name].x))
				{
					Save();
				}
			}
			break;
		case PreferenceNode::Type::Color4:
			if (preference_node.val)
			{
				ImVec4* i = reinterpret_cast<ImVec4*>(preference_node.val);
				if (ImGui::ColorEdit4(name, &i->x))
				{
					Save();
				}
			}
			else
			{
				if (ImGui::ColorEdit4(name, &preference_storage.color_map[name].x))
				{
					Save();
				}
			}
			break;
		case PreferenceNode::Type::String:
		{
			if (preference_node.val)
			{
				std::string* i = reinterpret_cast<std::string*>(preference_node.val);
				i->reserve(preference_node.string_max_length + 1);
				if (ImGui::InputText(name, (char*)i->data(), preference_node.string_max_length))
				{
					int len = strnlen_s(i->data(), preference_node.string_max_length);
					i->resize(len);
					Save();
				}
			}
			else
			{
				preference_storage.string_map[name].reserve(preference_node.string_max_length + 1);
				if (ImGui::InputText(name, (char*)preference_storage.string_map[name].data(), preference_node.string_max_length))
				{
					int len = strnlen_s(preference_storage.string_map[name].data(), preference_node.string_max_length);
					preference_storage.string_map[name].resize(len);
					Save();
				}
			}
		}
		break;
		case PreferenceNode::Type::Map:
			break;
		}
	}
}

void PreferenceBase::Preferences()
{
}
