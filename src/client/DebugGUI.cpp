#include "DebugGUI.h"
#include "CameraEntity.h"
#include "Util.h"
#include "entities/character/CharacterEntity.h"
#include "entities/character/SmartAICharacterController.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "raylib.h"

using namespace giewont;

void DebugGUI::draw_ui(Game &game) {

  if (show) {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("Show Debug Overlay", nullptr, &show_debug_overlay);
      ImGui::MenuItem("Show Entity Inspector", nullptr, &show_entity_inspector);
      if (ImGui::MenuItem("Swap character controller to SmartAI")) {
        auto &cam = game.camera_ref.get_as<CameraEntity>(game);
        if (cam.entity_to_follow.valid_as<CharacterEntity>(game)) {
          auto &character = cam.entity_to_follow.get_as<CharacterEntity>(game);
          character.controller = std::make_unique<SmartAICharacterController>();
        }
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  if (show_entity_inspector) {
    ImGui::Begin("Entities", &show, 0);
    ImGui::InputTextWithHint("Filter", "Filter", &inspector_filter, 0);
    if (ImGui::Button("Select self")) {
      auto &cam = game.camera_ref.get_as<CameraEntity>(game);
      if (cam.entity_to_follow.valid_as<CharacterEntity>(game)) {
        auto &character = cam.entity_to_follow.get_as<CharacterEntity>(game);
        inspector_selected_entity = character.get_ref();
      }
    }
    if (ImGui::BeginTable("entities_table", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, 320))) {
      ImGui::TableSetupColumn("Local ID");
      ImGui::TableSetupColumn("Net ID");
      ImGui::TableSetupColumn("Type name");
      ImGui::TableSetupColumn("Flags");
      ImGui::TableHeadersRow();
      for (size_t idx = 0; idx < game.entities.size(); idx++) {
        auto &ent = game.entities[idx];
        if (ent != nullptr) {
          if (!inspector_filter.empty() &&
              !string_contains_case_insensitive(ent->get_type_name(),
                                                inspector_filter.c_str())) {
            continue;
          }
          ImGui::TableNextRow();
          if (ent->is_static) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
          }
          ImGui::TableNextColumn();
          bool is_row_selected =
              this->inspector_selected_entity == ent->get_ref();
          char label[256];
          snprintf(label, sizeof(label), "%zu", idx);
          ImGui::Selectable(label, &is_row_selected,
                            ImGuiSelectableFlags_SpanAllColumns);
          if (is_row_selected) {
            this->inspector_selected_entity = ent->get_ref();
          }
          ImGui::TableNextColumn();
          ImGui::Text("%u", ent->net_id);
          ImGui::TableNextColumn();
          ImGui::Text("%s", ent->get_type_name());
          ImGui::TableNextColumn();
          ImGui::Text("%s", ent->is_static ? "static" : "");
          if (ent->is_static) {
            ImGui::PopStyleColor();
          }
        }
      }
      ImGui::EndTable();
    }

    if (inspector_selected_entity.valid(game)) {
      auto &ent = inspector_selected_entity.get_as<Entity>(game);
      ImGui::BeginChild("Inspector", ImVec2(0, 0), true);
      ent.draw_inspector_ui(game);
      ImGui::EndChild();
    }

    ImGui::End();
  }
}

void DebugGUI::draw(Game &game, std::vector<EntityRef> &entity_draw_list) {

  for (auto &entity_ref : entity_draw_list) {
    if (show_debug_overlay || entity_ref == inspector_selected_entity) {
      entity_ref.get(game).draw_debug(game);
    }
  }

  if (inspector_selected_entity.valid(game)) {
    auto &ent = inspector_selected_entity.get_as<Entity>(game);
    DrawCircleV(ent.position.to_raylib(), 25.0f, RED);
  }
}

void DebugGUI::update(Game &game) {
  // Check global keybinds
  if (IsKeyReleased(KEY_F11)) {
    show = !show;
  }
  if (IsKeyReleased(KEY_F10)) {
    show_entity_inspector = !show_entity_inspector;
  }
}
