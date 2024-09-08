#ifndef DATABINDER_H_
#define DATABINDER_H_

#include "Game.h"
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace giewont {

enum class DataBinderType {
  STRING,
  FLOAT,
  INT,
  VEC2,
  COLOR
};

class DataBinderBase;

class DataBinderField {
public:
  DataBinderType type;
  std::string name;
  size_t offset;

  float min = 0.0f;
  float max = 100.0f;

  DataBinderField(std::string typeName, std::string name, size_t offset);

  DataBinderField(std::string typeName, std::string name, size_t offset,
                  float min, float max);
};

class DataBinderBase {
public:
  std::vector<DataBinderField> fields;

protected:
  void assign_data_from_json_impl(void *instance, const nlohmann::json &data);
  nlohmann::json get_json_from_data_impl(const void *instance);
  void draw_inspector_ui_impl(void *instance);
};

/**
 * @brief Bind entity data to JSON fields, inspector fiels.
 *
 */
template <typename T> class DataBinder : public DataBinderBase {

public:
  DataBinder(std::initializer_list<DataBinderField> fields) {
    for (auto &field : fields) {
      this->fields.push_back(field);
    }
  }

  void assign_data_from_json(T &instance, const nlohmann::json &data) {
    assign_data_from_json_impl(&instance, data);
  }
  nlohmann::json get_json_from_data(const T &instance) {
    return get_json_from_data_impl(&instance);
  }
  void draw_inspector_ui(T &instance) { draw_inspector_ui_impl(&instance); }
};

#define GW_DATABINDER_DECLARE(klass) static DataBinder<klass> databinder;

#define GW_DATABINDER_DEFINE(klass, ...)                                       \
  using gw_databinder_klass = klass;                                           \
  DataBinder<klass> klass::databinder = {__VA_ARGS__}

#define GW_DATABINDER_FIELD(type, name)                                        \
  DataBinderField(#type, #name, offsetof(gw_databinder_klass, name))

#define GW_DATABINDER_FIELD_RANGE(type, name, min, max)                        \
    DataBinderField(#type, #name, offsetof(gw_databinder_klass, name), min, max)

#define GW_DATABINDER_AUTO_INSPECTOR(klass)                                    \
  void draw_inspector_ui(Game &game) override {                                \
    databinder.draw_inspector_ui(*this);                                       \
  }

class DemoClass {

public:
  GW_DATABINDER_DECLARE(DemoClass);
};

} // namespace giewont
#endif // DATABINDER_H_
