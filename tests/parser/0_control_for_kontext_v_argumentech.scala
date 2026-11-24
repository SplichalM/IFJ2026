import ifj26.*

def add(a: Int, b: Int): Int = a + b
def limit(value: Int): Int = value + 5
def step(value: Int): Int = value + 1

@main def start(): Unit =
  val to = 0
  val until = 2
  val by = 2
  for i <- add(to, until) to limit(by) by step(to) do print(i)
