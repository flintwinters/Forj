from sys import argv
from toml import loads
from os import system
import re
T = {} 
with open("challenges.toml", "r") as f:
    T = loads(f.read())

def fail(name, s):
    print(f"\033[31mfj FAIL : {name}\033[91;1m "+"─"*30+"\033[0m\n"+s)

def runforj(v):
    with open("challenge", "w") as f:
        f.write(v["challenge"]+" ")
    if system("./fj challenge > challengeresult")//256 > 1:
        with open("challengeresult", "r") as f:
            s = f.read()
            return "execution returned error code"+s
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    with open("challengeresult", "r") as f:
        s = f.read()
        st = ansi_escape.sub('', s).strip()
        if 'result' not in v:
            return "No result found"
        r = v['result'].strip()
        if r != st:
            return "expected:\n"+v["result"]+"\nactual:\n"+s

def main():
    ret = 0
    if len(argv) > 1:
        if argv[1].endswith(".fj"):
            system(f"cp {argv[1]} challenge")
        else:
            with open("challenge", "w") as f:
                f.write(T[argv[1]]["challenge"]+" ")
        # system("forjlang challenge > challengeresult")
        ret = 1
    else:
        failed = False
        for k, v in T.items():
            s = runforj(v)
            if s:
                fail(k, s)
                failed = True

        if not failed:
            print("\033[92;1mfj all pass "+"─"*30+"\033[0m")
        # system("rm challenge")
        # system("rm challengeresult")
    return ret

exit(main())
