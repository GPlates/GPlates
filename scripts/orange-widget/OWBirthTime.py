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
<name>BirthTime</name>
<description>Get birth time for all seeds.</description>
<icon>icons/compute_birth_time.png</icon>
<priority>800</priority>
<contact>Thomas Landgrebe (thomas.landgrebe(@at@)sydney.edu.au)</contact>
"""
from PyQt4 import QtGui, QtCore, uic
from OWWidget import *
import OWGUI, string, os.path, user, sys, warnings
import orngIO
import Orange
import gplates

class OWBirthTime(OWWidget):
    def __init__(self, parent=None, signalManager=None):
        OWWidget.__init__(self, parent, signalManager, 'Birth Time',wantMainArea = 0, resizingEnabled = 0)

        self.inputs = []
        #self.outputs = [("Birth Time", ExampleTable), ("Feature IDs", ExampleTable)]
        self.outputs = [("Birth Time", ExampleTable)]

        # GUI
        #self.box = OWGUI.widgetBox(self.controlArea, "Birth Time of Seeds")
        #OWGUI.button(self.box,self,"Get Data",callback=self.commit)
        script_path = os.path.dirname(__file__)
        self.ui = uic.loadUi(script_path+'/birth_time.ui')
        self.controlArea.layout().addWidget(self.ui)

        QtCore.QObject.connect(
            self.ui.commit_button,
            QtCore.SIGNAL('clicked()'),
            self.commit)

        self.resize(200,100)

        try:
              c = gplates.Client('localhost', 9777)
              self.coreg_layer = c.get_coregistration_layer()
        except Exception, e:
            print e
            self.coreg_layer = None
            print 'Failed to connect to GPlates'


    def commit(self):
        if not self.coreg_layer:
            try:
                c = gplates.Client('localhost', 9777)
                self.coreg_layer = c.get_coregistration_layer()
            except Exception, e:
                print e
                self.coreg_layer = None
                return 
        
        times = []
        seeds = self.coreg_layer.get_coreg_seeds()
        for seed in seeds:
            times.append(str(self.coreg_layer.get_begin_time(seed)))
            
        vv = [] 
        for item in set(times):
            vv.append(item)

        v = [
            orange.StringVariable('Feature ID'),
            orange.EnumVariable('Birth Time', values=vv)]
        domain =  Orange.data.Domain(v)
        data = Orange.data.Table(domain)
        
        seeds_str=[str(s) for s in seeds]
        for i in zip(seeds_str,times):
            data.append(list(i))
        self.send("Birth Time",data)

        '''v = [orange.StringVariable('Feature ID')]
        domain =  Orange.data.Domain(v)
        data = Orange.data.Table(domain)
        for i in seeds:
            data.append([str(i)])
        self.send("Feature IDs",data)'''

  
if __name__=="__main__":
    appl = QApplication(sys.argv)
    ow = OWBirthTime()
    ow.show()
    appl.exec_()

    

