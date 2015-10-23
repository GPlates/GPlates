#Given a directory consisting of coregistration csv files, create an Orange ExampleTable corresponding to a chosen attribute index for each time, ordered
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

#import LoadCoregFiles
#data=LoadCoregFiles.LoadCoregFiles('Z:/Work/rejection/tomworkoz/unisydgeo/exp/coreg/palaeogeog_ozmin2/test','take2')

#import sys
#sys.path.append('/Applications/Orange.app/Contents/Resources/orange')
#import orange
#coreg_dir='/Users/thomas/Downloads/py/ozmin_exp3'
#data=LoadCoregFiles.LoadCoregFiles(self.coregdir,'')


import os, re, orange
import itertools, csv

def LoadCoregFileSelectTime(coreg_dirnm,coreg_prefix='co_registration',select_time = 0.00,delimiter=',',undef_string = 'NaN'):
 gplatesid_ind = 0;
 nmlist = []
 print 'starting'
 for name in os.listdir(coreg_dirnm):
     path = os.path.join(coreg_dirnm,name)

     if os.path.isfile(path):
         ind = path.find(coreg_prefix)
         if ind != -1:
             print path
             nmlist.append(path)

 nmlist_ordered = ComputeOrderedList(nmlist)
 print nmlist_ordered
 found_time = 0
 ind_time=0
 for i in range(len(nmlist_ordered)):
     print nmlist_ordered[i][1]
     if nmlist_ordered[i][1] == select_time:
         found_time = 1
         ind_time = i
         print 'found'
 #dataset = []
 if found_time == 1:
     print 'Found data at specified time'
 else:
     print 'Data at specified time not found'
     data = orange.ExampleTable(orange.Domain([]))
     return data

 print nmlist_ordered[ind_time][0]

 #data = orange.ExampleTable(nmlist_ordered[ind_time][0])
 attrib_vals_list = list([])
 dat=list(csv.reader(open(nmlist_ordered[ind_time][0],'rU'), delimiter=','))
 theid=[list()]

 dat[0].append("theid")
 for i in range(len(dat)):
     if(i>0):
         dat[i].append(str(i-1))

 vsto=[]
 print 'c'
 #calc num attributes minus the first (the gplates id)

 numattrib = len(dat[0])###len(dat[0])-1
 print numattrib
 
 #enumvarlist=list([])
 #for attrib_ind in range(numattrib):
 #    enumvarlist.append(str(attrib_ind))

 #First line of coreg data gives the attribute name
 enumvarlist=list([])
 for attrib_ind in range(numattrib):
     enumvarlist.append(str(dat[0][attrib_ind]))
 
 print 'b'
 
 for attrib_ind in range(numattrib):
     print attrib_ind
     tmporgdat=[]
     
     cnt=0 #make a counter to skip the first row
     for item in dat: 
         ###tmporgdat.append(str(item[attrib_ind+1]))
         if(cnt > 0):
             tmporgdat.append(str(item[attrib_ind]))
         cnt=cnt+1
     #print tmporgdat
     attrib_values = ComputeUniqueAttribValues2(tmporgdat)
     attrib_values.append(undef_string)
     #print '1'
     print attrib_values
     attrib_vals_list.append(attrib_values)
     #print '2'
     #print enumvarlist
     v=[orange.EnumVariable(x,values=attrib_values) for x in enumvarlist]
     #print v
     vsto.append(v[attrib_ind])
     #vsto = vsto.append([orange.EnumVariable(x,values = attrib_values) for x in enumvarlist])
     #print '3'
     #print vsto
 d = orange.Domain(vsto,0)
 data = orange.ExampleTable(d)

 print numattrib
 for i in range(len(dat)):#skip the first row
     if(i > 0):
         exdata=[]
         for j in range(numattrib):
             tmp = dat[i][j]#skip the first row
 ###         tmp = dat[i][j+1]
             exdata.append(tmp)
             #print exdata
         ex=orange.Example(d,exdata)
         data.append(ex)     
 print 'Loaded!'
 
 return data  


#given a unique list of ids (gplates_ids), and a single candidate id (the_id),
# return the matched index in the list
def GetSeedIndex(the_id,gplates_ids):
    seed_index = -1
    for i in range(len(gplates_ids)):
        if the_id == gplates_ids[i]:
            seed_index = i
            return seed_index
    return seed_index

#given a multiple variable name list structure tdat, find the unique set of variables
# - this can be a big calc, but is the only way to reliably get all categories
def ComputeUniqueAttribValues(tdat):
    unique_values=[]
    for i in range(len(tdat)):
        tmpu_values=[k for k, v in itertools.groupby(sorted(tdat[i]))]
        tmpu_values_split = []
        for j in range(len(tmpu_values)):
            #tmpstr = str.split(tmpu_values[j])[0]
            #tmpu_values_split.append(tmpstr)
            tmpu_values_split.append(tmpu_values[j])
        unique_values.extend(tmpu_values_split)
    unique_values = [k for k, v in itertools.groupby(sorted(unique_values))]        
    return unique_values

#simple unique on a list
def ComputeUniqueAttribValues2(tdat):
    unique_values=[k for k, v in itertools.groupby(sorted(tdat))]        
    return unique_values


#Assume pattern xxx_xxxxxx_xxx...._xxxxxx_time.tab
def ComputeOrderedList(thelist):
    coreg_extension_string = '.csv'
    #newlist=[]
    #ind=[]
    list_tuple = []
    for entry in range(len(thelist)):
        #find all the underscores
        tmpstr = thelist[entry]
        numund=len(re.findall('_',tmpstr))
        #newlist.append(tmpstr)
        lim = 0
        
        for undind in range(numund):
            lim = tmpstr.find('_',lim) + 1
        plim = tmpstr.find(coreg_extension_string)
        print tmpstr[lim:plim]
        #ind.append(int(tmpstr[lim:plim]))
        #list_tuple.append((tmpstr,int(tmpstr[lim:plim])))
        list_tuple.append((tmpstr,float(tmpstr[lim:plim])))

        #sort reverse by second tuple element
        list_tuple_sorted = sorted(list_tuple, key=lambda oink: oink[1],reverse = True)
    return list_tuple_sorted

#Create domain - i.e. attributes with time-headings, no class
#dat is the ordered nmlist, where second dim is time
#values are the possible values you can get e.g. for a property
# callled 'car' we could have values ["bmw","vw","audi"]
def MakeDomain(dat,thevalues):
    enumvarlist=[]
    for i in range(len(dat)):
        enumvarlist.append(str(dat[i][1]))
    #enumvarlist.append("nullclassvalue")
    v=[orange.EnumVariable(x,values = thevalues) for x in enumvarlist]    
    d = orange.Domain(v,0) #,0 means class-less
    #d = orange.Domain(v) #,0 means class-less
    return d

#if __name__ == '__main__':
#    print LoadCoregFiles( 
#LoadCoregFiles.LoadCoregFiles('Z:/Work/rejection/tomworkoz/unisydgeo/exp/coreg/palaeogeog_ozmin2/ozmin_ex    

