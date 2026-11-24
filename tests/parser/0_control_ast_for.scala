import ifj26.*

@main def start(): Unit =
  for i <- 1 + 2 until readInt() by -2 if i > 0 if i < 8 do print(i)
  var x = 0
  for
    j <- 0 to 3
  do x = x + j
  print(x)
