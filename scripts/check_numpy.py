#-------------------------------------------------------------------------------
# Name:        module1
# Purpose:
#
# Author:      watson_robin
#
# Created:     13/02/2015
# Copyright:   (c) watson_robin 2015
# Licence:     <your licence>
#-------------------------------------------------------------------------------
#!/usr/bin/env python


def check_numpy(dummy):

    try:
        import numpy
        found_numpy = True
    except ImportError:
        found_numpy = False

    print "Found numpy: ",found_numpy