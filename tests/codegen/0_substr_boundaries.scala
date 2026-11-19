import ifj26.*
@main
def start(): Unit =
  val all = subStr("abc", 0, 3)
  val empty = subStr("abc", 1, 1)
  val tail = subStr("abc", 2, 3)
  print(all)
  print("|")
  print(empty)
  print("|")
  print(tail)
