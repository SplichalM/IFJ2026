import ifj26.*

@main def start(): Unit =
  for
    i <- 0 to 10 by 2
    if i != 3
    if i < 8
  do
    print(i)
