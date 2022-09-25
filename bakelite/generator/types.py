from dataclasses import dataclass
from typing import Any, List, Optional

from dataclasses_json import DataClassJsonMixin


@dataclass
class ProtoType(DataClassJsonMixin):
  name: str
  size: Optional[int]


@dataclass
class ProtoAnnotationArg(DataClassJsonMixin):
  name: Optional[str]
  value: Any


@dataclass
class ProtoAnnotation(DataClassJsonMixin):
  name: str
  arguments: List[ProtoAnnotationArg]


@dataclass
class ProtoEnumValue(DataClassJsonMixin):
  name: str
  value: Any
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass
class ProtoEnum(DataClassJsonMixin):
  values: List[ProtoEnumValue]
  type: ProtoType
  name: str
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass
class ProtoStructMember(DataClassJsonMixin):
  type: ProtoType
  name: str
  value: Optional[Any]
  comment: Optional[str]
  annotations: List[ProtoAnnotation]
  arraySize: Optional[int]


@dataclass
class ProtoStruct(DataClassJsonMixin):
  members: List[ProtoStructMember]
  name: str
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass
class ProtoOption(DataClassJsonMixin):
  name: str
  value: Any
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass
class ProtoMessageId(DataClassJsonMixin):
  name: str
  number: int
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass
class Protocol(DataClassJsonMixin):
  options: List[ProtoOption]
  message_ids: List[ProtoMessageId]
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


def primitive_types() -> List[str]:
  return [
      "bool",
      "int8",
      "int16",
      "int32",
      "int64",
      "uint8",
      "uint16",
      "uint32",
      "uint64",
      "float32",
      "float64",
      "bytes",
      "string",
  ]


def is_primitive(t: ProtoType) -> bool:
  return t.name in primitive_types()
