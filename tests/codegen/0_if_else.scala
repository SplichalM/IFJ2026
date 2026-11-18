import ifj26.*
def show(n: Int): Unit =
  if n <= 0 then
    print("L")
  else
    print("R")

@main
def start(): Unit =
  show(0)
  show(1)
