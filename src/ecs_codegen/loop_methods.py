"""
Contains method signatures for behaviour methods which are called during the game loop.
"""

from ecs_codegen.cpp_types import CppBehaviorMethodInfo
from ecs_codegen.domain_types import cpp_GameRef
from collections import OrderedDict

update = CppBehaviorMethodInfo(
    name="update",
    return_type=None,
    args=[cpp_GameRef],
    is_loop_method=True
)



LOOP_METHODS = [
    update
]
