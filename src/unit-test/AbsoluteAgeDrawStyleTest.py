import importlib.util
from pathlib import Path
import sys
import types
import unittest


class FakeColour:
	def __init__(self, red, green, blue, alpha):
		self.rgba = (red, green, blue, alpha)

	def get_red(self):
		return self.rgba[0]

	def get_green(self):
		return self.rgba[1]

	def get_blue(self):
		return self.rgba[2]

	def get_alpha(self):
		return self.rgba[3]


fake_pygplates = types.ModuleType('pygplates')
fake_pygplates.Colour = FakeColour
sys.modules['pygplates'] = fake_pygplates

script_path = (
	Path(__file__).resolve().parents[1]
	/ 'qt-resources' / 'python' / 'scripts' / 'colouring' / 'AbsoluteAge.py')
spec = importlib.util.spec_from_file_location('AbsoluteAge', script_path)
absolute_age = importlib.util.module_from_spec(spec)
spec.loader.exec_module(absolute_age)


class FakeFeature:
	def __init__(self, begin_time):
		self._begin_time = begin_time

	def begin_time(self):
		return self._begin_time


class FakeStyle:
	colour = None


class AbsoluteAgeDrawStyleTest(unittest.TestCase):
	def make_draw_style(self, steps='smooth'):
		draw_style = absolute_age.AbsoluteAge()
		draw_style.set_config({
			'Endpoint 1 colour': FakeColour(0.0, 0.0, 1.0, 1.0),
			'Endpoint 1 time (Ma)': '100',
			'Endpoint 2 colour': FakeColour(1.0, 0.0, 0.0, 0.5),
			'Endpoint 2 time (Ma)': '200',
			'Steps (or smooth)': steps,
		})
		return draw_style

	def colour_for_age(self, age, steps='smooth'):
		style = FakeStyle()
		self.make_draw_style(steps).get_style(FakeFeature(age), style)
		return style.colour.rgba

	def test_smooth_gradient_and_clamping(self):
		self.assertEqual((0.0, 0.0, 1.0, 1.0), self.colour_for_age(50))
		self.assertEqual((0.5, 0.0, 0.5, 0.75), self.colour_for_age(150))
		self.assertEqual((1.0, 0.0, 0.0, 0.5), self.colour_for_age(250))

	def test_step_count_includes_both_endpoint_colours(self):
		self.assertEqual((0.5, 0.0, 0.5, 0.75), self.colour_for_age(125, '3'))
		self.assertEqual((1.0, 0.0, 0.0, 0.5), self.colour_for_age(175, '3'))

	def test_reversed_endpoint_times(self):
		self.assertEqual(0.25, absolute_age._gradient_position(175, 200, 100))

	def test_invalid_step_count_falls_back_to_smooth(self):
		self.assertEqual((0.25, 0.0, 0.75, 0.875), self.colour_for_age(125, 'many'))

	def test_new_style_defaults_are_declared(self):
		config = absolute_age.AbsoluteAge().get_config()
		self.assertEqual('#2c7bb6', config['Endpoint 1 colour/default'])
		self.assertEqual('0', config['Endpoint 1 time (Ma)/default'])
		self.assertEqual('#d7191c', config['Endpoint 2 colour/default'])
		self.assertEqual('450', config['Endpoint 2 time (Ma)/default'])
		self.assertEqual('smooth', config['Steps (or smooth)/default'])


if __name__ == '__main__':
	unittest.main()
