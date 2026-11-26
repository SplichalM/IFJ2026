import ifj26.*

def select(flag: Boolean) = if flag then 1 else 2

@main def start(): Unit =
  print(select(true))
