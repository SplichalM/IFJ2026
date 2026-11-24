import ifj26.*

@main def start(): Unit =
  for i <- 0 to if true then 3 else 4 by 1 if i > 0 do print(i)
