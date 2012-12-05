"""
<name>TimeSeries</name>
<description>Computes a time-series for a specified GPlates coregistration by merging coregistration output files.</description>
<icon>icons/merge_timedepdata.png</icon>
<priority>100</priority>
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

class OWTimeSeries(OWWidget):

    def __init__(self, parent=None, signalManager = None):
        OWWidget.__init__(self, parent, signalManager, 'Single-var co-registration merge', wantMainArea = 0, resizingEnabled = 0)

        self.inputs = []
        self.outputs = [("Coreg Data", ExampleTable), ('Feature IDs', ExampleTable)]

        script_path = os.path.dirname(__file__)
        self.ui = uic.loadUi(script_path+'/time_series.ui')
        #w = OWGUI.widgetBox(self.controlArea, '', addSpace = True, orientation=0)
        self.controlArea.layout().addWidget(self.ui)
        #w.layout().addWidget(self.ui)
        #self.adjustSize()
        self.resize(390,150)
        #self.setCaption('surprise me!')

        QtCore.QObject.connect(
            self.ui.commit_button,
            QtCore.SIGNAL('clicked()'),
            self.commit)

        QtCore.QObject.connect(
            self.ui.refresh_button,
            QtCore.SIGNAL('clicked()'),
            self.refresh)
                  
        #OWGUI.button(self.controlArea,self,"Commit",callback=self.commit)
        try:
            c = gplates.Client('localhost', 9777)
            self.coreg_layer = c.get_coregistration_layer()
        
            associations = self.coreg_layer.get_coreg_associations()
            if associations:
               for a in associations:
                   self.ui.property_comboBox.addItem(a.get_name())

            self.b_time, self.e_time, self.inc = self.coreg_layer.get_time_setting()
            self.ui.begin_time.setValue(self.b_time)
            self.ui.end_time.setValue(self.e_time)
            self.ui.increment.setValue(self.inc)
        except Exception, e:
            print e
            self.coreg_layer = None
            print 'Failed to connect to GPlates'
            

    def refresh(self):
        self.ui.property_comboBox.clear()
        if not self.coreg_layer:
            try:
                c = gplates.Client('localhost', 9777)
                self.coreg_layer = c.get_coregistration_layer()
            except Exception, e:
                print e
                self.coreg_layer = None
                return False
            
        self.b_time, self.e_time, self.inc = self.coreg_layer.get_time_setting()
        self.ui.begin_time.setValue(self.b_time)
        self.ui.end_time.setValue(self.e_time)
        self.ui.increment.setValue(self.inc)
        
       
        associations = self.coreg_layer.get_coreg_associations()
        if associations:
           for a in associations:
               self.ui.property_comboBox.addItem(a.get_name())
        return True

    def commit(self):
        if not self.coreg_layer:
            if not self.refresh():
                return
            
        #num = (b_time - e_time)/inc +1
        time_seq = numpy.arange(self.b_time, self.e_time, 0-self.inc)
        
        if ((time_seq[-1] - self.e_time) > 0.000001) :
            time_seq = numpy.append(time_seq,[self.e_time])

        if  self.ui.property_comboBox.count() == 0:
            return
        
        cur_idx = self.ui.property_comboBox.currentIndex()
        
        matrix = []
        feature_ids = []
        pb = OWGUI.ProgressBar(self,iterations=len(time_seq))
        self.ui.property_comboBox.setEnabled(False)
        self.ui.refresh_button.setEnabled(False)
        self.ui.commit_button.setEnabled(False)
        try:
            for time in time_seq:
                pb.advance()
                t = self.coreg_layer.get_coreg_data(time)
                if t:
                    counter=0
                    for row in t:
                        if(len(matrix) == counter):
                            matrix.append([str(row[cur_idx+2])]) #treat everything as strings
                            feature_ids.append(str(row[0]))
                        else:
                            matrix[counter].append(str(row[cur_idx+2]))
                        counter += 1
        except Exception,e:
            print e
        pb.finish()
        data =  self._creat_data_table(matrix, time_seq)
        self.ui.property_comboBox.setEnabled(True)
        self.ui.refresh_button.setEnabled(True)
        self.ui.commit_button.setEnabled(True)
        self.send("Coreg Data", data)
        v = [orange.StringVariable('Feature ID')]
        domain = Orange.data.Domain(v)
        ids = Orange.data.Table(domain)
        for i in feature_ids:
            ids.append([i])
        self.send('Feature IDs', ids)
                             


    def _creat_data_table(self, matrix, time_seq):
        v = None
        if self._is_numeric(matrix):
            v = [orange.FloatVariable(str(x)) for x in time_seq]
        else:
            vv = self._get_unique_vec(matrix)
            v = [orange.EnumVariable(str(x),values = vv) for x in time_seq]
            
        Domain = Orange.data.Domain(v)
        data = Orange.data.Table(Domain) #create empty table.
        for row in matrix:
            data.append(row)
        return data

    def _is_numeric(self, matrix):
        for row in matrix:
            for cell in row:
                if isinstance(cell,str):
                    if(cell.lower() != 'nan'):
                        return False
        return True

    def _get_unique_vec(self,matrix):
        vec = []
        for row in matrix:
            for cell in row:
                vec.append(cell)

        ret = [] 
        for item in set(vec):
            ret.append(item)
        return ret
            

if __name__=="__main__":
    import orange
    a = QApplication(sys.argv)
    ow = OWTimeSeries()
    a.setMainWidget(ow)

    ow.show()
    a.exec_loop()
    ow.saveSettings()

