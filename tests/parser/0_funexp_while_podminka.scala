import ifj26.*

def identity(x: Int): Int = x
def add(a: Int, b: Int): Int = a + b
def unitValue(): Unit = ()

@main def start(): Unit =
  var x = 0
  while identity(x) < add(1, 2) do
    x = x + 1
