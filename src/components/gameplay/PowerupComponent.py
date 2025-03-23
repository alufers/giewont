from ecs_codegen.cpp_types import EcsComponent, behavior_method
from ecs_codegen.loop_methods import update

@EcsComponent("giewont")
class PowerupComponent:
    heal_amount: int
    lifetime_left: int
    lifetime_total: int

    @behavior_method(signature=update)
    def update(self):
        pass
