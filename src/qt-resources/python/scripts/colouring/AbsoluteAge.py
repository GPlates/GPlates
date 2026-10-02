'''
 *
 * Copyright (C) 2026 CaliTarheel
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


# Used only when the configured view range cannot be read - see _default_times().
FALLBACK_YOUNGEST_TIME = 0.0
FALLBACK_OLDEST_TIME = 410.0

# Newest features are orange, oldest are green.
YOUNGEST_COLOUR = '#e66101'
OLDEST_COLOUR = '#1a9641'


def _default_times():
	# Span the reconstruction range the project actually uses, taken from
	# Preferences > Default View Settings, rather than a hardcoded guess. Older versions of
	# GPlates do not expose this, so fall back to the documented default range.
	try:
		oldest, youngest = pygplates.Application().default_time_range()
		oldest = float(oldest)
		youngest = float(youngest)
	except Exception:
		return FALLBACK_YOUNGEST_TIME, FALLBACK_OLDEST_TIME

	if not math.isfinite(oldest) or not math.isfinite(youngest) or oldest == youngest:
		return FALLBACK_YOUNGEST_TIME, FALLBACK_OLDEST_TIME

	return youngest, oldest


def _format_time(value):
	return ('%g' % value)


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


def _variant(colour_1, time_1, colour_2, time_2, steps):
	# Endpoint 1 is not required to be the younger time - see the reversed example below.
	return {
		'Endpoint 1 colour': colour_1,
		'Endpoint 1 time (Ma)': _format_time(time_1),
		'Endpoint 2 colour': colour_2,
		'Endpoint 2 time (Ma)': _format_time(time_2),
		'Steps (or smooth)': steps,
	}


class AbsoluteAge:
	def get_style(self, feature, style):
		youngest_time, oldest_time = _default_times()
		age = _finite_float(feature.begin_time(), youngest_time)
		time_1 = _finite_float(self.cfg['Endpoint 1 time (Ma)'], youngest_time)
		time_2 = _finite_float(self.cfg['Endpoint 2 time (Ma)'], oldest_time)
		position = _gradient_position(age, time_1, time_2)
		position = _apply_steps(position, self.cfg['Steps (or smooth)'])
		style.colour = _interpolate_colour(
			self.cfg['Endpoint 1 colour'],
			self.cfg['Endpoint 2 colour'],
			position)

	def get_config(self):
		youngest_time, oldest_time = _default_times()
		return {
			'Endpoint 1 colour/type': 'Color',
			'Endpoint 1 colour/default': YOUNGEST_COLOUR,
			'Endpoint 1 time (Ma)/type': 'String',
			'Endpoint 1 time (Ma)/default': _format_time(youngest_time),
			'Endpoint 2 colour/type': 'Color',
			'Endpoint 2 colour/default': OLDEST_COLOUR,
			'Endpoint 2 time (Ma)/type': 'String',
			'Endpoint 2 time (Ma)/default': _format_time(oldest_time),
			'Steps (or smooth)/type': 'String',
			'Steps (or smooth)/default': 'smooth',
		}

	def get_config_variants(self):
		# Worked examples rather than one empty starting point. Seeing several configurations
		# side by side is the quickest way to understand that this style is an age-to-colour ramp
		# between two editable endpoints - which is not obvious from a single default, and was not
		# obvious at all when the list started empty.
		#
		# Each example is here to demonstrate something the style can do - the whole range, a
		# narrowed range, a reversed range, quantisation into bands - rather than being just
		# another pair of colours. Select one and click New to get an editable copy of it.
		youngest_time, oldest_time = _default_times()
		span = oldest_time - youngest_time
		midpoint_time = youngest_time + span / 2.0
		near_present_time = youngest_time + span / 8.0

		return {
			# Newest orange, oldest green, across the project's whole range.
			'Default': _variant(
				YOUNGEST_COLOUR, youngest_time, OLDEST_COLOUR, oldest_time, 'smooth'),

			# The same range and colours quantised into bands, which makes age groupings
			# readable at a glance instead of blending continuously.
			'Banded (6 steps)': _variant(
				YOUNGEST_COLOUR, youngest_time, OLDEST_COLOUR, oldest_time, '6'),

			# Two steps is the smallest banding that means anything: one hard boundary at the
			# middle of the range, so every feature reads as simply older or younger than that.
			'Two tone (2 steps)': _variant(
				YOUNGEST_COLOUR, youngest_time, OLDEST_COLOUR, oldest_time, '2'),

			# Only the more recent half of the range, so recent features are separated instead
			# of being crushed into one end of the ramp. Anything older clamps to the old colour.
			'Recent half': _variant(
				YOUNGEST_COLOUR, youngest_time, OLDEST_COLOUR, midpoint_time, 'smooth'),

			# The mirror of the above: spread the ramp over the older half instead, for reading
			# basement ages when the recent end is not what is being looked at.
			'Ancient half': _variant(
				YOUNGEST_COLOUR, midpoint_time, OLDEST_COLOUR, oldest_time, 'smooth'),

			# A narrow window close to the present. Clamping is what makes a narrow window
			# usable: everything older than the window simply takes the old colour.
			'Near present': _variant(
				YOUNGEST_COLOUR, youngest_time, OLDEST_COLOUR, near_present_time, 'smooth'),

			# Endpoint 1 does not have to be the younger time. Give the two times the other way
			# round and the ramp runs backwards, so the oldest features take the first colour.
			'Reversed': _variant(
				YOUNGEST_COLOUR, oldest_time, OLDEST_COLOUR, youngest_time, 'smooth'),

			# A cool-to-warm alternative for when orange and green are already in use.
			'Blue to red': _variant(
				'#2c7bb6', youngest_time, '#d7191c', oldest_time, 'smooth'),

			# Sequential yellow to brown, which stays readable under the common forms of colour
			# blindness that make the orange and green of the default hard to tell apart.
			'Yellow to brown': _variant(
				'#fff7bc', youngest_time, '#993404', oldest_time, 'smooth'),

			# Neutral ramp for figures that are printed or reproduced in greyscale.
			'Greyscale': _variant(
				'#f0f0f0', youngest_time, '#252525', oldest_time, 'smooth'),
		}

	def set_config(self, config):
		self.cfg = config


def register():
	pygplates.Application().register_draw_style(AbsoluteAge())
