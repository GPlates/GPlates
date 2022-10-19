/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSHADERSOURCE_H
#define GPLATES_OPENGL_GLSHADERSOURCE_H

#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <QByteArray>
#include <QFile>
#include <QString>


namespace GPlatesOpenGL
{
	/**
	 * A convenience class to handle shader source code segments and whether the individual
	 * code segments come from a string or a file (useful for logging failed compiles/links).
	 *
	 * One or more shader source code segments can be grouped together before they are compiled.
	 *
	 * NOTE: The "#version 450 core" directive (GLSL equivalent of OpenGL 4.5 core profile) is internally
	 *       specified as an extra shader segment that is internally added as the first code segment.
	 *       
	 *       This is because the "#version" directive must come before any non-commented source code
	 *       (which otherwise would be difficult with multiple source code segments where usually the
	 *       "#version" directive is placed in the segment defining the 'main()' shader function and
	 *       this usually is the last segment because it uses other shader segments and hence they
	 *       must be defined first).
	 *       
	 *       So the solution used here is this class will create a "#version" shader segment and add it as
	 *       the first shader segment which means it should not be defined in any supplied shader segments.
	 */
	class GLShaderSource
	{
	public:

		/**
		 * The filename of the shader source file (Qt resource) containing shader utilities.
		 */
		static const QString UTILS_FILE_NAME;


		/**
		 * Represents information of a shader code segment.
		 */
		struct CodeSegment
		{
			explicit
			CodeSegment(
					const QByteArray &source_code_,
					boost::optional<QString> source_file_name_ = boost::none);

			QByteArray source_code;

			//! Number of lines is at least one.
			unsigned int num_lines;

			//! Source filename is valid if source code loaded from a file, otherwise was loaded from a string.
			boost::optional<QString> source_file_name;
		};


		/**
		 * Creates a @a GLShaderSource object when only a single shader source, from a file, is required.
		 */
		static
		GLShaderSource
		create_shader_source_from_file(
				const QString& shader_source_file_name);


		//! Default constructor contains no shader source.
		explicit
		GLShaderSource();

		/**
		 * Implicit converting constructor when only a single shader source is required.
		 *
		 * Note that the @a shader_source char array is copied internally, so it doesn't have to
		 * remain in existence after this call.
		 *
		 * Note: Do not specify the "#version" directive in the shader source.
		 */
		GLShaderSource(
				const char *shader_source);

		/**
		 * Implicit converting constructor when only a single shader source is required.
		 *
		 * Note: Do not specify the "#version" directive in the shader source.
		 */
		GLShaderSource(
				const QByteArray &shader_source);


		/**
		 * Adds a shader source code segment.
		 *
		 * Note: Do not specify the "#version" directive in the shader source.
		 */
		void
		add_code_segment(
				const char *shader_source);

		/**
		 * Adds a shader source code segment.
		 *
		 * Note: Do not specify the "#version" directive in the shader source.
		 */
		void
		add_code_segment(
				const QByteArray &shader_source);

		/**
		 * Adds a shader source code segment from a file.
		 *
		 * Note: Do not specify the "#version" directive in the shader source.
		 */
		void
		add_code_segment_from_file(
				const QString& shader_source_file_name);


		/**
		 * Returns all shader source code segments.
		 *
		 * This includes the initial (first) segment containing the #version string and
		 * any #extension strings found in subsequently added code segments.
		 * Note that any #extension strings are copied to the initial segment and commented out of the
		 * code segment they belong to. This is because #extension must not occur *after* any
		 * non-preprocessor source code.
		 *
		 * Each code segment is guaranteed to have at least one line.
		 */
		std::vector<CodeSegment>
		get_code_segments() const;

	private:

		/**
		 * Shader source version string.
		 *
		 * Use OpenGL 4.5 core which in GLSL is "#version 450 core".
		 */
		static const char *SHADER_VERSION_STRING;


		//! Code segment containing #version and any #extension found in code segments added by client.
		CodeSegment d_initial_code_segment;

		//! Code segments added by client.
		std::vector<CodeSegment> d_added_code_segments;

		/**
		 * Do any processing of the code segment and then add it to our internal sequence.
		 *
		 * Processing includes: any #extension strings are copied to the initial segment and
		 * commented out of the code segment they belong to. This is because #extension must not
		 * occur *after* any non-preprocessor source code.
		 */
		void
		add_processed_code_segment(
				QByteArray source_code,
				boost::optional<QString> source_file_name = boost::none);
	};
}

#endif // GPLATES_OPENGL_GLSHADERSOURCE_H
