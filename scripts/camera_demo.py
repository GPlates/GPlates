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
	pass
	#pygplates.Application().register_utility(CameraDemo_1())
	#pygplates.Application().register_utility(CameraDemo_2())
	#pygplates.Application().register_utility(CameraDemo_3())

