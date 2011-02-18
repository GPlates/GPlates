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

		pygplates.instance.exec_file_on_main_thread(canvas_path)
		pygplates.instance.exec_string_on_main_thread("dlg = OrangeCanvasDlg(None); dlg.show()")

		sys.stdout = backup_stdout
		del sys.argv
		globals()["__name__"] = backup_name

# This needs to be run on the main thread because it causes an item to be added to the menus.
pygplates.instance.exec_string_on_main_thread("pygplates.instance.register_utility(OrangeUtility())")

