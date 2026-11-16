import ifj26.*
def count(n: Int): Int =
  if n <= 0 then
    return 0
  else
    val prev = n - 1
    return count(prev)
@main def start(): Unit = ()
