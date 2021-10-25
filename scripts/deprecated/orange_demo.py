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


