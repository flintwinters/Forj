from toml import loads
import gdb

def connect(port=1234): # arbitrary default
    T = []
    with open("debug.toml", "r") as f:
        T = loads(f.read())
    breaks = "b "+"\nb ".join(T["breakpoints"])
    displays = "display "+"\ndisplay ".join(T["displays"])
    # display /i $pc
    gdb.execute(f"""
    set sysroot
    target remote :{port}
    {displays}
    {breaks}
    c
    """)
    
connect()
