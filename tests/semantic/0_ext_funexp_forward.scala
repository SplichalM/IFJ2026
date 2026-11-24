import ifj26.*

def result(): Int = later(2) + 1
def later(x: Int): Int = x * 2

@main def start(): Unit =
  print(result())
