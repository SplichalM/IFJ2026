import ifj26.*

def identity(x: Int): Int = x
def add(a: Int, b: Int): Int = a + b
def unitValue(): Unit = ()

@main def start(): Unit =
  val x = -identity(3) + add(-1, -(2 + 3))
  print(x)
