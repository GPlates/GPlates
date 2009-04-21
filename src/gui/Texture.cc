/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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
#include <iostream>
#include <cmath>
#include <cstring>

#include <QtOpenGL/qgl.h>
#include <QFileDialog>
#include <QObject>
#include <QString>

#include "maths/LatLonPointConversions.h"
#include "maths/PointOnSphere.h"

#include "OpenGLBadAllocException.h"
#include "OpenGLException.h"
#include "Texture.h"

// RGBA_SIZE is the number of bytes required to store an RGBA value. RGBA is the only format we deal with at the moment. 
const int RGBA_SIZE = 4;

namespace{

	GLuint
	translate_colour_format_to_gl(
		GPlatesPropertyValues::InMemoryRaster::ColourFormat format)
	{
		switch (format) {
		case GPlatesPropertyValues::InMemoryRaster::RgbFormat:
			return GL_RGB;
		case GPlatesPropertyValues::InMemoryRaster::RgbaFormat:
			return GL_RGBA;
		}

		// As long as the supplied colour format is valid (and this function is kept up-to-date
		// with GL-format values which correspond to the abstract colour format values), the end of
		// the function will never be reached.  However, since we unfortunately can't express this
		// to GCC, and GCC hence complains "control reaches end of non-void function", let's just
		// return a value.
		return GL_RGB;
	}

	/**
	 * Rounds up to the nearest power of two. 
	 */
	int
	next_power_of_two(
		int n)
	{
		// Check if it's already a power of 2. 
		if ((n & (n-1)) == 0){
			return n;
		}

		// It's not a power of 2, so find the next power of 2.
		int count = 1;
		while (n > 1)
		{
			n = n / 2;
			count ++;
		}

		return static_cast<int>(pow(2.,count));
	}

	/**
	 * Pads the image out to power of two dimensions without stretching the image data. 
	 */ 
	void
	expand_to_power_two(
		std::vector<unsigned char> &data,
		int image_width,
		int image_height,
		int &texture_width,
		int &texture_height)
	{
		int new_width = next_power_of_two(image_width);
		int new_height = next_power_of_two(image_height);

		std::vector<unsigned char> padded_data;



		try{
			padded_data.resize(new_width * new_height * RGBA_SIZE);
		}
		catch (...)
		{
			std::cerr << "Unable to allocate memory for expanded texture." << std::endl;
			texture_width = image_width;
			texture_height = image_height;
			return;
		}

		texture_width = new_width;
		texture_height = new_height;

		std::vector<unsigned char>::iterator it = data.begin();
		std::vector<unsigned char>::iterator p_it = padded_data.begin();

		int offset = (texture_width - image_width)*RGBA_SIZE;

		int total_width = image_width*RGBA_SIZE;

		for (int h = 0 ; h < image_height ; h++)
		{
			for (int w = 0; w < total_width ; w++)
			{
				*p_it = *it;
				p_it++;
				it++;
			}

			p_it += offset;	
		}

		data.swap(padded_data);
	}

	/**
	 * Pads the image out to power of two dimensions without stretching the image data. 
	 */ 
	void
	expand_to_power_two(
		unsigned char *data,
		std::vector<unsigned char> &expanded_data,
		int image_width,
		int image_height,
		int &texture_width,
		int &texture_height)
	{
		int new_width = next_power_of_two(image_width);
		int new_height = next_power_of_two(image_height);

		try{
			expanded_data.resize(new_width * new_height * RGBA_SIZE);
		}
		catch (...)
		{
			std::cerr << "Unable to allocate memory for expanded texture." << std::endl;
			return;
		}


		texture_width = new_width;
		texture_height = new_height;

		unsigned char *data_ptr = data;
		unsigned char *padded_ptr = &expanded_data[0];

		int offset = (texture_width - image_width)*RGBA_SIZE;

		int total_width = image_width*RGBA_SIZE;

		for (int h = 0 ; h < image_height ; h++)
		{
			std::memcpy(padded_ptr,data_ptr,total_width);
			data_ptr += total_width;
			padded_ptr += total_width + offset;
		}
	}

	/**
	 * Checks if we have enough resources to accomodate the given size and format of texture. 
	 */
	void
	check_texture_size(
		GPlatesPropertyValues::InMemoryRaster::ColourFormat format,
		int width,
		int height)
	{
		GLint width_out;
		glTexImage2D(GL_PROXY_TEXTURE_2D,0,translate_colour_format_to_gl(format),
					width,height,0,translate_colour_format_to_gl(format),GL_UNSIGNED_BYTE,NULL);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,0,GL_TEXTURE_INTERNAL_FORMAT,&width_out);

		if (width_out == 0){
			QString message = QString("Cannot render texture of size %1 by %2").arg(width).arg(height);
			throw GPlatesGui::OpenGLException(GPLATES_EXCEPTION_SOURCE,
					message.toStdString().c_str());
		}
	}
		

	void
	clear_gl_errors()
	{	
		while (glGetError() != GL_NO_ERROR)
		{};
	}

	GLenum
	check_gl_errors(
		GLuint &texture_name,
		const char *message = "")
	{
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			std::cout << message << ": openGL error: " << gluErrorString(error) << std::endl;
			glDeleteTextures(1,&texture_name);
			throw GPlatesGui::OpenGLException(GPLATES_EXCEPTION_SOURCE,
					"OpenGL error in Texture.cc");
		}
		return error;
	}

	void
	check_glu_errors(
		GLuint error,
		GLuint &texture_name)
	{
		if (error == GLU_OUT_OF_MEMORY)
		{
			glDeleteTextures(1,&texture_name);
			throw GPlatesGui::OpenGLBadAllocException(GPLATES_EXCEPTION_SOURCE,
					"There was insufficient memory to load the requested texture.");
		}
		if (error != GL_NO_ERROR)
		{
			std::cout << " GLU error: " << gluErrorString(error) << std::endl;
			glDeleteTextures(1,&texture_name);
			throw GPlatesGui::OpenGLException(GPLATES_EXCEPTION_SOURCE,
					"GLU error in Texture.cc");
		}
	}

}




GPlatesGui::Texture::~Texture()
{
	if (d_image_data != NULL){
		delete[] d_image_data;
	}
	glDeleteTextures(1,&d_texture_name);
}

void
GPlatesGui::Texture::generate_test_texture()
{
	if (d_image_data != NULL){
		delete[] d_image_data;
	}
	glDeleteTextures(1,&d_texture_name);

	d_image_width = 2048;
	d_image_height = 1024;

	int size = d_image_width*d_image_height;

	d_image_data = new GLubyte[size*4];

	GLubyte *image_ptr = d_image_data;
	int i,j;
	int a;

	GLubyte value1, value2;

	for(i = 0; i < d_image_height; i++)
	{
		int check = (i/16)%2;
		if (check == 0){
			value1 = 0;
			value2 = 255;
		}
		else{
			value1 = 255;
			value2 = 0;
		}
		for(j = 0; j < d_image_width; j+=16)
		{	
			for(a = 0; a < 8; a++)
			{
				*image_ptr++ = value1;
				*image_ptr++ = value1;
				*image_ptr++ = value1;
				*image_ptr++ = value1;
			}
			for(a = 0; a < 8; a++)
			{
				*image_ptr++ = value2;
				*image_ptr++ = value2;
				*image_ptr++ = value2;
				*image_ptr++ = value2;
			}
		}
	}

	glGenTextures(1,&d_texture_name);
	glBindTexture(GL_TEXTURE_2D, d_texture_name);



	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE,d_image_width,d_image_height,GL_LUMINANCE,GL_UNSIGNED_BYTE,d_image_data);

	glEnable(GL_TEXTURE_2D);

	delete[] d_image_data;
	d_image_data = NULL;
}


GLuint
GPlatesGui::Texture::get_texture_name()
{
	return d_texture_name;
}

void
GPlatesGui::Texture::paint()
{
	if(d_enabled){

		glEnable(GL_TEXTURE_2D);
		
		glBindTexture(GL_TEXTURE_2D, d_texture_name);

		int i,j;
		for (j = 0; j < NUM_STRIPS_T ; j++)
		{
			glBegin(GL_QUAD_STRIP);

			for ( i = 0 ; i < NUM_STRIPS_S ; i++)
			{
				glTexCoord2f(d_texture_vertices[i][j].s, 
							d_texture_vertices[i][j].t);

				glVertex3f(d_sphere_vertices[i][j].x,
							d_sphere_vertices[i][j].y,
							d_sphere_vertices[i][j].z);

				glTexCoord2f(d_texture_vertices[i][j+1].s, 
							d_texture_vertices[i][j+1].t);

				glVertex3f(d_sphere_vertices[i][j+1].x,
							d_sphere_vertices[i][j+1].y,
							d_sphere_vertices[i][j+1].z);

				glTexCoord2f(d_texture_vertices[i+1][j].s, 
							d_texture_vertices[i+1][j].t);

				glVertex3f(d_sphere_vertices[i+1][j].x,
							d_sphere_vertices[i+1][j].y,
							d_sphere_vertices[i+1][j].z);

				glTexCoord2f(d_texture_vertices[i+1][j+1].s, 
							d_texture_vertices[i+1][j+1].t);

				glVertex3f(d_sphere_vertices[i+1][j+1].x,
							d_sphere_vertices[i+1][j+1].y,
							d_sphere_vertices[i+1][j+1].z);
							
			}
			glEnd();
		}

		glDisable(GL_TEXTURE_2D);


	}
	else{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void
GPlatesGui::Texture::toggle()
{
	d_enabled = !d_enabled;
}


void
GPlatesGui::Texture::generate_raster(
	std::vector<unsigned_byte_type> &data, 
	QSize &size, 
	ColourFormat format)
{
	clear_gl_errors();
	glDeleteTextures(1,&d_texture_name);
	glGenTextures(1,&d_texture_name);
	glBindTexture(GL_TEXTURE_2D, d_texture_name);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);


	d_image_width = size.width();
	d_image_height = size.height();

	generate_mapping_coordinates(d_extent);

	check_gl_errors(d_texture_name);

	GLenum error = gluBuild2DMipmaps(GL_TEXTURE_2D,translate_colour_format_to_gl(format),d_image_width,d_image_height,
		translate_colour_format_to_gl(format),GL_UNSIGNED_BYTE,&data[0]);

	check_glu_errors(error,d_texture_name);

	check_gl_errors(d_texture_name);
}


void
GPlatesGui::Texture::generate_raster(
	unsigned_byte_type *data, 
	QSize &size, 
	ColourFormat format)
{
	clear_gl_errors();

	glDeleteTextures(1,&d_texture_name);
	glGenTextures(1,&d_texture_name);
	glBindTexture(GL_TEXTURE_2D, d_texture_name);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

	d_image_width = size.width();
	d_image_height = size.height();

	GLint max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_texture_size);

	generate_mapping_coordinates(d_extent);

	check_gl_errors(d_texture_name);

	GLenum error = GL_NO_ERROR;

	error = gluBuild2DMipmaps(GL_TEXTURE_2D,translate_colour_format_to_gl(format),d_image_width,d_image_height,
		translate_colour_format_to_gl(format),GL_UNSIGNED_BYTE,data);

	check_glu_errors(error,d_texture_name);

	check_gl_errors(d_texture_name);
}


void
GPlatesGui::Texture::generate_mapping_coordinates()
{
	float s_coord, t_coord;
	float lat, lon;
	for (int j = 0; j <= NUM_STRIPS_T ; j++)
	{
		t_coord = static_cast<float>(j) / static_cast<float>(NUM_STRIPS_T);
		lat = LAT_START + (LAT_END-LAT_START)*t_coord;
		for (int i = 0; i <= NUM_STRIPS_S ; i++)
		{
			s_coord = static_cast<float>(i) / static_cast<float>(NUM_STRIPS_S);
			lon = LON_START + (LON_END-LON_START)*s_coord;

			GPlatesMaths::LatLonPoint latlon(lat,lon);
			GPlatesMaths::PointOnSphere p = 
				GPlatesMaths::make_point_on_sphere(latlon);

			d_texture_vertices[i][j].s = s_coord;
			d_texture_vertices[i][j].t = t_coord;

			d_sphere_vertices[i][j].x = static_cast<float>(p.position_vector().x().dval());
			d_sphere_vertices[i][j].y = static_cast<float>(p.position_vector().y().dval());
			d_sphere_vertices[i][j].z = static_cast<float>(p.position_vector().z().dval());
		}
	}
}


void
GPlatesGui::Texture::generate_mapping_coordinates(
	QRectF &extent)
{

	float lon_start = extent.left();
	float lat_start = extent.bottom();
	float width = extent.width();
	float height = extent.height();

	// The height, as stored in the QRectF d_extent, should be negative. 
	// We want to make it positive for the mapping algorithm below. 
	//
	// A positive height in the QRectF corresponds to an invalid range for mapping - 
	// unless we interpret it as a request to vertically flip the image. I'm not 
	// going to allow vertical flipping, and so if we get a positive height, return. 
	if (height < 0.){
		height = -height;
	}
	else
	{
		return;
	}

	// A negative width means we've gone over the date line, and that's OK, we
	// just need to correct it. 
	if (width < 0.) 
	{
		width += 360.;
	};

	// A width > 360 means we've gone round the globe more than once. 
	if (width > 360.)
	{
		width -= 360.;
	}

	// FIXME: Do some optimisation on these loops...
	float s_coord, t_coord;
	float lat, lon;
	for (int j = 0; j <= NUM_STRIPS_T ; j++)
	{
		t_coord = static_cast<float>(j) / static_cast<float>(NUM_STRIPS_T);
		lat = lat_start + height*t_coord;
		for (int i = 0; i <= NUM_STRIPS_S ; i++)
		{
			s_coord = static_cast<float>(i) / static_cast<float>(NUM_STRIPS_S);
			lon = lon_start + width*s_coord;

			GPlatesMaths::LatLonPoint latlon(lat,lon);
			GPlatesMaths::PointOnSphere p = 
				GPlatesMaths::make_point_on_sphere(latlon);

			d_texture_vertices[i][j].s = s_coord;
			d_texture_vertices[i][j].t = t_coord;

			d_sphere_vertices[i][j].x = static_cast<float>(p.position_vector().x().dval());
			d_sphere_vertices[i][j].y = static_cast<float>(p.position_vector().y().dval());
			d_sphere_vertices[i][j].z = static_cast<float>(p.position_vector().z().dval());
		}
	}
}


void
GPlatesGui::Texture::remap_texture()
{
	// Check that a texture already exists. 
	if (!texture_is_loaded()) return;

	clear_gl_errors();

	// The dimensions of the image in texture memory may differ from those of the original image, 
	// so we need to query openGL to get the current width and height. 
	GLint width, height;
	
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);

	// Allocate memory to store the image which we'll later grab from texture memory
	std::vector<unsigned_byte_type> texture_data(width*height*RGBA_SIZE);

	// Grab the texture from texture memory, stick it in texture_data.
	glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,&texture_data[0]);


	glDeleteTextures(1,&d_texture_name);
	glGenTextures(1,&d_texture_name);
	glBindTexture(GL_TEXTURE_2D, d_texture_name);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

	generate_mapping_coordinates(d_extent);

	check_gl_errors(d_texture_name);

	GPlatesPropertyValues::InMemoryRaster::ColourFormat format =
		GPlatesPropertyValues::InMemoryRaster::RgbaFormat;

	GLenum error = gluBuild2DMipmaps(GL_TEXTURE_2D,translate_colour_format_to_gl(format),width,height,
		translate_colour_format_to_gl(format),GL_UNSIGNED_BYTE,&texture_data[0]);

	check_glu_errors(error,d_texture_name);

	check_gl_errors(d_texture_name);

}

void
GPlatesGui::Texture::set_extent(
	const QRectF &rect)
{
	d_extent = rect;

	// Seeing as we've changed the extent, check if a texture is loaded. If there is, re-map it.
	if (texture_is_loaded())
	{
		remap_texture();
	}
}

bool
GPlatesGui::Texture::texture_is_loaded()
{

	if (! glIsTexture(d_texture_name))
	{
		return false;
	}

	GLint width, height;
	
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);

	if ((width == 0) || (height == 0))
	{
		return false;
	}

	return true;
}
