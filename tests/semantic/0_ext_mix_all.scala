import ifj26.*

def check(x: Int): Boolean = x > 0

@main def start(): Unit =
  for i <- 0 to 3 if check(i) do print((if check(i) then "yes" else "no").length)
