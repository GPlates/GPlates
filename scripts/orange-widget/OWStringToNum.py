"""
<name>StringToNum</name>
<description>Given an input dataset with strings, convert to numeric if possible.</description>
<icon>icons/string_to_num.png</icon>
<priority>1100</priority>
<contact>Thomas Landgrebe (thomas.landgrebe(@at@)sydney.edu.au)</contact>
"""

from OWWidget import *
import OWGUI, string, os.path, user, sys, warnings
import orngIO


class OWStringToNum(OWWidget):
    #settingsList = ['undef_string']
    settingsList = ['undef_string']
    def __init__(self, parent=None, signalManager=None):
        OWWidget.__init__(self, parent, signalManager, 'NumData')

        self.inputs = [("Data", ExampleTable, self.data)]
        self.outputs = [("Converted Data", ExampleTable)]

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
            self.datan = self.convertdata(dataset,'NaN')
            self.send("Converted Data",self.datan)
            #self.optionsBox.setDisabled(0)
            #self.selection()
            #self.commit()
        else:
            self.send("Converted Data", None)
            #self.optionsBox.setDisabled(1)
            self.infoa.setText('No data on input yet, waiting to get something.')
            self.infob.setText('')


    def convertdata(self,dataset,undef_string = 'NaN'):
        numr = len(dataset)
        numc = len(dataset[0])
        vnm=list([])
        for i in range(numc):
            vnm.append(orange.FloatVariable(dataset.domain.variables[i].name))
            
        dd=orange.Domain(vnm,0)
        datawc = orange.ExampleTable(dd)
        for i in range(numr):
            exdata=[]
            for j in range(numc):
                val = dataset[i][j];
                #NB the next line is required to extract the native value from the Orange datastructure
                val = val.native()
                if(val == undef_string):
                    tmp = -99999999.0
                elif(val == 'OutOfRange'):
                    tmp = -99999999.0
                else:
                    tmp = float(val)
                exdata.append(tmp)
            datawc.append(exdata)
        return datawc


##############################################################################
# Test the widget, run from prompt

if __name__=="__main__":
    appl = QApplication(sys.argv)
    ow = OWStringToNum()
    ow.show()
    dataset = orange.ExampleTable('../datasets/iris.tab')
    ow.data(dataset)
    appl.exec_()
