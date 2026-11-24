import ifj26.*

def invert(flag: Boolean): Boolean = !flag

@main def start(): Unit =
  val flag: Boolean = invert(false)
  print(toStr(flag))
