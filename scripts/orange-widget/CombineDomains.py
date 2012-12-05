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

