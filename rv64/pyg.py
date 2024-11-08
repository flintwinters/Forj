from toml import loads
from sys import argv
import gdb

# https://stackoverflow.com/questions/2290842/gdb-python-scripting-where-has-parse-and-eval-gone
def parseandeval(s):
    gdb.execute("set logging redirect on")
    gdb.execute("set logging enabled on")
    gdb.execute("print " + s)
    gdb.execute("set logging enabled off")
    return gdb.history(0)

def connect(challenge="default"): # arbitrary default
    port = 1234
    T = []
    with open("debug.toml", "r") as f:
        T = loads(f.read())
    if challenge:
        T = T[challenge]
        T["runs"] += "\nexit"
    else:
        T = T["default"]
    def buildcmds(s, l):
        return s + " " + ("\n"+s+" ").join(l)
    breaks   = buildcmds("b ", T["breakpoints"])
    displays = buildcmds("display ", T["displays"])
    gdb.execute(f"""
    set confirm off
    set logging overwrite
    set logging debugredirect
    set logging enabled off
    set logging enabled on
    set sysroot
    target remote :{port}
    {displays}
    {breaks}
    c
    {T["runs"]}
    """)
