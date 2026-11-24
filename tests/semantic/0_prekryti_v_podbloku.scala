import ifj26.*
@main def start(): Unit =
  val x = 1
  if x == 1 then
    val x = "inner"
    print(x)
  else
    ()
  print(x)
