from sys import argv
from toml import loads
from os import system
import re
T = {} 
with open("challenges.toml", "r") as f:
    T = loads(f.read())

def fail(name, s):
    print(f"\033[31mFAIL : {name}\033[91;1m "+"─"*30+"\033[0m\n"+s)

def runforj(v):
    with open("challenge", "w") as f:
        f.write(v["challenge"]+" ")
    system("forjlang challenge > challengeresult")
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    with open("challengeresult", "r") as f:
        s = f.read()
        st = ansi_escape.sub('', s).strip()
        r = v['result'].strip()
        if r != st:
            return "expected:\n"+v["result"]+"\nactual:\n"+s

failed = False
if len(argv) > 1:
    s = runforj(T[argv[1]])
    if s:
        fail(argv[1], s)
        failed = True
    exit(1)
else:
    for k, v in T.items():
        s = runforj(v)
        if s:
            fail(k, s)
            failed = True
            

if not failed:
    print("\033[92;1mall pass "+"─"*30+"\033[0m")