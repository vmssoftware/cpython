from _decc import *

import os

def fopen(filename, mode, *opts):
    flags = 0
    _readable = False
    _writable = False
    if 'x' in mode:
        _writable = True
        flags = os.O_EXCL | os.O_CREAT
    elif 'r' in mode:
        _readable = True
        flags = 0
    elif 'w' in mode:
        _writable = True
        flags = os.O_CREAT | os.O_TRUNC
    elif 'a' in mode:
        _writable = True
        flags = os.O_APPEND | os.O_CREAT

    if '+' in mode:
        _readable = True
        _writable = True

    if _readable and _writable:
        flags |= os.O_RDWR
    elif _readable:
        flags |= os.O_RDONLY
    else:
        flags |= os.O_WRONLY
    fd = os.open(filename, flags, rms=opts)
    if fd == -1:
        return None
    return open(fd, mode)

def fclose(ff):
    ff.close()
    return 0

def fileno(ff):
    return ff.fileno()

def write(fd, data):
    return os.write(fd, data)

def fgets(ff, maxbytes):
    line = ff.readline()
    if not line:
        return None
    return line

def feof(ff):
    return ff.tell() == os.fstat(ff.fileno()).st_size

def ferror(ff):
    return False
