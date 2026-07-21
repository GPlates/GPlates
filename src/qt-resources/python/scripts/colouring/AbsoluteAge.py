'''
 *
 * Copyright (C) 2026 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
'''

import math

import pygplates


DEFAULT_TIME_1 = 0.0
DEFAULT_TIME_2 = 450.0


def _finite_float(value, default):
	try:
		result = float(value)
	except (TypeError, ValueError):
		return default
	return result if math.isfinite(result) else default


def _gradient_position(age, time_1, time_2):
	if time_1 == time_2:
		return 0.0
	position = (age - time_1) / (time_2 - time_1)
	return max(0.0, min(1.0, position))


def _apply_steps(position, value):
	# An integer selects that many distinct colours, including both endpoint
	# colours. "smooth" (or "continuous") leaves the gradient unquantised.
	text = str(value).strip().lower()
	if not text or text in ('smooth', 'continuous'):
		return position
	try:
		steps = max(2, int(text))
	except (TypeError, ValueError):
		return position
	return math.floor(position * (steps - 1) + 0.5) / float(steps - 1)


def _interpolate_colour(colour_1, colour_2, position):
	return pygplates.Colour(
		colour_1.get_red() + (colour_2.get_red() - colour_1.get_red()) * position,
		colour_1.get_green() + (colour_2.get_green() - colour_1.get_green()) * position,
		colour_1.get_blue() + (colour_2.get_blue() - colour_1.get_blue()) * position,
		colour_1.get_alpha() + (colour_2.get_alpha() - colour_1.get_alpha()) * position)


class AbsoluteAge:
	def get_style(self, feature, style):
		age = _finite_float(feature.begin_time(), DEFAULT_TIME_1)
		time_1 = _finite_float(self.cfg['Endpoint 1 time (Ma)'], DEFAULT_TIME_1)
		time_2 = _finite_float(self.cfg['Endpoint 2 time (Ma)'], DEFAULT_TIME_2)
		position = _gradient_position(age, time_1, time_2)
		position = _apply_steps(position, self.cfg['Steps (or smooth)'])
		style.colour = _interpolate_colour(
			self.cfg['Endpoint 1 colour'],
			self.cfg['Endpoint 2 colour'],
			position)

	def get_config(self):
		return {
			'Endpoint 1 colour/type': 'Color',
			'Endpoint 1 colour/default': '#2c7bb6',
			'Endpoint 1 time (Ma)/type': 'String',
			'Endpoint 1 time (Ma)/default': '0',
			'Endpoint 2 colour/type': 'Color',
			'Endpoint 2 colour/default': '#d7191c',
			'Endpoint 2 time (Ma)/type': 'String',
			'Endpoint 2 time (Ma)/default': '450',
			'Steps (or smooth)/type': 'String',
			'Steps (or smooth)/default': 'smooth',
		}

	def get_config_variants(self):
		return {
			'Default': {
				'Endpoint 1 colour': '#2c7bb6',
				'Endpoint 1 time (Ma)': '0',
				'Endpoint 2 colour': '#d7191c',
				'Endpoint 2 time (Ma)': '450',
				'Steps (or smooth)': 'smooth',
			}
		}

	def set_config(self, config):
		self.cfg = config


def register():
	pygplates.Application().register_draw_style(AbsoluteAge())
