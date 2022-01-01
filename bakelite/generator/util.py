def to_camel_case(name, first_capital=True):
  next_upper = False
  result = ""

  for c in name:
    if c == '_':
      next_upper = True
      continue

    if next_upper:
      c = c.upper()
      next_upper = False

    result += c

  if first_capital:
    result = result[0].upper() + result[1:]
  else:
    result = result[0].lower() + result[1:]

  return result
