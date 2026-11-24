import ifj26.*
def factorial(n: Int): Int =
  if n <= 1 then
    return 1
  else
    val previous = n - 1
    val result = factorial(previous)
    return n * result
@main
def start(): Unit =
  val result = factorial(5)
  print(result)
