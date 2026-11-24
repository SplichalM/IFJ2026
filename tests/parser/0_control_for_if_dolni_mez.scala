import ifj26.*

@main def start(): Unit =
  for i <- if true then 0 else 1 to 3 do print(i)
