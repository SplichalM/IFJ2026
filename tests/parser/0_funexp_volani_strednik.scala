import ifj26.*

def identity(x: Int): Int = x
def add(a: Int, b: Int): Int = a + b
def unitValue(): Unit = ()

@main def start(): Unit =
  identity(1) + 2; print(identity(3))
