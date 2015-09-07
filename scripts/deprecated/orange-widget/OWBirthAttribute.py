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
<name>BirthAttribute</name>
<description>Get attribute data at seed birth.</description>
<icon>icons/compute_birth_attribute.png</icon>
<priority>200</priority>
<contact>Thomas Landgrebe (thomas.landgrebe(@at@)sydney.edu.au)</contact>
"""
from decimal import *
import math
 
from PyQt4 import QtGui, QtCore, uic
import numpy 

from OWWidget import *
import OWGUI, string, os.path, user, sys, warnings
import orngIO
import Orange
import gplates

class OWBirthAttribute(OWWidget):

    def __init__(self, parent=None, signalManager = None):
        OWWidget.__init__(self, parent, signalManager, 'Birth Attribute', wantMainArea = 0, resizingEnabled = 0)

        self.inputs = []
        self.outputs = [("Attribute At Birth", ExampleTable)]
        #self.outputs = [("Attribute At Birth", ExampleTable),("Feature IDs", ExampleTable),("Birth Time", ExampleTable)]

        script_path = os.path.dirname(__file__)
        self.ui = uic.loadUi(script_path+'/birth_attr.ui')
        self.controlArea.layout().addWidget(self.ui)
        self.resize(250,120)
        
        self.RETRY_NUM = 5
        self.TIMEOUT = 30
        
        QtCore.QObject.connect(
            self.ui.commit_button,
            QtCore.SIGNAL('clicked()'),
            self.commit)
        QtCore.QObject.connect(
            self.ui.refresh_button,
            QtCore.SIGNAL('clicked()'),
            self.refresh)
      
        try:
            c = gplates.Client('localhost', 9777, self.TIMEOUT)
            self.coreg_layer = c.get_coregistration_layer()
      
            associations = self.coreg_layer.get_coreg_associations()
            if associations:
                for a in associations:
                    self.ui.property_comboBox.addItem(a.get_name())

        except Exception, e:
            print e
            self.coreg_layer = None
            print 'Failed to connect to GPlates'
        
    
    def commit(self):
        if not self.coreg_layer:
            if not self.refresh():
                return
        
        if self.ui.property_comboBox.count() == 0:
            return
        
        cur_idx = self.ui.property_comboBox.currentIndex()
        seeds = self.coreg_layer.get_coreg_seeds()             

        count = 0
        seed_id_vec = []
        birth_time_vec = []
        birth_attr_vec = []       

        pb = OWGUI.ProgressBar(self,iterations=len(seeds))
        for seed in seeds:
            pb.advance()
            while(count < self.RETRY_NUM):
                try:
                    attributes = self.coreg_layer.get_birth_attributes(seed) #a list of attributes
                    birth_attr_vec.append(str(attributes[cur_idx+2]))
                    seed_id_vec.append(str(seed))
                    birth_time_vec.append(str(attributes[1]))
                    break
                except Exception, e:
                    count +=1
                    print e
                    print 'Failed to get birth attribute.'
                    print 'retrying... ' + str(count)
        pb.finish()
        
        v = [orange.StringVariable('Feature ID'),
             orange.StringVariable('Birth Time'),
             orange.StringVariable(str(self.ui.property_comboBox.currentText()))]

        domain = Orange.data.Domain(v)
        data = Orange.data.Table(domain) #create empty table.
        #put data into the table
        for i in zip(seed_id_vec,birth_time_vec,birth_attr_vec):
            data.append(list(i))
        self.send("Attribute At Birth", data)

        
    def refresh(self):
        if not self.coreg_layer:
            try:
                c = gplates.Client('localhost', 9777, self.TIMEOUT)
                self.coreg_layer = c.get_coregistration_layer()
            except Exception, e:
                print e
                self.coreg_layer = None
                return False
            
        self.ui.property_comboBox.clear()
        associations = self.coreg_layer.get_coreg_associations()
        if associations:
           for a in associations:
               self.ui.property_comboBox.addItem(a.get_name())
        return True

 

if __name__=="__main__":
    a = QApplication(sys.argv)
    ow = OWBirthAttribute()
    a.setMainWidget(ow)

    ow.show()
    a.exec_loop()
    ow.saveSettings()

