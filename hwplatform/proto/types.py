from dataclasses import dataclass
from typing import Any, List, Dict, Optional
from dataclasses_json import dataclass_json

@dataclass_json
@dataclass
class ProtoType:
  name: str
  size: Optional[int]
  array: bool


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
class ProtoArg:
  name: str
  type: ProtoType


@dataclass_json
@dataclass
class ProtoCommand:
  name: str
  args: List[ProtoArg]
  return_type: ProtoType
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class ProtoEvent:
  name: str
  args: List[ProtoArg]
  comment: Optional[str]
  annotations: List[ProtoAnnotation]


@dataclass_json
@dataclass
class Protocol:
  options: Dict[str, ProtoOption]
  commands: List[ProtoCommand]
  events: List[ProtoEvent]
  comment: Optional[str]
  annotations: List[ProtoAnnotation]
