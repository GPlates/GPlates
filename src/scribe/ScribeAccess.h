/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEACCESS_H
#define GPLATES_SCRIBE_SCRIBEACCESS_H

#include <exception>
#include <new>
#include <vector>
#include <boost/mpl/bool.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/enum_shifted.hpp>
#include <boost/preprocessor/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/ref.hpp>

#include "TranscribeResult.h"

#include "global/CompilerWarnings.h"


/**
 * The maximum number of object constructor arguments supported in Access::construct_object().
 */
#define GPLATES_SCRIBE_ACCESS_CONSTRUCT_MAX_CONSTRUCTOR_ARGS 10


namespace GPlatesScribe
{
	// Forward declarations.
	template <typename ObjectType> class ConstructObject;
	class ExportClassType;
	class Scribe;


	/**
	 * A central place for client classes to befriend in order for the scribe system
	 * to privately access client classes.
	 */
	class Access
	{
	private:

		//
		// Friends that can access the private methods below.
		//

		template <typename ObjectType>
		friend TranscribeResult transcribe(Scribe &, ObjectType &, bool);

		template <typename ObjectType>
		friend TranscribeResult transcribe_construct_data(Scribe &, ConstructObject<ObjectType> &);

		template <typename ObjectType>
		friend TranscribeResult transcribe_construct_data_impl(Scribe &, ConstructObject<ObjectType> &, boost::mpl::true_);

		template <typename ObjectType>
		friend void relocated(Scribe &, const ObjectType &, const ObjectType &);

		template <typename ObjectType>
		friend void relocated_impl(Scribe &, const ObjectType &, const ObjectType &, boost::mpl::true_);

		template <typename ObjectType>
		friend class ConstructObject;

		friend class Scribe;


		//
		// Methods that can privately access client classes provided those classes declare:
		//
		//   friend class GPlatesScribe::Access;
		//


		template <typename ObjectType>
		static
		TranscribeResult
		transcribe(
				Scribe &scribe,
				ObjectType &object,
				bool transcribed_construct_data)
		{
			// If you get a compile-time error here then you need to either:
			//   (1) Provide a specialisation, or overload, of...
			//   
			//          template <typename ObjectType>
			//          TranscribeResult GPlatesScribe::transcribe(Scribe &, ObjectType &, bool);
			//          
			//       ...to match your 'ObjectType' class (see "Transcribe.h"), or
			//       
			//   (2) Provide a *private* 'transcribe()' method in your 'ObjectType' class and
			//       make the scribe system a friend of your 'ObjectType' class using:
			//       
			//          friend class GPlatesScribe::Access;
			//
			return object.transcribe(scribe, transcribed_construct_data);
		}


		template <typename ObjectType>
		static
		TranscribeResult
		transcribe_construct_data(
				Scribe &scribe,
				ConstructObject<ObjectType> &object)
		{
			// If you get a compile-time error here then it's likely that the scribe system
			// had not been made a friend of class 'ObjectType' using the following in 'ObjectType':
			//       
			//          friend class GPlatesScribe::Access;
			//
			return ObjectType::transcribe_construct_data(scribe, object);
		}


		template <typename ObjectType>
		static
		void
		relocated(
				Scribe &scribe,
				const ObjectType &relocated_object,
				const ObjectType &transcribed_object)
		{
			// If you get a compile-time error here then it's likely that the scribe system
			// had not been made a friend of class 'ObjectType' using the following in 'ObjectType':
			//       
			//          friend class GPlatesScribe::Access;
			//
			ObjectType::relocated(scribe, relocated_object, transcribed_object);
		}


		// Disable MSVC warning C4345:
		//
		//   "behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized"
		//
		// ...that happens when 'new (object) ObjectType()' is called on a POD type.
		//
		PUSH_MSVC_WARNINGS
		DISABLE_MSVC_WARNING(4345)

		template <typename ObjectType>
		static
		void
		construct_object(
				ObjectType *object)
		{
			// Note: If you get a compile-time error here then it's likely that 'ObjectType'
			// does not have a default constructor and you will need to provide a specialisation or
			// overload of the non-member function...
			//   
			//      template <typename ObjectType>
			//      TranscribeResult
			//      GPlatesScribe::transcribe_construct_data(
			//				Scribe &,
			//				ConstructObject<ObjectType> &);
			// 
			// ...or implement a static method in the 'ObjectType' class with the following signature...
			//
			//		static
			//		TranscribeResult
			//		ObjectType::transcribe_construct_data(
			//				Scribe &,
			//				ConstructObject<ObjectType> &);
			//
			// ...see "Transcribe.h" for more details.
			//
			//
			// NOTE: If you have already done one of the above then check that the function signature
			// is correct. For example, you may have the wrong 'ObjectType' in the
			// 'ConstructObject<ObjectType> &' part of the signature, or you may have missed
			// adding the 'static' keyword when using the static method approach.
			//
			new (object) ObjectType();
		}

		POP_MSVC_WARNINGS


		//
		// The following preprocessor macros generate the following code:
		//
		//    template <typename ObjectType, typename A1>
		//    static
		//    void
		//    construct_object(
		//        ObjectType *object,
		//        const A1 &a1);
		//
		//    template <typename ObjectType, typename A1, typename A2>
		//    static
		//    void
		//    construct_object(
		//        ObjectType *object,
		//        const A1 &a1,
		//        const A2 &a2);
		//
		//    template <typename ObjectType, typename A1, typename A2, typename A3>
		//    static
		//    void
		//    construct_object(
		//        ObjectType *object,
		//        const A1 &a1,
		//        const A2 &a2,
		//        const A3 &a3);
		//
		//    ...ETC
		//

		#define GPLATES_SCRIBE_ACCESS_CONSTRUCT_OBJECT_PARAM(z, i, _) \
				BOOST_PP_CAT(const A, i) &BOOST_PP_CAT(a, i)

		#define GPLATES_SCRIBE_ACCESS_CONSTRUCT_OBJECT(z, n, _) \
				template < \
						typename ObjectType, \
						BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), typename A)> \
				static \
				void \
				construct_object( \
						ObjectType *object, \
						BOOST_PP_ENUM_SHIFTED(BOOST_PP_INC(n), GPLATES_SCRIBE_ACCESS_CONSTRUCT_OBJECT_PARAM, _) ) \
				{ \
					new (object) ObjectType( \
							BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), a)); \
				} \
				/**/

		// Create 'construct_object()' overloads.
		BOOST_PP_REPEAT_FROM_TO( \
				1, \
				BOOST_PP_INC(GPLATES_SCRIBE_ACCESS_CONSTRUCT_MAX_CONSTRUCTOR_ARGS), \
				GPLATES_SCRIBE_ACCESS_CONSTRUCT_OBJECT, \
				_)

		#undef GPLATES_SCRIBE_ACCESS_CONSTRUCT_OBJECT_PARAM
		#undef GPLATES_SCRIBE_ACCESS_CONSTRUCT_OBJECT



		//
		// The following are implementation details that enable us to provide meta-functions that
		// check if class 'ObjectType' has particular members.
		//
		// The implementation is based on:
		//    http://stackoverflow.com/questions/257288/is-it-possible-to-write-a-c-template-to-check-for-a-functions-existence/264088#264088
		//

		typedef char yes[1];
		typedef char no[2];
		
		template <typename U, U>
		struct TypeCheck;
		

		template <typename ObjectType>
		static
		yes &
		check_transcribe_construct_data(
				TypeCheck<
						//
						// Signature and static method name matching...
						//
						//		static
						//		TranscribeResult
						//		ObjectType::transcribe_construct_data(
						//				Scribe &,
						//				ConstructObject<ObjectType> &);
						//
						// NOTE: If you get a compile-time error here then it's likely that the scribe system
						// had not been made a friend of class 'ObjectType' using the following in 'ObjectType':
						//       
						//          friend class GPlatesScribe::Access;
						//
						TranscribeResult (*)(Scribe &, ConstructObject<ObjectType> &),
						&ObjectType::transcribe_construct_data
				> *);
		
		template <typename ObjectType>
		static
		no &
		check_transcribe_construct_data(...);

		
		template <typename ObjectType>
		static
		yes &
		check_relocated(
				TypeCheck<
						//
						// Signature and static method name matching...
						//
						//		static
						//		void
						//		ObjectType::relocated(
						//				Scribe &,
						//				const ObjectType &,
						//				const ObjectType &);
						//
						// NOTE: If you get a compile-time error here then it's likely that the scribe system
						// had not been made a friend of class 'ObjectType' using the following in 'ObjectType':
						//       
						//          friend class GPlatesScribe::Access;
						//
						void (*)(Scribe &, const ObjectType &, const ObjectType &),
						&ObjectType::relocated
				> *);
		
		template <typename ObjectType>
		static
		no &
		check_relocated(...);


		/**
		 * A meta-function that checks if class 'ObjectType' has the following static method:
		 *
		 *		static
		 *		TranscribeResult
		 *		ObjectType::transcribe_construct_data(
		 *				Scribe &,
		 *				ConstructObject<ObjectType> &);
		 *
		 * Note: Only class Access can form the expression '&ObjectType::transcribe_construct_data'
		 * because that static method is private to ObjectType and only class Access can
		 * privately access class ObjectType. Therefore we have to make the
		 * 'check_transcribe_construct_data()' function a member of class Access (ie, we cannot
		 * encapsulate it nicely within this meta-function class).
		 */
		template <typename ObjectType>
		struct HasStaticMemberTranscribeConstructData
		{
			//
			// NOTE: If you get a compile-time error here then it's likely that the scribe system
			// had not been made a friend of class 'ObjectType' using the following in 'ObjectType':
			//       
			//          friend class GPlatesScribe::Access;
			//
			static const bool value =
					sizeof(Access::check_transcribe_construct_data<ObjectType>(0)) == sizeof(Access::yes);

			typedef boost::mpl::bool_<value> type;
		};


		/**
		 * A meta-function that checks if class 'ObjectType' has the following static method:
		 *
		 *		static
		 *		void
		 *		ObjectType::relocated(
		 *				Scribe &,
		 *				const ObjectType &,
		 *				const ObjectType &);
		 *
		 * Note: Only class Access can form the expression '&ObjectType::relocated'
		 * because that static method is private to ObjectType and only class Access can
		 * privately access class ObjectType. Therefore we have to make the
		 * 'check_relocated()' function a member of class Access (ie, we cannot
		 * encapsulate it nicely within this meta-function class).
		 */
		template <typename ObjectType>
		struct HasStaticMemberRelocated
		{
			//
			// NOTE: If you get a compile-time error here then it's likely that the scribe system
			// had not been made a friend of class 'ObjectType' using the following in 'ObjectType':
			//       
			//          friend class GPlatesScribe::Access;
			//
			static const bool value =
					sizeof(Access::check_relocated<ObjectType>(0)) == sizeof(Access::yes);

			typedef boost::mpl::bool_<value> type;
		};


		/**
		 * Typedef for a sequence of export registered classes.
		 */
		typedef std::vector< boost::reference_wrapper<const ExportClassType> > export_registered_classes_type;

		/**
		 * Static variable to force classes to be exported registered at program startup.
		 */
		static const export_registered_classes_type EXPORT_REGISTERED_CLASSES;

		/**
		 * Static method used to initialise @a EXPORT_REGISTERED_CLASSES.
		 */
		static
		export_registered_classes_type
		export_register_classes();

	};
}

#endif // GPLATES_SCRIBE_SCRIBEACCESS_H
