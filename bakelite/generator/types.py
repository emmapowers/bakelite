from dataclasses import dataclass
from typing import Any, List, Dict, Optional
from dataclasses_json import dataclass_json

@dataclass_json
@dataclass
class ProtoType:
  name: str
  size: Optional[int]


@dataclass_json
@dataclass
class ProtoAnnotationArg:
  name: Optional[str]
  value: Any


@dataclass_json
@dataclass
class ProtoAnnotation:
  name: str
  arguments: List[ProtoAnnotationArg]


@dataclass_json
@dataclass
class ProtoEnumValue:
  name: str
  value: Any
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class ProtoEnum:
  values: List[ProtoEnumValue]
  type: ProtoType
  name: str
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class ProtoStructMember:
  type: ProtoType
  name: str
  value: Optional[Any]
  comment: Optional[str]
  annotations: List[ProtoAnnotation]
  arraySize: Optional[int]


@dataclass_json
@dataclass
class ProtoStruct:
  members: List[ProtoStructMember]
  name: str
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class ProtoOption:
  name: str
  value: Any
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class ProtoMessageId:
  name: str
  number: int
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class Protocol:
  options: List[ProtoOption]
  message_ids: List[ProtoMessageId]
  comment: Optional[str]
  annotations: List[ProtoAnnotation]

def primitive_types():
  return [
    "bool",
    "int8", "int16", "int32", "int64",
    "uint8", "uint16", "uint32", "uint64",
    "float32", "float64",
    "bytes",
    "string",
  ]

def is_primitive(t: ProtoType) -> bool:
  return t.name in primitive_types()
