from _ile3 import *
import _dscdef

# wrap old intreface
def new():
    return ile3list()

def delete(il):
    del il

def size(il):
    return len(il)

def addint(il, item, item_type, item_value):
    il.append(item, item_type, item_value)

def getint(il, index):
    return il[index]

def gethex(il, index):
    return hex(il[index])

def addstr(il, item, item_value, item_length):
    if not item_value:
        il.append(item, _dscdef.DSC_K_DTYPE_T, item_length)
    else:
        il.append(item, _dscdef.DSC_K_DTYPE_T, item_value)

def addstrd(il, item, item_value:str, item_length):
    if not item_value:
        il.append(item, _dscdef.DSC_K_DTYPE_VT, item_length)
    else:
        item_value = '{:{item_length}.{item_length}}'.format(item_value, item_length = item_length)
        il.append(item, _dscdef.DSC_K_DTYPE_VT, item_value)

def addstrn(il:object, item:int, item_value:str, item_length:int):
    if not item_value:
        il.append(item, _dscdef.DSC_K_DTYPE_T, item_length)
    else:
        item_value = '{:{item_length}.{item_length}}'.format(item_value, item_length = item_length)
        il.append(item, _dscdef.DSC_K_DTYPE_T, item_value)

def getstr(il, index, flag):
    v = il[index]
    if v and flag:
        v = v[1:]
    return v

def addbin(il, item, item_value, item_offset, item_len):
    bytes_value = (item_value).to_bytes(8,'little')
    bytes_value = bytes_value[item_offset:item_offset+item_len]
    il.append(item, _dscdef.DSC_K_DTYPE_T, bytes_value)
    pass

def getbyte(il, index, item_offset):
    return il[index][item_offset]
