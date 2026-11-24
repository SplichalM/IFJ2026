import ifj26.*

def identity(x: Int): Int = x
def add(a: Int, b: Int): Int = a + b
def unitValue(): Unit = ()

@main def start(): Unit =
  print(toStr(1.5 + 2.5))
  print("a" + "b")
