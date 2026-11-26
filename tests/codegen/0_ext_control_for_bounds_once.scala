import ifj26.*

def show(x: Int): Int =
  print(x)
  return x
def add(a: Int, b: Int): Int = a + b

@main def start(): Unit =
  for i <- show(1) to show(3) by show(1) do print(i)
