import ifj26.*

@main def start(): Unit =
  val a = true || false && !false
  val b = !true == false
  val c = 1 < 2 && 3 != 4
  val d = true && false && true
  val e = !(true || false)
  if true then
    ()
  else if false then
    ()
  if false then
    ()
  print(true)
