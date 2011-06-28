import pygplates
import random

class CameraDemo_1:
	def __init__(self):
		self.category = "Demo"
		self.name = "spin the world"

	def __call__(self):
		w = pygplates.MainWindow()
		while(1):
			w.rotate_camera_clockwise()
			w.move_camera_up()
			
class CameraDemo_2:
	def __init__(self):
		self.category = "Demo"
		self.name = "random looking"

	def __call__(self):
		w = pygplates.MainWindow()
		while(1):
			lat = random.randint(-90,90)
			lon = random.randint(-180,180)
			w.set_camera(lat,lon)
			
class CameraDemo_3:
	def __init__(self):
		self.category = "Demo"
		self.name = "super random"

	def __call__(self):
		w = pygplates.MainWindow()
		while(1):
			lat = random.randint(-90,90)
			lon = random.randint(-180,180)
			w.set_camera(lat,lon)
			zoom = random.randint(100,10000)
			w.set_zoom_percent(zoom)
			w.rotate_camera_clockwise()
			w.move_camera_up()
			

def register():
	pygplates.Application().register_utility(CameraDemo_1())
	pygplates.Application().register_utility(CameraDemo_2())
	pygplates.Application().register_utility(CameraDemo_3())

