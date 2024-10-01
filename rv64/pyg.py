import gdb

def connect(port=2222): # arbitrary default
    gdb.execute(f"""
    set sysroot
    target remote :{port}
    """)
    
connect()