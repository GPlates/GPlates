import pygplates

class OrangeUtility:
	def __init__(self):
		self.category = "Third Party"
		self.name = "Open Orange Canvas"

	def __call__(self):
		print "Full path to orngCanvas.pyw:",
		canvas_path = raw_input()

		# Backup sys.stdout because the Orange Canvas redirects it to its output window.
		import sys
		backup_stdout = sys.stdout

		sys.argv = [canvas_path]
		backup_name = globals()["__name__"]
		globals()["__name__"] = ""

		#original orngCanvas.pyw should not work. you need to change it.
		pygplates.Application().exec_gui_file(canvas_path) 
        		
		sys.stdout = backup_stdout
		del sys.argv
		globals()["__name__"] = backup_name
		
def register():
	pygplates.Application().register_utility(OrangeUtility())


