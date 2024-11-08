from toml import loads
import gdb

# https://stackoverflow.com/questions/2290842/gdb-python-scripting-where-has-parse-and-eval-gone
def parseandeval(s):
    gdb.execute("set logging redirect on")
    gdb.execute("set logging enabled on")
    gdb.execute("print " + s)
    gdb.execute("set logging enabled off")
    return gdb.history(0)

def connect(): # arbitrary default
    T = []
    with open("debug.toml", "r") as f:
        T = loads(f.read())
    def buildcmds(s, l):
        if l: return s + " " + ("\n"+s+" ").join(l)
        return ""
    breaks   = buildcmds("b ", T["breakpoints"])
    displays = buildcmds("display ", T["displays"])
    gdb.execute(f"""
    set confirm off
    exec-file ./fj
    b main
    run
    d
    {breaks}
    run
    {displays}
    """)
    
connect()
