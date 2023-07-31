from _sys import *

_crelnm = crelnm
_trnlnm = trnlnm
_dellnm = dellnm
_getjpi = getjpi
_getlki = getlki
_getsyi = getsyi
_schdwk = schdwk

def crelnm(attr, tabnam, lognam, acmode, il):
    return _crelnm(lognam, tabnam, il, acmode, attr)

def trnlnm(attr, tabnam, lognam, acmode, il):
    return _trnlnm(lognam, tabnam, il, acmode, attr)

def dellnm(tabnam, lognam, acmode):
    return _dellnm(lognam, tabnam, acmode)

def getjpi(pid, pnam, item_list):
    return _getjpi(item_list, pnam or pid)

def getlki(lki, item_list):
    return _getlki(item_list, lki)

def getsyi(csid, node_name, item_list):
    return _getsyi(item_list, node_name or csid)

def schdwk(pid, pname, time, reptim = 0):
    return _schdwk(time, pname or pid, reptim)
