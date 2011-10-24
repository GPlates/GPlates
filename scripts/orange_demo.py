import pygplates
import os, sys
import orange

orange_path = os.path.dirname(os.path.abspath(orange.__file__))
orange_canvas_path = orange_path + "/OrangeCanvas"
print orange_path

import shutil
shutil.copy2(orange_canvas_path+"/orngCanvas.pyw", "./GOrangeCanvas.py")
sys.path.append(os.getcwd())

import GOrangeCanvas 
from PyQt4.QtGui import *

class OrangeUtility:
	def __init__(self):
		self.category = "Third Party"
		self.name = "Open Orange Canvas"
		self.gui_obj = True;

	def __call__(self):
		#print "Full path to orngCanvas.pyw:",
		#canvas_path = raw_input()

		# Backup sys.stdout because the Orange Canvas redirects it to its output window.
		#import sys
		backup_stdout = sys.stdout

		#sys.argv = [canvas_path]
		backup_name = globals()["__name__"]
		globals()["__name__"] = ""

		sys.argv = ["test"]
		dlg = GOrangeCanvas.OrangeCanvasDlg(None)
		dlg.show()
        		
		sys.stdout = backup_stdout
		#del sys.argv
		globals()["__name__"] = backup_name
		
def register():
	pygplates.Application().register_utility(OrangeUtility())


