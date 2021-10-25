"""
<name>MergeTableRowAligned</name>
<description>Merge two tables assuming they are row-aligned.</description>
<icon>icons/combine_data.png</icon>
<priority>801</priority>
<contact>TL</contact>
"""
import orange
from OWWidget import *
import OWGUI, string
import CombineDomains

class OWMergeRowAligned(OWWidget):
    def __init__(self, parent=None, signalManager=None, name="MergeTableRowAligned"):
        OWWidget.__init__(self, parent, signalManager, name, wantMainArea=0)
        self.inputs = [("Table1", orange.ExampleTable, self.setData), ("Table2", orange.ExampleTable, self.setMoreData)]
        self.outputs = [("MergedTable", ExampleTable)]
        box = OWGUI.widgetBox(self.controlArea, "Data Set Size", addSpace=True)
        #OWGUI.rubber(self.controlArea)
        self.resize(200, 200)
        self.data1 = None
        self.data2 = None
        OWGUI.rubber(self.controlArea)
        self.loadSettings()
        self.adjustSize()

    def setData(self, data):
        self.data1 = data
        self.apply()

    def setMoreData(self, data):
        self.data2 = data
        self.apply()

    def apply(self):
        if self.data1:
            if self.data2:
                print len(self.data1)
                print len(self.data2)
                if(len(self.data1) == len(self.data2)):
                    #self.send("MergedTable",self.data1)
                    dc = CombineDomains.CombineDomains(self.data1.domain,self.data2.domain)
                    exlist=[]
                    for i in range(len(self.data1)):
                        #the next line causes a Mac crash
                        exlist.append(orange.Example(dc,[self.data1[i],self.data2[i]]))
                    self.send("MergedTable",orange.ExampleTable(exlist))
                else:
                    print 'Cannot merge because tables have different sizes'

    
if __name__ == "__main__":
    app = QApplication(sys.argv)
    w = OWMergeRowAligned()
    data = orange.ExampleTable("../../doc/datasets/iris.tab")
    w.setData(data)
    w.setMoreData(data)
    w.show()
    app.exec_()
