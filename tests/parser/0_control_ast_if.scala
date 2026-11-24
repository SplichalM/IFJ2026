import ifj26.*

@main def start(): Unit =
  val a = if true then 1 else 2
  val b = if false then
    3
  else
    4
  val c = (if true then 5 else 6) * 2
  val d = if true then 7 else if false then 8 else 9
  print(b)
