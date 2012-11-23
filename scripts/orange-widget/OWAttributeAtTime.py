"""
<name>AttributeAtTime</name>
<description>Get data at a given time.</description>
<icon>icons/compute_attrib_at_time.png</icon>
<priority>201</priority>
<contact>Thomas Landgrebe (thomas.landgrebe(@at@)sydney.edu.au)</contact>
"""
from decimal import *
 
from PyQt4 import QtGui, QtCore, uic
import numpy 

from OWWidget import *
import OWGUI, string, os.path, user, sys, warnings
import orngIO
import Orange
import gplates

class OWAttributeAtTime(OWWidget):

    def __init__(self, parent=None, signalManager = None):
        OWWidget.__init__(self, parent, signalManager, 'Attribute At Time', wantMainArea = 0, resizingEnabled = 0)

        self.inputs = []
        self.outputs = [("Attribute Data", ExampleTable), ("Feature IDs", ExampleTable)]

        script_path = os.path.dirname(__file__)
        self.ui = uic.loadUi(script_path+'/attr_at_time.ui')
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

        time = self.ui.time_spinbox.value()

        if self.ui.property_comboBox.count() == 0:
            return 
        cur_idx = self.ui.property_comboBox.currentIndex()
        
        vec = []
        feature_ids=[]
        t = self.coreg_layer.get_coreg_data(time)
        if t:
            for row in t:
                vec.append(str(row[cur_idx+2])) #treat everything as strings
                feature_ids.append(str(row[0]))

        vv = []
        for item in set(vec):
            vv.append(item)
            
        v = [orange.EnumVariable(str(self.ui.property_comboBox.currentText())+str(time),values = vv)]
        domain = Orange.data.Domain(v)
        data = Orange.data.Table(domain) #create empty table.
        for i in vec:
            data.append([i])
        self.send("Attribute Data", data)

        v = [orange.StringVariable('Feature ID')]
        domain = Orange.data.Domain(v)
        data = Orange.data.Table(domain) #create empty table.
        for i in feature_ids:
            data.append([i])
        self.send("Feature IDs", data)
       

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
    ow = AttributeAtTime()
    a.setMainWidget(ow)

    ow.show()
    a.exec_loop()
    ow.saveSettings()

