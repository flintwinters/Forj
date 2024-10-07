import gdb

def connect(port=1234): # arbitrary default
    gdb.execute(f"""
    set sysroot
    target remote :{port}
    b _start
    continue
    """)
    
connect()
