'''
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 '''

import orange
import itertools

def CombineDomains(domain1,domain2):
    print 'combining domains'
    num_att1 = len(domain1.attributes)
    num_att2 = len(domain2.attributes)
    attlist=[]
    for i in range(num_att1):
        attlist.append(domain1.attributes[i])
    for i in range(num_att2):
        attlist.append(domain2.attributes[i])        
    domain_combined = orange.Domain(attlist,0) 
    return domain_combined

#              values headings
def MakeDomain(dat,thevalues):
    enumvarlist=[]
    for i in range(len(dat)):
        enumvarlist.append(str(dat[i][1]))
    #enumvarlist.append("nullclassvalue")
    v=[orange.EnumVariable(x,values = thevalues) for x in enumvarlist]    
    d = orange.Domain(v,0) #,0 means class-less
    #d = orange.Domain(v) #,0 means class-less
    return d

