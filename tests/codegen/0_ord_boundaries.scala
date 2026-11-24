import ifj26.*
@main
def start(): Unit =
  val a = ord("abc", 0)
  val b = ord("abc", 2)
  val c = ord("abc", 3)
  val negative = 0 - 1
  val d = ord("abc", negative)
  val e = ord("", 0)
  print(a)
  print("|")
  print(b)
  print("|")
  print(c)
  print("|")
  print(d)
  print("|")
  print(e)
