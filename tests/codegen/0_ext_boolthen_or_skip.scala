import ifj26.*

def mark(x: Int): Boolean =
  print(x)
  return true

@main def start(): Unit =
  print(true || mark(1))
