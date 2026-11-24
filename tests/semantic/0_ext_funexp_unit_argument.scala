import ifj26.*

def identity(x: Unit): Unit = x
def unitValue(): Unit = ()

@main def start(): Unit =
  print(identity(unitValue()))
