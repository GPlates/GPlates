/* $Id$ */

/**
 * @file 
 * Contains the definition of the Colour class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 The University of Sydney, Australia
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
 */

#ifndef GPLATES_GUI_COLOUR_H
#define GPLATES_GUI_COLOUR_H

#include <algorithm>
#include <iosfwd>
#include <boost/cstdint.hpp>
#include <opengl/OpenGL.h>
#include <QColor>
#include <QDataStream>


namespace GPlatesGui
{
	struct HSVColour
	{
		/** Hue */
		double h;
		/** Saturation */
		double s;
		/** Value */
		double v;
		/** Alpha */
		double a;

		HSVColour(
				double h_,
				double s_,
				double v_,
				double a_ = 1.0) :
			h(h_),
			s(s_),
			v(v_),
			a(a_)
		{  }
	};

	struct CMYKColour
	{
		/** Cyan */
		double c;
		/** Magenta */
		double m;
		/** Yellow */
		double y;
		/** Black */
		double k;

		CMYKColour(
				double c_,
				double m_,
				double y_,
				double k_) :
			c(c_),
			m(m_),
			y(y_),
			k(k_)
		{  }
	};

	struct rgba8_t
	{
		static const int NUM_COMPONENTS = 4;

		union
		{
			// If it's easier to access as an array of chars...
			char components[NUM_COMPONENTS];

			// If it's easier to access using named components...
			struct
			{
				boost::uint8_t red, green, blue, alpha;
			};

			// If it's easier to access as a 32-bit int...
			boost::int32_t int32_value;

			// If it's easier to access as a 32-bit unsigned int...
			boost::uint32_t uint32_value;
		};

		/**
		 * This DOES NOT initialise any of the components.
		 */
		rgba8_t()
		{  }

		rgba8_t(
				boost::uint8_t red_,
				boost::uint8_t green_,
				boost::uint8_t blue_,
				boost::uint8_t alpha_) :
			red(red_),
			green(green_),
			blue(blue_),
			alpha(alpha_)
		{  }

		bool
		operator==(
				const rgba8_t &other)
		{
			return int32_value == other.int32_value;
		}

		bool
		operator!=(
				const rgba8_t &other)
		{
			return int32_value != other.int32_value;
		}
	};


	/**
	 * Convert an array of pixels from the 32-bit integer format 0xAARRGGBB
	 * to the 4 x 8-byte (R,G,B,A) format (ie, the @a rgba8_t type).
	 *
	 * Both array must have at least @a num_pixels pixels.
	 *
	 * Note that (a,b,c,d) means a 4-byte ordering in *memory* (not in a 32-bit integer).
	 *
	 * Also note that QImage::Format_ARGB32 means the 32-bit integer 0xAARRGGBB and
	 * not necessarily (B,G,R,A) - they're only the same on little-endian machines where
	 * the least significant part of the integer (litte end) goes into the byte array first.
	 *
	 * Also note that GL_RGBA means (R,G,B,A) on both little and big endian machines -
	 * in other words it specifies byte ordering in memory (not in a 32-bit integer like Qt).
	 */
	void
	convert_argb32_to_rgba8(
			const boost::uint32_t *argb32_pixels,
			rgba8_t *rgba8_pixels,
			unsigned int num_pixels);

	/**
	 * Convert an array of pixels from the 4 x 8-byte (R,G,B,A) format (ie, the @a rgba8_t type)
	 * to the 32-bit integer format 0xAARRGGBB.
	 *
	 * Both array must have at least @a num_pixels pixels.
	 *
	 * Note that (a,b,c,d) means a 4-byte ordering in *memory* (not in a 32-bit integer).
	 *
	 * Also note that QImage::Format_ARGB32 means the 32-bit integer 0xAARRGGBB and
	 * not necessarily (B,G,R,A) - they're only the same on little-endian machines where
	 * the least significant part of the integer (litte end) goes into the byte array first.
	 *
	 * Also note that GL_RGBA means (R,G,B,A) on both little and big endian machines -
	 * in other words it specifies byte ordering in memory (not in a 32-bit integer like Qt).
	 */
	void
	convert_rgba8_to_argb32(
			const rgba8_t *rgba8_pixels,
			boost::uint32_t *argb32_pixels,
			unsigned int num_pixels);


	/**
	 * Writes an array of @a rgba8_t pixels to the output stream.
	 *
	 * NOTE: It bypasses the expensive output operator '<<' and hence is *much*
	 * faster than doing a loop with '<<' (as determined by profiling).
	 *
	 * Note that GPlatesGui::rgba8_t stores 4 bytes in memory as (R,G,B,A) and the
	 * data is written to the stream as bytes (not 32-bit integers) so there's
	 * no need to re-order the bytes according to the endianess of the current system.
	 * Note that QDataStream stores streams in big-endian format (but that's irrelevant here).
	 */
	inline
	void
	output_pixels(
			QDataStream &out,
			const rgba8_t *rgba8_pixels,
			unsigned int num_pixels)
	{
		out.writeRawData(
				reinterpret_cast<const char *>(rgba8_pixels),
				num_pixels * sizeof(rgba8_t));
	}

	/**
	 * Read to array of @a rgba8_t pixels from the input stream.
	 *
	 * NOTE: It bypasses the expensive input operator '>>' and hence is *much*
	 * faster than doing a loop with '>>' (as determined by profiling).
	 *
	 * Note that GPlatesGui::rgba8_t stores 4 bytes in memory as (R,G,B,A) and the
	 * data is read from the stream as bytes (not 32-bit integers) so there's
	 * no need to re-order the bytes according to the endianess of the current system.
	 * Note that QDataStream stores streams in big-endian format (but that's irrelevant here).
	 */
	inline
	void
	input_pixels(
			QDataStream &in,
			rgba8_t *rgba8_pixels,
			unsigned int num_pixels)
	{
		in.readRawData(
				reinterpret_cast<char *>(rgba8_pixels),
				num_pixels * sizeof(rgba8_t));
	}


	/**
	 * Writes a single @a rgba8_t pixel to the output stream.
	 *
	 * NOTE: It is *much* faster to use @a output_pixels instead.
	 */
	inline
	QDataStream &
	operator<<(
			QDataStream &out,
			const rgba8_t &pixel)
	{
		out << static_cast<quint8>(pixel.red)
			<< static_cast<quint8>(pixel.green)
			<< static_cast<quint8>(pixel.blue)
			<< static_cast<quint8>(pixel.alpha);

		return out;
	}

	/**
	 * Reads a single @a rgba8_t pixel from the input stream.
	 *
	 * NOTE: It is *much* faster to use @a input_pixels instead.
	 */
	inline
	QDataStream &
	operator>>(
			QDataStream &in,
			rgba8_t &pixel)
	{
		in >> pixel.red >> pixel.green >> pixel.blue >> pixel.alpha;

		return in;
	}


	/**
	 * Writes a single @a rgba8_t pixel to the output stream.
	 */
	inline
	std::ostream &
	operator<<(
			std::ostream &os,
			const rgba8_t &pixel)
	{
		os << "("
				<< static_cast<int>(pixel.red) << ", "
				<< static_cast<int>(pixel.green) << ", "
				<< static_cast<int>(pixel.blue) << ", "
				<< static_cast<int>(pixel.alpha) << ")";

		return os;
	}


	class Colour
	{
	public:
		/*
		 * Some commonly used colours.
		 *
		 * Note: functions are used instead of non-member variables to
		 * avoid undefined initialisation order of non-local static variables.
		 * For example defining one static variable equal to another.
		 * These functions have a local static colour variable.
		 */
		static const Colour &get_black();
		static const Colour &get_white();
		static const Colour &get_red();
		static const Colour &get_green();

		static const Colour &get_blue();
		static const Colour &get_grey();
		static const Colour &get_silver();
		static const Colour &get_maroon();

		static const Colour &get_purple();
		static const Colour &get_fuchsia();
		static const Colour &get_lime();
		static const Colour &get_olive();

		static const Colour &get_yellow();
		static const Colour &get_navy();
		static const Colour &get_teal();
		static const Colour &get_aqua();

		/**
		 * Indices of the respective colour componets.
		 */
		enum
		{
			RED_INDEX = 0,
			GREEN_INDEX = 1,
			BLUE_INDEX = 2,
			ALPHA_INDEX = 3,
			RGBA_SIZE  // the number of elements in an RGBA colour
		};

		/**
		 * Construct a colour with the given red, green and
		 * blue components.
		 * 
		 * The parameters represent the percentage of red,
		 * green and blue in the resulting colour.
		 * 
		 * The parameters should be in the range 0.0 - 1.0
		 * inclusive.  Values outside this range will not
		 * be clamped, since OpenGL does its own clamping.
		 */
		explicit
		Colour(
				const GLfloat &red = 0.0,
				const GLfloat &green = 0.0,
				const GLfloat &blue = 0.0,
				const GLfloat &alpha = 1.0);

		/**
		 * Construct a Colour from its QColor equivalent.
		 * Note: this is not an explicit constructor by design.
		 */
		Colour(
				const QColor &qcolor);

		Colour(
				const Colour &colour)
		{
			std::copy(colour.d_rgba, colour.d_rgba + RGBA_SIZE, d_rgba);
		}

		/**
		 * Converts the Colour to a QColor.
		 */
		operator QColor() const;

		// Accessor methods

		GLfloat
		red() const
		{
			return d_rgba[RED_INDEX];
		}

		GLfloat
		green() const
		{
			return d_rgba[GREEN_INDEX];
		}

		GLfloat
		blue() const
		{
			return d_rgba[BLUE_INDEX];
		}

		GLfloat
		alpha() const
		{
			return d_rgba[ALPHA_INDEX];
		}

		// Accessor methods which allow modification

		GLfloat &
		red()
		{
			return d_rgba[RED_INDEX];
		}

		GLfloat &
		green()
		{
			return d_rgba[GREEN_INDEX];
		}

		GLfloat &
		blue()
		{
			return d_rgba[BLUE_INDEX];
		}

		GLfloat &
		alpha()
		{
			return d_rgba[ALPHA_INDEX];
		}

		/*
		 * Type conversion operators for integration with
		 * OpenGL colour commands.
		 */

		operator GLfloat*()
		{
			return &d_rgba[0];
		}

		operator const GLfloat*() const
		{
			return &d_rgba[0];
		}

		/**
		 * Linearly interpolate between two colours.
		 * @param first The first colour to mix
		 * @param second The second colour to mix
		 * @param position A value between 0.0 and 1.0 (inclusive), which can be
		 * interpreted as where the returned colour lies in the range between the
		 * first colour and the second colour.
		 */
		static
		Colour
		linearly_interpolate(
				Colour first,
				Colour second,
				double position);

		/**
		 * Converts a CMYK colour to a Colour (which is RGBA). The cyan,
		 * magenta, yellow and black components of the colour must be in the range
		 * 0.0-1.0 inclusive. Alpha value of colour is set to 1.0.
		 */
		static
		Colour
		from_cmyk(
				const CMYKColour &cmyk);

		/**
		 * Converts a Colour (which is RGBA) to CMYK.
		 * @returns CMYK colour, where the component values are between 0.0-1.0 inclusive.
		 */
		static
		CMYKColour
		to_cmyk(
				const Colour &colour);

		/**
		 * Converts a HSV colour to a Colour (which is RGBA). The hue,
		 * saturation, value and alpha components of the colour must be in the range
		 * 0.0-1.0 inclusive.
		 */
		static
		Colour
		from_hsv(
				const HSVColour &hsv);

		/**
		 * Converts a Colour (which is RGBA) to HSV.
		 * @returns HSV colour, where the component values are between 0.0-1.0 inclusive.
		 */
		static
		HSVColour
		to_hsv(
				const Colour &colour);

		/**
		 * Converts an RGBA colour with 8-bit integer components to a Colour (which
		 * uses floating-point values internally).
		 */
		static
		Colour
		from_rgba8(
				const rgba8_t &rgba8);

		/**
		 * Converts a Colour (which uses floating-point values internally) to an RGBA
		 * colour with 8-bit integer components.
		 */
		static
		rgba8_t
		to_rgba8(
				const Colour &colour);

		/**
		 * Converts a QRgb to a Colour, preserving the alpha component.
		 */
		static
		Colour
		from_qrgb(
				const QRgb &rgba);

		/**
		 * Converts a Colour to a QRgb, preserving the alpha component.
		 */
		static
		QRgb
		to_qrgb(
				const Colour &colour);

	private:

		/**
		 * The storage space for the colour components.
		 *
		 * This is an array because that allows it to be
		 * passed to OpenGL as a vector, which is often
		 * faster than passing (and hence copying each
		 * individual component.
		 */
		GLfloat d_rgba[RGBA_SIZE];
	};

	std::ostream &
	operator<<(
			std::ostream &os,
			const Colour &c);
}

#endif  // GPLATES_GUI_COLOUR_H
