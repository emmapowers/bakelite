import os
from lark import Lark
from lark.visitors import Transformer
from dataclasses import dataclass
from typing import Any, List, Dict

from .types import *

_g_parser = None

@dataclass
class _Comment:
  value: str

@dataclass
class _Name:
  value: str

@dataclass
class _Value:
  value: str

@dataclass
class _ProtoCommands:
  commands: List[ProtoCommand]

@dataclass
class _ProtoEvents:
  events: List[ProtoEvent]


def _filter(args, class_type):
  return [v for v in args if isinstance(v, class_type)]


def _find_one(args, class_type):
  args = _filter(args, class_type)
  if len(args) == 0:
    return None
  elif len(args) > 1:
    raise RuntimeError(f"Found more than one {class_type}")
  
  if args[0] and hasattr(args[0], 'value'):
    return args[0].value
  else:
    return args[0]

def _find_many(args, class_type):
  return _filter(args, class_type)

class TreeTransformer(Transformer):
  def argument(self, args):
    return ProtoArg(
      name=str(args[0]),
      type=args[1])

  def argument_val(self, args):
    if len(args) == 1:
      return ProtoAnnotationArg(
        name=None,
        value=str(args[0]))
    elif len(args) == 2:
      return ProtoAnnotationArg(
        name=str(args[0]),
        value=str(args[1]))
    else:
      raise RuntimeError("Argument has more than three args")
  
  def annotation(self, args):
    return ProtoAnnotation(
      name=str(args[0]),
      arguments=_find_many(args, ProtoAnnotationArg))

  def command(self, args):
    return ProtoCommand(
      return_type=_find_one(args, ProtoType),
      name=_find_one(args, _Name),
      args=_find_many(args, ProtoArg),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def comment(self, args):
    return _Comment(
      value=str(args[0]))
  
  def enum(self, args):
    return ProtoEnum(
      type=_find_one(args, ProtoType),
      name=_find_one(args, _Name),
      values=_find_many(args, ProtoEnumValue),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def enum_value(self, args):
    return ProtoEnumValue(
      name=_find_one(args, _Name),
      value=_find_one(args, _Value),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def event(self, args):
    return ProtoEvent(
      name=_find_one(args, _Name),
      args=_find_many(args, ProtoArg),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def name(self, args):
    return _Name(
      value=str(args[0]))

  def prim_sized(self, args):
    return ProtoType(
      name=str(args[0]),
      size=int(args[1]),
      array=False)
  
  def prim_unsized(self, args):
    return ProtoType(
      name=str(args[0]),
      size=0,
      array=False)

  def proto(self, args):
    commands = _find_one(args, _ProtoCommands)
    events = _find_one(args, _ProtoEvents)

    return Protocol(
      options=_find_many(args, ProtoOption),
      commands= commands.commands if commands else [],
      events= events.events if events else [],
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def proto_commands(self, args):
    return _ProtoCommands(
      commands=_find_many(args, ProtoCommand))

  def proto_events(self, args):
    return _ProtoEvents(
      events=_find_many(args, ProtoEvent))

  def proto_member(self, args):
    return ProtoOption(
      name=_find_one(args, _Name),
      value=_find_one(args, _Value),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def struct(self, args):
    return ProtoStruct(
      name=_find_one(args, _Name),
      members=_find_many(args, ProtoStructMember),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def struct_member(self, args):
    return ProtoStructMember(
      name=_find_one(args, _Name),
      type=_find_one(args, ProtoType),
      value=_find_one(args, _Value),
      comment=_find_one(args, _Comment),
      annotations=_find_many(args, ProtoAnnotation))

  def value(self, args):
    return _Value(
      value=str(args[0]))
  
  def type(self, args):
    if str(args[0]) in primitive_types():
      if len(args) == 2:
        return self.prim_sized(args)
      else:
        return self.prim_unsized(args)

    if len(args) == 1:
      return ProtoType(
        name=str(args[0]),
        size=None,
        array=False)
    else:
      return ProtoType(
        name=str(args[0]),
        size=int(args[1]),
        array=True)

def parse(text: str):
  global _g_parser

  if not _g_parser:
    with open(f"{os.path.dirname(__file__)}/protodef.lark") as f:
      grammar = f.read()
    
    _g_parser = Lark(grammar)

  tree = _g_parser.parse(text)
  tree = TreeTransformer().transform(tree)

  items = list(tree.iter_subtrees_topdown())[0].children

  enums = _find_many(items, ProtoEnum)
  structs = _find_many(items, ProtoStruct)
  protocol = _find_one(items, Protocol)
  comments = [comment.value for comment in _find_many(items, _Comment)]

  return (enums, structs, protocol, comments)