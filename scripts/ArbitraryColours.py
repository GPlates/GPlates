'''
 * 
 * Copyright (C) 2014, 2015 The University of Sydney, Australia
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
# Author: James Clark. I am not a particularly 'pythonic' person so this is not the greatest script in the world, sorry.

import pygplates
import colorsys

# The pygplates API expects rgba values; we want to supply hsv.
def colour_hsv(h, s, v):
	rgb = colorsys.hsv_to_rgb(h, s, v)
	# Although python apparently has a * operator that flattens out tuples, it doesn't seem to be working well
	# with the extra 'alpha' parameter required; the least bad way seems to be separating out the components
	# individually and then calling the function.
	return pygplates.Colour(rgb[0], rgb[1], rgb[2], 1.0)

# Take each character of the string and, assuming it is in the ASCII range (I have no idea how Python treats Unicode),
# sum the ordinal value of each character and modulo 256 to generate a number which will be representational of
# that string in a deterministic and "random" fashion; we don't want to use secure crypto hashes here, this is colouring.
#
# ... you know what? let's try and make a cache for it.
hash_str_cache = {}
def hash_str(s):
	# This is probably the slowest bit of the colouring, and if we moved it to C++ might improve the performance a bit,
	# but it's still _python_ colouring so it's not gonna be super fast anyway.
	# Incidentally, if we did move it to C++, we could potentially just munge the memory address since GPlates' shared
	# strings should give each one a the same somewhat-arbitrary number. But it might not have the same neat property
	# of this approach, which is that strings that are anagrams of each other return the same number: this is, amazingly,
	# actually relevant for e.g. the isochon names where they've written stuff like "AFRICA-SOUTH AMERICA ISOCHRON" 
	# and "SOUTH AMERICA-AFRICA ISOCHRON"; they magically get assigned the same colour which is awesome.
	if s in hash_str_cache:
		return hash_str_cache[s]
	else:
		hash = sum(map(ord, s)) % 256
		hash_str_cache[s] = hash
		return hash

def str_to_float(s, min, max):
	# Get a deterministic-but-random-looking number 0-255 based on the string.
	# divide that by 256.0 to force floating-point division apparently, and return a value between 0.0 and almost 1.0.
	normalised = hash_str(s) / 255.0
	# To make things prettier, we can then scale the output according to some supplied min and max.
	scaled = min + (normalised * (max - min))
	return scaled

def channel_to_float(s):
	# Okay so... we have a String, it is probably a float 0.0-1.0 but could be the word "Vary".
	# Or anything else. This function attempts to make it a float, and if it's anything else,
	# returns -1 (signifying to the style object "please vary this channel")
	try:
		f = float(s)
		return f
	except ValueError:
		return -1


def parse_variance_params(min_str, max_str):
	# Okay so... we have a String, it should be a float 0.0-1.0 but really, who knows... =(
	try:
		min_float = float(min_str)
		max_float = float(max_str)
		return (min_float, max_float)
	except ValueError:
		return (0.0, 1.0)


# ArbitraryColours : Just pick a damned colour based on featureId.
class ArbitraryColours:
	def __init__(self):
		print "ArbitraryColours::init()"
		#self.cfg['Vary'] = 'Saturation' # If I do anything here, script never gets registered. why?
		return
			
	def get_style(self, feature, style):
		# We use the feature ID as a deterministic-yet-arbitrary seed for the colour variation;
		# We scale that to a given range for enhanced prettiness.
		# The parsing of the strings to floats is terrible and I shouldn't be doing it here,
		# I should be doing it... after params are changed? Somehow?
		(variance_min, variance_max) = parse_variance_params(self.cfg['Variance Min'], self.cfg['Variance Max'])
		feature_variance = str_to_float(feature.feature_id(), variance_min, variance_max)

		h = channel_to_float(self.cfg['Hue'])
		if (h < 0):
			h = feature_variance;
		s = channel_to_float(self.cfg['Saturation'])
		if (s < 0):
			s = feature_variance;
		v = channel_to_float(self.cfg['Value'])
		if (v < 0):
			v = feature_variance;
		style.colour = colour_hsv(h, s, v)
		return

	def get_config(self):
		self.cfg_dict = {}
		# Each of these parameters is a string, which could be 0.0-1.0 indicating the relative strength of that colour
		# channel, or the word "Vary" (technically, or any other thing we can't floatify). We will vary those channels
		# based on feature ID.
		# There are several *much better* ways to go about this, but I am time and space limited; I'd prefer to have
		# a single Base Colour attribute that uses the Colour pygplates draw style type, so users can just set a colour
		# using the picker; however, I've discovered that the returned pygplates.Colour type HAS NO ACCESSORS.
		self.cfg_dict['Hue/type'] = 'String'
		self.cfg_dict['Saturation/type'] = 'String'
		self.cfg_dict['Value/type'] = 'String'
		self.cfg_dict['Variance Min/type'] = 'String'
		self.cfg_dict['Variance Max/type'] = 'String'
		return self.cfg_dict

	def get_config_variants(self):
		variants = {}
		# Braces? In my Python? It's more likely than you think.
		variants['Varied Hue - Bright'] = {
			'Hue': 'Vary', 'Saturation': '0.8', 'Value': '1.0',
			'Variance Min': '0.0',
			'Variance Max': '1.0',
		}
		variants['Varied Hue - Dark'] = {
			'Hue': 'Vary', 'Saturation': '0.8', 'Value': '0.5',
			'Variance Min': '0.0',
			'Variance Max': '1.0',
		}
		variants['Varied Hue - Pastel'] = {
			'Hue': 'Vary', 'Saturation': '0.3', 'Value': '1.0',
			'Variance Min': '0.0',
			'Variance Max': '1.0',
		}
		variants['128 Shades of Gray'] = {
			'Hue': '0.0', 'Saturation': '0.0', 'Value': 'Vary',
			'Variance Min': '0.25',
			'Variance Max': '0.75',
		}
		variants['Varied Saturation - Reds'] = {
			'Hue': '0.0', 'Saturation': 'Vary', 'Value': '1.0',
			'Variance Min': '0.25',
			'Variance Max': '1.0',
		}
		variants['Varied Saturation - Blues'] = {
			'Hue': '0.66', 'Saturation': 'Vary', 'Value': '1.0',
			'Variance Min': '0.25',
			'Variance Max': '1.0',
		}
		variants['Varied Value - Greens'] = {
			'Hue': '0.33', 'Saturation': '1.0', 'Value': 'Vary',
			'Variance Min': '0.25',
			'Variance Max': '1.0',
		}
		variants['Varied Value - Cyans'] = {
			'Hue': '0.5', 'Saturation': '1.0', 'Value': 'Vary',
			'Variance Min': '0.25',
			'Variance Max': '1.0',
		}
		variants['Varied Value - Magentas'] = {
			'Hue': '0.83', 'Saturation': '0.8', 'Value': 'Vary',
			'Variance Min': '0.25',
			'Variance Max': '1.0',
		}
		return variants
		
	def set_config(self, config):
		# I don't think this is getting called and don't know why.
		print "ArbitraryColours::set_config()"
		self.cfg = config


# ArbitraryByProperty : Just pick a damned colour based on a given property.
class ArbitraryByProperty:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		# We use the given property, if present, as a deterministic-yet-arbitrary seed for the colour variation;
		property_name = self.cfg['Property Name']
		properties = feature.get_properties_by_name(property_name)
		if (len(properties) == 0):
			return
		property_value = properties[0]
		# We scale that to a given range for enhanced prettiness.
		# The parsing of the strings to floats is terrible and I shouldn't be doing it here,
		# I should be doing it... after params are changed? Somehow?
		(variance_min, variance_max) = parse_variance_params(self.cfg['Variance Min'], self.cfg['Variance Max'])
		feature_variance = str_to_float(property_value, variance_min, variance_max)

		h = channel_to_float(self.cfg['Hue'])
		if (h < 0):
			h = feature_variance;
		s = channel_to_float(self.cfg['Saturation'])
		if (s < 0):
			s = feature_variance;
		v = channel_to_float(self.cfg['Value'])
		if (v < 0):
			v = feature_variance;
		style.colour = colour_hsv(h, s, v)
		return

	def get_config(self):
		self.cfg_dict = {}
		# What property to look up.
		self.cfg_dict['Property Name/type'] = 'String'
		# Each of these parameters is a string, which could be 0.0-1.0 indicating the relative strength of that colour
		# channel, or the word "Vary" (technically, or any other thing we can't floatify). We will vary those channels
		# based on feature ID.
		# There are several *much better* ways to go about this, but I am time and space limited; I'd prefer to have
		# a single Base Colour attribute that uses the Colour pygplates draw style type, so users can just set a colour
		# using the picker; however, I've discovered that the returned pygplates.Colour type HAS NO ACCESSORS.
		self.cfg_dict['Hue/type'] = 'String'
		self.cfg_dict['Saturation/type'] = 'String'
		self.cfg_dict['Value/type'] = 'String'
		self.cfg_dict['Variance Min/type'] = 'String'
		self.cfg_dict['Variance Max/type'] = 'String'
		return self.cfg_dict

	def get_config_variants(self):
		variants = {}
		# Braces? In my Python? It's more likely than you think.
		variants['gml:name'] = {
			# Amusingly, due to how the 'hashing' works, features whose names are anagrams of each other get the same colour.
			'Property Name': 'gml:name',
			'Hue': 'Vary', 'Saturation': '0.75', 'Value': '0.9',
			'Variance Min': '0.0',
			'Variance Max': '1.0',
		}
		variants['gpml:subcategory'] = {
			'Property Name': 'gpml:subcategory',
			'Hue': 'Vary', 'Saturation': '0.75', 'Value': '0.9',
			'Variance Min': '0.0',
			'Variance Max': '1.0',
		}
		variants['gpml:polarityChronId'] = {
			'Property Name': 'gpml:polarityChronId',
			'Hue': '0.5', 'Saturation': 'Vary', 'Value': '0.9',
			'Variance Min': '0.1',
			'Variance Max': '1.0',
		}
		return variants
		
	def set_config(self, config):
		self.cfg = config


# ArbitraryChrons : Specifically for Isochron / Magnetic Pick stuff.
class ArbitraryChrons:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		# For chrons, ideally we will have gpml:polarityChronId set. But this isn't always going to be the case, probably.
		# We can fall back to using gml:name if polarityChronId isn't found.
		id_property_name_1 = self.cfg['Identity Property Name']
		id_property_name_2 = self.cfg['Fallback Property Name']
		id_property_value = ""
		properties = feature.get_properties_by_name(id_property_name_1)
		if (len(properties) > 0):
			id_property_value = properties[0]
		else:
			properties = feature.get_properties_by_name(id_property_name_2)
			if (len(properties) > 0):
				id_property_value = properties[0]

		# For speed, we hash that value only once; for maximum variation between chrons, we then modulo that number
		# against different numbers to choose saturation and value.
		hashed = hash_str(id_property_value)

		# lightness of the colour is expressed as a rapid change for minimal variation in the string.
		colour_value = 0.5 + (hashed % 3) * 0.25

		# saturation of the colour is allowed to vary more smoothly.
		colour_saturation = 0.25 + (((hashed * 37) % 256) / 256.0) * 0.75

		# hue is determined by the orientation of the magnetic field, normal or reverse.
		# depending on the data, this may not be available.
		colour_hue = channel_to_float(self.cfg['Normal Hue'])
		properties = feature.get_properties_by_name("gpml:polarityChronOrientation")
		if (len(properties) > 0):
			if (properties[0] == 'Reverse'):
				colour_hue = channel_to_float(self.cfg['Reverse Hue'])
		# Hue is tweaked based on the hash number as well, plus or minus 0.1 should work.
		colour_hue = colour_hue + (((hashed % 11) - 5) / 50)
		# Keep it clamped within range though.
		if (colour_hue < 0):
			colour_hue += 1.0
		if (colour_hue > 1.0):
			colour_hue -= 1.0

		style.colour = colour_hsv(colour_hue, colour_saturation, colour_value)
		return

	def get_config(self):
		self.cfg_dict = {}
		# What colours to assign.
		self.cfg_dict['Normal Hue/type'] = 'String'
		self.cfg_dict['Reverse Hue/type'] = 'String'
		# What properties to try and use for the 'identity' component.
		self.cfg_dict['Identity Property Name/type'] = 'String'
		self.cfg_dict['Fallback Property Name/type'] = 'String'
		return self.cfg_dict

	def get_config_variants(self):
		variants = {}
		# Braces? In my Python? It's more likely than you think.
		variants['Default'] = {
			'Normal Hue': '0.5',
			'Reverse Hue': '0.65',
			'Identity Property Name': 'gpml:polarityChronId',
			'Fallback Property Name': 'gml:name',
		}
		return variants
		
	def set_config(self, config):
		self.cfg = config


		
def register():
	pygplates.Application().register_draw_style(ArbitraryColours())
	pygplates.Application().register_draw_style(ArbitraryByProperty())
	pygplates.Application().register_draw_style(ArbitraryChrons())


