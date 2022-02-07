/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEEXPORTREGISTRATION_H
#define GPLATES_SCRIBE_SCRIBEEXPORTREGISTRATION_H

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include "ScribeExportRegistry.h"


/**
 * Export registration enables a class or type to be transcribed (internally by the scribe implementation).
 *
 * The following classes/types should be export registered:
 *  1) All non-abstract *polymorphic* classes (ie, have 'virtual' methods) and that get transcribed, and
 *  2) All types used in any boost::variant objects that get transcribed.
 *
 * For example, export registration is required when a base class pointer to a derived class object
 * is transcribed because otherwise, when the transcribed archive is loaded, the type of derived
 * class object to load will not be known.
 *
 * Normally only objects (instances of classes/types) are transcribed but the class/type is not.
 * So export registration of classes/types is needed when that is insufficient (in the cases above).
 *
 * NOTE: Abstract classes (with at least one *pure* virtual method) should not be registered since
 * they cannot be instantiated.
 * If you do then you'll get a compile-time error in 'ExportRegistry::register_class_type()'.
 * For example, in the following code *only* class MyDerived should be registered:
 *
 *		class MyBase
 *		{
 *		public:
 *			virtual ~MyBase() { }
 *			virtual void do() = 0;
 *		};
 *   
 *		class MyDerived :
 *			public MyBase
 *		{
 *		public:
 *			virtual void do() { ... }
 *		};
 *
 *
 * Note that if your class is a *private* nested class then you'll need to make the scribe system a
 * friend of the parent class using:
 *
 *		friend class GPlatesScribe::Access;
 *
 * ...for example...
 *
 *		class Parent
 *		{
 *		public:
 *		...
 *		private:
 *			class MyClass { ... };
 *			
 *			friend class GPlatesScribe::Access;
 *		};
 *
 *
 * You'll notice that many non-polymorphic types (like the fundamental integer/float types) are
 * listed in 'SCRIBE_EXPORT_EXTERNAL'. This is in case they are used inside a transcribed boost::variant.
 *
 *
 *
 * A macro should be defined in each program (eg, gplates and gplates-unit-test) and in each
 * dynamic/shared library (eg, pygplates) that groups together all sub-directory registrations.
 * For example:
 *
 *	#define SCRIBE_EXPORT_GPLATES \
 *		SCRIBE_EXPORT_APP_LOGIC \
 *		SCRIBE_EXPORT_FILE_IO \
 *		...
 *		SCRIBE_EXPORT_EXTERNAL
 *
 * And each source code sub-directory that export registers classes for the scribe system should
 * define a macro named 'SCRIBE_EXPORT_<sub-dir-name>' that contains a list of those classes and
 * types that need to be export registered by the scribe system.
 * For example:
 *
 *	#define SCRIBE_EXPORT_APP_LOGIC \
 *		\
 *		((GPlatesAppLogic::MyClassA, "MyClassA")) \
 *		\
 *		((GPlatesAppLogic::MyClassB, "MyClassB")) \
 *		\
 *		...
 *
 * To register a class or type, add a line to an export macro that looks like:
 *
 *     ((<ClassType>, <ClassIdName>)) \
 *
 * ...where <ClassType> is replaced by your class or type, and <ClassIdName> is replaced by a
 * unique string identifier.
 * For example, to register class MyClass add the following:
 *
 *     ((MyClass, "MyClass")) \
 *
 * NOTE: The <ClassIdName> identifier should be unique across all registrations.
 * In other words, no two registrations should have the same <ClassIdName> identifier.
 *
 * WARNING: Once you've been using a particular <ClassIdName> identifier to generate transcriptions
 * you should not change it later on, even if you move your class to a different namespace or
 * change the name of your class. For example if you changed class MyClass to class YourClass then
 * you should still try to register as:
 *
 *     ((YourClass, "MyClass")) \
 *
 * ...because changing the identifier will break backward/forward compatibility between GPlates releases.
 * This is because the identifier is written to, and read from, the transcription (archive).
 *
 *
 * NOTE: The reason for the export macro format is it is a boost preprocessor 'sequence' that
 * is used by class Access to export register these types at program startup.
 * A boost preprocessor sequence looks like '(a)(b)(c)'.
 * In our case the sequence elements are boost preprocessor 2-tuples where a 2-tuple looks like '(x,y)'.
 * So a sequence of 2-tuples looks like '((ax,ay))((bx,by))((cx,cy))'.
 */



/**
 * This macro should be used in a '.cc' file associated with the program being compiled/linked.
 *
 * It uses, for example, the 'SCRIBE_EXPORT_GPLATES' macro in the example above which contains all
 * the scribe export registered classes/types for the 'gplates' program.
 *
 * For example, the 'gplates' program might have a "ScribeExportGPlates.cc" file that is only
 * compiled into the 'gplates' executable (and not compiled into others like 'gplates-unit-test').
 * And this file might be similar to the following:
 *
 *
 *	#include "app-logic/ScribeExportAppLogic.h"
 *
 *	#include "file-io/ScribeExportFileIO.h"
 *
 *	...
 *
 *	#include "scribe/ScribeExportExternal.h"
 *	#include "scribe/ScribeExportRegistration.h"
 *
 *
 *	#define SCRIBE_EXPORT_GPLATES \
 *		SCRIBE_EXPORT_APP_LOGIC \
 *		SCRIBE_EXPORT_FILE_IO \
 *		...
 *		SCRIBE_EXPORT_EXTERNAL
 *
 *	SCRIBE_EXPORT_REGISTRATION(SCRIBE_EXPORT_GPLATES)
 *
 */
#define SCRIBE_EXPORT_REGISTRATION(scribe_export_sequence) \
		GPlatesScribe::Access::export_registered_classes_type \
		GPlatesScribe::Access::export_register_classes() \
		{ \
			export_registered_classes_type export_registered_classes; \
			\
			\
			/** \
			 * Create a group of export registration classes for class types/ids defined in a boost \
			 * preprocessor sequence that looks like: \
			 * \
			 *		((MyClassA, "MyClassA"))((MyClassB, "MyClassB"))... \
			 * \
			 * ...which defines the following export registration calls... \
			 * \
			 *		export_registered_classes.push_back( \
			 *				boost::cref( \
			 *						ExportRegistry::instance().register_class_type<MyClassA>("MyClassA"))); \
			 * \
			 *		export_registered_classes.push_back( \
			 *				boost::cref( \
			 *						ExportRegistry::instance().register_class_type<MyClassB>("MyClassB"))); \
			 * \
			 */ \
			BOOST_PP_SEQ_FOR_EACH( \
					GPLATES_ACCESS_EXPORT_REGISTER_CLASS_TYPE_MACRO, \
					_, \
					scribe_export_sequence) \
			\
			\
			return export_registered_classes; \
		}


/**
 * Only class Access can form the expression 'register_class_type<class_type>' because
 * 'class_type' might be a private nested class of a parent class and only class Access
 * can privately access that parent class (assuming it has a friend declaration).
 *
 * Note: We use macros to create the necessary registrations because 'class_type'
 * cannot be passed into this function (via a template type parameter) from outside
 * because that would require the caller to also be a friend of the parent class.
 */
#define GPLATES_ACCESS_EXPORT_REGISTER_CLASS_TYPE( \
				class_type, \
				class_id_name) \
		\
		export_registered_classes.push_back( \
				boost::cref( \
						GPlatesScribe::ExportRegistry::instance().register_class_type<class_type>(class_id_name)));

#define GPLATES_ACCESS_EXPORT_REGISTER_CLASS_TYPE_MACRO(r, data, elem) \
		GPLATES_ACCESS_EXPORT_REGISTER_CLASS_TYPE( \
				BOOST_PP_TUPLE_ELEM(2, 0, elem), \
				BOOST_PP_TUPLE_ELEM(2, 1, elem))


#endif // GPLATES_SCRIBE_SCRIBEEXPORTREGISTRATION_H
