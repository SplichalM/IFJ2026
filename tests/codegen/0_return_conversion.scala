import ifj26.*
def value(): Double = 2
@main
def start(): Unit =
  val result = value()
  val text = toStr(result)
  print(text)
