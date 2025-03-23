from collections import defaultdict
from typing import List, Optional, TextIO, Protocol, runtime_checkable
from .templating import jinja2_env
from dataclasses import dataclass
from collections import OrderedDict

COMPONENT_DIRECTORY: dict[str, List[type]] = defaultdict(list)

def EcsComponent(namespace: str):
    """
    Decorator used to generate ECS boilerplate from a Python class.
    """
    def decorator(cls: type):
        COMPONENT_DIRECTORY[namespace].append(cls)
        iter_class_fields(cls)
        iter_class_methods(cls)
        return cls
    return decorator



@runtime_checkable
class ICppType(Protocol):
    def header_includes(self) -> List[str]:
        ...
    def gen_member_spec(self, name:str) -> str:
        ...

class PrimitiveType(ICppType):
    def __init__(self, cpp_name: str):
        self.cpp_name = cpp_name
    def header_includes(self) -> List[str]:
        return []
    def gen_member_spec(self, name:str) -> str:
        return f"{self.cpp_name} {name};"
    


cpp_int = PrimitiveType("int")
cpp_float = PrimitiveType("float")
cpp_unsigned_int = PrimitiveType("unsigned int")
cpp_bool = PrimitiveType("bool")
cpp_void = PrimitiveType("void")


PRIMITIVE_MAP = {
    int: cpp_int,
    float: cpp_float,
    bool: cpp_bool,
    None: cpp_void
}

@dataclass
class CppClassFieldInfo:
    name: str
    cpp_type: ICppType
    doc_comment: str = ""

    @property
    def member_spec(self) -> str:
        print("member_spec", self.cpp_type)
        return self.cpp_type.gen_member_spec(self.name)
    
@dataclass
class CppBehaviorMethodInfo:
    name: str
    return_type: ICppType
    args: OrderedDict[str, ICppType]
    doc_comment: str = ""

    is_loop_method: bool = False

    @property 
    def behavior_spec(self) -> str:
        return 



def behavior_method(signature: Optional[CppBehaviorMethodInfo] = None):
    def decorator(fn):
        if signature:
            fn._ecs_behavior_method = signature
        else:
            raise NotImplementedError("Signature autogeneration not yet.")
        return fn
    return decorator

def iter_class_fields(cls: type):
    if '_ecs_component_fields' not in cls.__dict__:
        cls._ecs_component_fields = []
    for name, field in cls.__annotations__.items():
        cpp_type = field
        if field in PRIMITIVE_MAP:
            cpp_type = PRIMITIVE_MAP[field]


        print("cpp_type", cpp_type)
        if isinstance(cpp_type, ICppType):
            cls._ecs_component_fields.append(CppClassFieldInfo(
                name=name,
                cpp_type=cpp_type,
              
            ))

def iter_class_methods(cls: type):
    if '_ecs_behavior_methods' not in cls.__dict__:
        cls._ecs_behavior_methods = []
    for name, method in cls.__dict__.items():
        if hasattr(method, "_ecs_behavior_method"):
            cls._ecs_behavior_methods.append(method)
        

def generate_header_file(output_file: TextIO):
    output_file.write(jinja2_env.get_template("header.tpl.h").render(
        components=COMPONENT_DIRECTORY  
    ))
