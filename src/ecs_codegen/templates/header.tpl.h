#pragma once

{%- for namespace, component_classes in components.items() %}
namespace {{ namespace }} {
{%- for component_class in component_classes %}
class {{ component_class.__name__ }} {
public:
    {%- for field_info in component_class._ecs_component_fields %}
        {{ field_info.member_spec }}
    {%- endfor %}
};

class {{ component_class.__name__ }}_Behavior {
    public:
    {%- for method_info in component_class._ecs_behavior_methods %}
        {{ method_info.behavior_spec }}
    {%- endfor %}
};

{%- endfor %}
} // namespace {{ namespace }}
{%- endfor %}

