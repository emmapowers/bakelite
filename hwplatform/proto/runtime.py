from typing import Any, Dict
from enum import Enum
from dataclasses import is_dataclass

class Registry:
  def __init__(self):
    self.types: Dict[str, Any] = {}
  
  def register(self, name: str, cls: Any) -> None:
    self.types[name] = cls
  
  def get(self, name: str):
    return self.types[name]

  def is_enum(self, name: str):
    return issubclass(self.types[name], Enum)
  def is_struct(self, name: str):
    return is_dataclass(self.types[name])
