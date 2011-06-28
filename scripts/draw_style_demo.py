import pygplates

class DrawStyle_1:
	def __init__(self):
		self.name = "ds1"
	
	def get_style(self, feature, style):
		style.dummy=1234;
		print "returning from get_style python."

class DrawStyle_2:
	def __init__(self):
		self.name = "ds2"
	
	def get_style(self, feature, style):
		style.dummy=1234;
		print "returning from get_style python."

		
class DrawStyle_3:
	def __init__(self):
		self.name = "ds3"
	
	def get_style(self, feature, style):
		style.dummy=1234;
		print "returning from get_style python."

		
class DrawStyle_4:
	def __init__(self):
		self.name = "ds4"
	
	def get_style(self, feature, style):
		style.dummy=1234;
		print "returning from get_style python."

class DrawStyle_5:
	def __init__(self):
		self.name = "ds5"
	
	def get_style(self, feature, style):
		style.dummy=1234;
		print "returning from get_style python."

def register():
	pygplates.Application().register_draw_style(DrawStyle_1())
	pygplates.Application().register_draw_style(DrawStyle_2())
	pygplates.Application().register_draw_style(DrawStyle_3())
	pygplates.Application().register_draw_style(DrawStyle_4())
	pygplates.Application().register_draw_style(DrawStyle_5())

