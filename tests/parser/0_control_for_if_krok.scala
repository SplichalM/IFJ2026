import ifj26.*

@main def start(): Unit =
  for i <- 0 to 3 by if true then 1 else 2 do print(i)
