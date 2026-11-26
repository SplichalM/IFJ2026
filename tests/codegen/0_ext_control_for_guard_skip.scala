import ifj26.*

def mark(x: Int): Boolean =
  print(x)
  return true

@main def start(): Unit =
  for i <- 0 to 2 if false if mark(i) do print(i)
  print("end")
