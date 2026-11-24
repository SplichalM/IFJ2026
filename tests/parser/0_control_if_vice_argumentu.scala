import ifj26.*

def add(a: Int, b: Int): Int = a + b

@main def start(): Unit =
  print(add(if true then 1 else 2, if false then 3 else 4))
