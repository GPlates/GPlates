"""
<name>ReadDataSingleTime</name>
<description>Reads a single GPlates coregistration files for a chosen time instant.</description>
<icon>icons/load_data_select_time.png</icon>
<priority>400</priority>
<contact>Thomas Landgrebe (thomas.landgrebe(@at@)sydney.edu.au)</contact>
"""

from OWWidget import *
import OWGUI, string, os.path, user, sys, warnings
import orngIO
import LoadCoregFileSelectTime

class OWReadDataSingleTime(OWWidget):	
    settingsList = ["recentFiles",'commitOnChange','selected_time']

    def __init__(self, parent=None, signalManager = None, name='Read coreg data at t'):
        self.callbackDeposit = [] # deposit for OWGUI callback functions
        OWWidget.__init__(self, parent, signalManager, name) 
        
        self.outputs = [("Example Table", ExampleTable)]

        self.selected_time = 0.00
        self.commitOnChange = 0
        self.recentFiles=[]
        self.fileIndex = 0
        self.takeAttributeNames = False
        self.data = None
        self.loadSettings()
        self.coregdir = "."
        self.attrib_names = [] #for storing attribute names
#        self.coregvar_ind = 1;

        box = OWGUI.widgetBox(self.controlArea, "Coregistration Directory")
        hbox = OWGUI.widgetBox(box, orientation = 0)
        self.filecombo = OWGUI.comboBox(hbox, self, "fileIndex", callback = self.loadFile)
        self.filecombo.setMinimumWidth(250)
        button = OWGUI.button(hbox, self, '...', callback = self.browseFile)
        button.setMaximumWidth(25)

        OWGUI.separator(self.controlArea)
        self.selectvarBox = OWGUI.widgetBox(self.controlArea,"Parameters")
        OWGUI.doubleSpin(self.selectvarBox,self,'selected_time',min=0.00,max=4600.00,step=0.01,label='Time selection (MA):',callback=[self.selection,self.checkCommit])
        OWGUI.checkBox(self.selectvarBox,self,'commitOnChange', 'Commit data upon selection')
        OWGUI.button(self.selectvarBox,self,"Commit",callback=self.commit)
        self.selectvarBox.setDisabled(0)

        self.adjustSize()            

        if self.recentFiles:
            self.loadFile()

        #self.send("Example Table", data)
###                
    def selection(self):
        #self.infob.setText('%d sampled instances' % len(self.sample))
        print self.selected_time
        
    def commit(self):
        #self.send("Sampled Data", self.sample)
        self.loadFile()

    def checkCommit(self):
        if self.commitOnChange:
            self.commit()
####
    def browseFile(self):
        if self.recentFiles:
            lastPath = os.path.split(self.recentFiles[0])[0]
        else:
            lastPath = "."

        coregdir = str(QFileDialog.getExistingDirectory ( self, "Open Coreg Dir", lastPath, QFileDialog.ShowDirsOnly ))
        print coregdir
        if not coregdir:
            return
        
        if coregdir in self.recentFiles: # if already in list, remove it
            self.recentFiles.remove(coregdir)
        self.recentFiles.insert(0, coregdir)

        print coregdir
        self.coregdir = coregdir

        #fn = os.path.abspath(fn)
        #if fn in self.recentFiles: # if already in list, remove it
        #    self.recentFiles.remove(fn)
        #self.recentFiles.insert(0, fn)
        self.fileIndex = 0
        self.loadFile()
        

       
    def loadFile(self):
        #if self.fileIndex:
        #   fn = self.recentFiles[self.fileIndex]
        #    self.recentFiles.remove(fn)
        #    self.recentFiles.insert(0, fn)
        #    self.fileIndex = 0
        #else:
        #    fn = self.recentFiles[0]

        #self.filecombo.clear()
        #for file in self.recentFiles:
        #    self.filecombo.addItem(os.path.split(file)[1])
        #self.filecombo.updateGeometry()
        #self.error()
        self.filecombo.clear()
        for file in self.recentFiles:
            self.filecombo.addItem(os.path.split(file)[1])
        self.filecombo.updateGeometry()
        self.error()
        data = None
        try:
            pb = OWGUI.ProgressBar(self,iterations=100)
            print '+++++++++++++'
            data=LoadCoregFileSelectTime.LoadCoregFileSelectTime(self.coregdir,'co_registration',self.selected_time)
            print '============='
            pb.finish()
            #print data
            
            self.send("Example Table", data)
            if not data:
                self.error("No dir or unknown problem")
        except:
            self.error("Cannot read the dir")
        
 


if __name__=="__main__":
    import orange
    a = QApplication(sys.argv)
    ow = OWReadDataSingleTime()
    a.setMainWidget(ow)

    ow.show()
    a.exec_loop()
    ow.saveSettings()
