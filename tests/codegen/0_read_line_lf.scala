import ifj26.*
@main
def start(): Unit =
  val line = readLine()
  val length = strLength(line)
  val last = ord(line, 3)
  print(length)
  print("|")
  print(last)
