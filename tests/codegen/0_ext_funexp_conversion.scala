import ifj26.*

def twice(x: Double): Double = x * 2.0

@main def start(): Unit =
  print(toStr(twice(1 + 2)))
