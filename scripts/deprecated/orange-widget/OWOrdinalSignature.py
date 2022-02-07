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

"""
<name>OrdinalSignature</name>
<description>Given an orange data table encoded as a time series, compute an ordinal signature, which is a sequence list of  single variable time-series without class, compute a new table where classes are represented as the first valid variable per example.</description>
<icon>icons/ordinal_signature.png</icon>
<priority>205</priority>
<contact>Thomas Landgrebe (thomas.landgrebe(@at@)sydney.edu.au)</contact>
"""

from OWWidget import *
import OWGUI, string


class OWOrdinalSignature(OWWidget):
    settingsList = ['undef_string']
    def __init__(self, parent=None, signalManager=None):
        OWWidget.__init__(self, parent, signalManager, 'SampleDataB')

        self.inputs = [("Data", ExampleTable, self.data)]
        self.outputs = [("OrdinalSignature Data", ExampleTable)]

        self.commitOnChange = 0
        self.loadSettings()

        # GUI
        box = OWGUI.widgetBox(self.controlArea, "Info")
        self.infoa = OWGUI.widgetLabel(box, 'No data on input yet, waiting to get something.')
        self.infob = OWGUI.widgetLabel(box, '')

        self.resize(100,50)

    def data(self, dataset):
        if dataset:
            self.dataset = dataset
            self.infoa.setText('%d instances in input data set' % len(dataset))
            attrib_name = ['BirthAttrib_' + str(dataset.domain[0].values[0])]
            #self.datawc = dataset#ComputeAttributeAtBirth.ComputeAttributeAtBirth(dataset,str(attrib_name))
            self.datawc = self.ComputeOrdinalSignature2(dataset)
            self.send("OrdinalSignature Data",self.datawc)
            #self.optionsBox.setDisabled(0)
            #self.selection()
            #self.commit()
        else:
            self.send("OrdinalSignature Data", None)
            #self.optionsBox.setDisabled(1)
            self.infoa.setText('No data on input yet, waiting to get something.')
            self.infob.setText('')

    def ComputeOrdinalSignature(self,the_data):
        
        #the_data_o = the_data
        attrib_values=the_data[0].domain[0].values
        
        #the_data_o = orange.ExampleTable(thed)
        the_data_o = the_data.clone()
        #for i in range(len(the_data_o)):
        #    for j in range(len(the_data_o[i])):
        #        the_data_o[i][j]='NaN'
        for i in range(len(the_data)):
            ex = the_data[i];
            exl=[];
            for k in range(len(ex)):
                exl.append(ex[k].value)
            exlo = self.RemoveAdjacent(exl)
            #print exlo
            lastind = 0
            for k in range(len(exlo)):
                the_data_o[i][k]=exlo[k]
                lastind = k
            if lastind < len(the_data[0]):
                for k in range(lastind+1,len(the_data_o[i])):
                    the_data_o[i][k] = 'NaN'
        d = the_data_o.domain
        for i in range(len(d)):
            d[i].name = str(i)
        #the_data_o.domain = d
        
        return the_data_o

    def ComputeOrdinalSignature2(self,the_data):
        
        #the_data_o = the_data
        attrib_values=the_data[0].domain[0].values
        vv=[]
        exdat=[]
        for i in range(len(the_data[0])):
            #v=the_data[i].domain.attributes
            elist=[]
            elist.append(str(i))
            v=[orange.EnumVariable(x,values = the_data[i].domain.attributes[0].values) for x in elist]
            #v[0].name = str(i)
            vv.append(v[0])
            exdat.append('NaN')
            
        thed = orange.Domain(vv,0)

        the_data_o = orange.ExampleTable(thed)
        for i in range(len(the_data)):
            the_data_o.append(exdat)
        print '!!'
        print the_data_o[0]
        print the_data_o[1]
        print the_data_o[2]        
        #the_data_o = the_data.clone()
        

        #theatts=[]       
        #for i in range(len(the_data[0].domain.attributes[0].values)):
        #    tmpv=the_data[0].domain[i]
        #    tmpv.name=str(i)
        #    theatts.append(tmpv)

        for i in range(len(the_data)):
            ex = the_data[i];
            exl=[];
            for k in range(len(ex)):
                exl.append(ex[k].value)
            exlo = self.RemoveAdjacent(exl)
            #print exlo
            lastind = 0
            for k in range(len(exlo)):
                the_data_o[i][k]=exlo[k]
                lastind = k
            if lastind < len(the_data[0]):
                for k in range(lastind+1,len(the_data_o[i])):
                    the_data_o[i][k] = 'NaN'
        #d = the_data_o.domain
        #for i in range(len(d)):
        #    d[i].name = str(i)
        #print "!"
        #print theatts
        #the_data_o.domain = orange.Domain(theatts,0)
        print '!!!'
        print the_data_o[0]
        
        return the_data_o

    def RemoveAdjacent(self,example):
        a = []
        for item in example:
            if len(a):
               if a[-1] != item:
                  a.append(item)
            else:
                a.append(item)
        return a

    def MakeOrdinalList(self,the_data):
        ord_list=[]
        nums = len(the_data)
        numd = len(the_data[0])
        for i in range(nums):
            sample = the_data[i]
            sample2=[]
            for k in range(numd):
                sample2.append(sample[k].value)
            
            startind = 0
            tmplist = []
            while startind < numd:
                delta = 0
                attrib_to_find = sample2[startind]
                tmplist.append(attrib_to_find)
                while delta < 2:
                    curind = sample2.index(attrib_to_find,startind+1)
                    delta = curind-startind
                    startind = startind+1
                    delta=2
                    
            print ord_list        
            ord_list=[ord_list,tmplist]
        return ord_list
                    
        
        
    def MakeDomain(self,dat,thevalues):
        enumvarlist=[]
        for i in range(len(dat)):
            enumvarlist.append(str(dat[i][1]))
        #enumvarlist.append("nullclassvalue")
        v=[orange.EnumVariable(x,values = thevalues) for x in enumvarlist]    
        d = orange.Domain(v,0) #,0 means class-less
        return d

    #def selection(self):
        #indices = orange.MakeRandomIndices2(p0=self.proportion / 100.)
        #ind = indices(self.dataset)
        #self.sample = self.dataset.select(ind, 0)
   #     self.datawc = ComputeAttributeAtBirth.ComputeAttributeAtBirth(self.dataset)
   #     self.infob.setText('%d sampled instances' % len(self.datawc))

   # def commit(self):
   #     self.send("OrdinalSignature Data", self.datawc)
   #     

   # def checkCommit(self):
   #     if self.commitOnChange:
   #         self.commit()

##############################################################################
# Test the widget, run from prompt

if __name__=="__main__":
    appl = QApplication(sys.argv)
    ow = OWOrdinalSignature()
    ow.show()
    dataset = orange.ExampleTable('../datasets/iris.tab')
    ow.data(dataset)
    appl.exec_()
