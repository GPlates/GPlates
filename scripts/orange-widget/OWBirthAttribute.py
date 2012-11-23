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
        self.outputs = [("Attribute Data", ExampleTable),("Feature IDs", ExampleTable),("Birth Time", ExampleTable)]

        script_path = os.path.dirname(__file__)
        self.ui = uic.loadUi(script_path+'/birth_attr.ui')
        self.controlArea.layout().addWidget(self.ui)
        self.resize(250,120)
        
        QtCore.QObject.connect(
            self.ui.commit_button,
            QtCore.SIGNAL('clicked()'),
            self.commit)
        QtCore.QObject.connect(
            self.ui.refresh_button,
            QtCore.SIGNAL('clicked()'),
            self.refresh)
      
        try:
            c = gplates.Client('localhost', 9777)
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
            
        cache={float('inf'):self.coreg_layer.get_coreg_data(9999)}

        if self.ui.property_comboBox.count() == 0:
            return
        cur_idx = self.ui.property_comboBox.currentIndex()

        vec = []
        birth_time_vec=[]
        seeds = self.coreg_layer.get_coreg_seeds()
        
        for seed in seeds:
            table = []
            bt_time = self.coreg_layer.get_begin_time(seed)
            birth_time_vec.append(str(bt_time))
            if bt_time == float('inf'):
                bt_time = 9999;

            if str(bt_time)=='nan':
                vec.append('NaN')
                continue
            
            if bt_time not in cache:        
                table = self.coreg_layer.get_coreg_data(bt_time)
                cache[bt_time]=table
            else:
                table = cache[bt_time]

            for row in table:
                if row[0] == seed:
                    vec.append(str(row[cur_idx+2]))
        
        vv = []
        for item in set(vec):
            vv.append(item)
        bt_vv=[]
        for btt in set(birth_time_vec):
            bt_vv.append(btt)

        v = [orange.EnumVariable(str(self.ui.property_comboBox.currentText()),values = vv)]
        domain = Orange.data.Domain(v)
        data = Orange.data.Table(domain) #create empty table.
        for i in vec:
            data.append([i])
        self.send("Attribute Data", data)

        v = [orange.StringVariable('Feature ID')]
        domain = Orange.data.Domain(v)
        data = Orange.data.Table(domain) #create empty table.
        for i in seeds:
            data.append([str(i)])
        self.send("Feature IDs", data)

        v = [orange.EnumVariable('Birth Time', values = bt_vv)]
        domain = Orange.data.Domain(v)
        data = Orange.data.Table(domain) #create empty table.
        for i in birth_time_vec:
            data.append([str(i)])
        self.send("Birth Time", data)


    def refresh(self):
        if not self.coreg_layer:
            try:
                c = gplates.Client('localhost', 9777)
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

