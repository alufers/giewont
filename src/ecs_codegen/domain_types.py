
from typing import List
from ecs_codegen.cpp_types import ICppType


class _DomainRefOnlyType(ICppType):
    def __init__(self, cpp_name: str, cpp_include: str):
        self.cpp_name = cpp_name
        self.cpp_include = cpp_include
    def header_includes(self) -> List[str]:
        return [self.cpp_include]
    def gen_member_spec(self, name:str) -> str:
        raise RuntimeError(f"_DomainRefType {self.cpp_name} cannot be used as a class member.")
    def gen_arg_spec(self, name:str) -> str:
        return f"{self.cpp_name} {name}"

cpp_GameRef = _DomainRefOnlyType("Game", "Game.h")
