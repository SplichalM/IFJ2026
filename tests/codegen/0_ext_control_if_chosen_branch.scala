import ifj26.*

def show(x: Int): Int =
  print(x)
  return x
def add(a: Int, b: Int): Int = a + b

@main def start(): Unit =
  print(if true then show(1) else show(2))
