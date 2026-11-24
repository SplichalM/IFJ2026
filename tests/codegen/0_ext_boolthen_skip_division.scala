import ifj26.*

@main def start(): Unit =
  print(false && (1 / 0 == 0))
  print("|")
  print(true || (1 / 0 == 0))
