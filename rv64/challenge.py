from sys import argv
from toml import loads
from os import system
import re
T = {} 
with open("debug.toml", "r") as f:
    T = loads(f.read())

def rungdb(challenge="default"):
    sys = system(f"""cd .. && make CHALLENGE={challenge}""")
    if sys: return
    with open("gdb.txt", "r") as f:
        return f.read()

def fail(name, s):
    print(f"\033[31mFAIL : {name}\033[91;1m "+"─"*30+"\033[0m\n")

def runos(v):
    result = rungdb()
    if not result:
        return
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    with open("gdb.txt", "r") as f:
        s = f.read()
        st = ansi_escape.sub('', s).strip()
        r = v['result'].strip()
        if r not in st:
            return "expected:\n"+v["result"]+"\nactual:\n"+s

def main():
    ret = 0
    failed = False
    for k, v in T.items():
        s = runos(v)
        if s:
            fail(k, s)
            failed = True

    if not failed:
        print("\033[92;1mall pass "+"─"*30+"\033[0m")
    return ret

exit(main())