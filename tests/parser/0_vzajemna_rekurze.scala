import ifj26.*
def first(n: Int): Int =
  return second(n)
def second(n: Int): Int =
  if n == 0 then
    return 0
  else
    val prev = n - 1
    return first(prev)
@main def start(): Unit = ()
