#include "DataBinder.h"
#include "Color.h"
#include "GameplayManager.h"
#include "Vec2.h"
#include <stdexcept>
#ifdef GIEWONT_HAS_GRAPHICS
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#endif

using namespace giewont;

DataBinderField::DataBinderField(std::string typeName, std::string name,
                                 size_t offset)
    : name(name), offset(offset) {
  if (typeName == "std::string") {
    type = DataBinderType::STRING;
  } else if (typeName == "float") {
    type = DataBinderType::FLOAT;
  } else if (typeName == "int") {
    type = DataBinderType::INT;
  } else if (typeName == "Vec2") {
    type = DataBinderType::VEC2;
  } else if (typeName == "GColor") {
    type = DataBinderType::COLOR;
  } else if (typeName == "GameplayTeam") {
    type = DataBinderType::GAMEPLAY_TEAM;
  } else {
    throw std::runtime_error("Unknown type name: " + typeName);
  }
}

DataBinderField::DataBinderField(std::string typeName, std::string name,
                                 size_t offset, float min, float max)
    : DataBinderField(typeName, name, offset) {
  this->min = min;
  this->max = max;
}

void DataBinderBase::draw_inspector_ui_impl(void *instance) {
#ifdef GIEWONT_HAS_GRAPHICS
  for (auto &field : fields) {
    switch (field.type) {
    case DataBinderType::STRING: {
      std::string *str = (std::string *)((char *)instance + field.offset);
      ImGui::InputText(field.name.c_str(), str, 256);
      break;
    }
    case DataBinderType::FLOAT: {
      float *f = (float *)((char *)instance + field.offset);
      ImGui::SliderFloat(field.name.c_str(), f, field.min, field.max);
      break;
    }
    case DataBinderType::INT: {
      int *i = (int *)((char *)instance + field.offset);
      ImGui::InputInt(field.name.c_str(), i);
      break;
    }
    case DataBinderType::VEC2: {
      Vec2 *v = (Vec2 *)((char *)instance + field.offset);
      float f[2] = {v->x, v->y};
      ImGui::InputFloat2(field.name.c_str(), f);
      v->x = f[0];
      v->y = f[1];
      break;
    }
    case DataBinderType::COLOR: {
      GColor *c = (GColor *)((char *)instance + field.offset);
      c->editor_ui(field.name.c_str());
      break;
    }
    case DataBinderType::GAMEPLAY_TEAM: {
      GameplayTeam *team = (GameplayTeam *)((char *)instance + field.offset);

      const char *items[] = {"UNKNOWN_TEAM", "RED_TEAM", "BLUE_TEAM"};
      int current_item = (int)*team;
      ImGui::Combo(field.name.c_str(), &current_item, items,
                   IM_ARRAYSIZE(items));
      *team = (GameplayTeam)current_item;

      break;
    }
    }
  }
#endif
}
  