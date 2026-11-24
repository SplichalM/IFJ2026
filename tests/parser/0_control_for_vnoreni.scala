import ifj26.*

@main def start(): Unit =
  for i <- 0 to 3 do
    for j <- 0 until i do print(j)
  print(9)
