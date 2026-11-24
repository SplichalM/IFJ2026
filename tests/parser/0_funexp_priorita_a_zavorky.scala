import ifj26.*

def identity(x: Int): Int = x
def add(a: Int, b: Int): Int = a + b
def unitValue(): Unit = ()

@main def start(): Unit =
  val x = 2 * identity((3 + 4) * 5) + 1
  print(x)
