import sys

def greet():
    i = XsInteger(101)
    #print globals()
    #print sys.path[0]
    return '%s % d' % (sys.path[0], i.get_integer())
