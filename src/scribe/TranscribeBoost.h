/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBEBOOST_H
#define GPLATES_SCRIBE_TRANSCRIBEBOOST_H

#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/weak_ptr.hpp>

#include "ScribeExceptions.h"
#include "ScribeExportRegistry.h"
#include "ScribeInternalAccess.h"
#include "Transcribe.h"
#include "TranscribeSmartPointerProtocol.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	//
	// Boost library specialisations and overloads of the transcribe function.
	//

	class Scribe;

	//! Transcribe boost::intrusive_ptr.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::intrusive_ptr<T> &intrusive_ptr_object,
			bool transcribed_construct_data);

	//! Transcribe boost::optional containing non-reference object.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::optional<T> &optional_object,
			bool transcribed_construct_data);

	//! Transcribe boost::optional containing reference to an object.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::optional<T &> &optional_object_reference,
			bool transcribed_construct_data);

	//! Relocated boost::optional containing non-reference object.
	template <typename T>
	void
	relocated(
			Scribe &scribe,
			const boost::optional<T> &relocated_optional_object,
			const boost::optional<T> &transcribed_optional_object);

	//! Relocated boost::optional containing reference to an object.
	template <typename T>
	void
	relocated(
			Scribe &scribe,
			const boost::optional<T &> &relocated_optional_object,
			const boost::optional<T &> &transcribed_optional_object);

	//! Transcribe boost::scoped_ptr.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::scoped_ptr<T> &scoped_ptr_object,
			bool transcribed_construct_data);

	//! Transcribe boost::shared_ptr.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::shared_ptr<T> &shared_ptr_object,
			bool transcribed_construct_data);

	//! Transcribe boost::weak_ptr.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::weak_ptr<T> &weak_ptr_object,
			bool transcribed_construct_data);

	/**
	 * Transcribe boost::variant.
	 *
	 * NOTE: All stored types in the boost::variant must be registered in 'ScribeExportRegistration.h'.
	 *
	 * A boost::variant instantiation is only default constructable if its first type is default constructable.
	 * If a boost::variant instantiation is default-constructable then it can be transcribed with or
	 * without save/load construction. An example without save/load construct is:
	 * 
	 *		boost::variant<...> x;
	 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, x, "x"))
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 * 
	 * ...but if it is not default constructable then it must be transcribed using save/load construction
	 * or initialised with a dummy value (and then transcribing).
	 * For example:
	 * 
	 *		GPlatesScribe::LoadRef< boost::variant<...> > x =
	 *		   scribe.load< boost::variant<...> >(TRANSCRIBE_SOURCE, "x");
	 *		if (!x.is_valid())
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 * 
	 * ...or...
	 * 
	 *		boost::variant<...> x(dummy_value);
	 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, x, "x"))
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 */
	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &variant_object,
			bool transcribed_construct_data);

	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> > &variant_object);


	// We don't need to relocate boost::variant because its internal object is stored
	// directly (inline) in the variant class and the Scribe library handles this for us.
	// The following link explains why boost::variant storage is stack-based (except for exceptions)
	// http://www.boost.org/doc/libs/1_34_1/doc/html/variant/design.html#variant.design.never-empty
#if 0
	//! Relocated boost::variant.
	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void
	relocated(
			Scribe &scribe,
			const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &relocated_variant_object,
			const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &transcribed_variant_object);
#endif
}


//
// Template implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::intrusive_ptr<T> &intrusive_ptr_object,
			bool transcribed_construct_data)
	{
		T *raw_ptr = NULL;

		if (scribe.is_saving())
		{
			raw_ptr = intrusive_ptr_object.get();
		}

		TranscribeResult transcribe_result =
				transcribe_smart_pointer_protocol(TRANSCRIBE_SOURCE, scribe, raw_ptr, true/*shared_owner*/);
		if (transcribe_result != TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			intrusive_ptr_object.reset(raw_ptr);
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::optional<T> &optional_object,
			bool transcribed_construct_data)
	{
		bool initialised;

		if (scribe.is_saving())
		{
			initialised = optional_object;
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, initialised, "initialised", DONT_TRACK))
		{
			return scribe.get_transcribe_result();
		}

		if (initialised)
		{
			if (scribe.is_saving())
			{
				// Mirror the load path.
				scribe.save(TRANSCRIBE_SOURCE, optional_object.get(), "value");
			}
			else // loading...
			{
				// We don't know if 'T' has default constructor or not.
				// And track the object that's inside the boost::optional in case someone has a reference to it.
				LoadRef<T> value = scribe.load<T>(TRANSCRIBE_SOURCE, "value");
				if (!value.is_valid())
				{
					return scribe.get_transcribe_result();
				}

				// Copy the transcribed value into the boost::optional.
				optional_object = value.get();

				// The transcribed value now has a new address.
				scribe.relocated(TRANSCRIBE_SOURCE, optional_object.get(), value);
			}
		}
		else // not initialised...
		{
			if (scribe.is_loading())
			{
				optional_object = boost::none;
			}
		}

		return TRANSCRIBE_SUCCESS;
	}

	template <typename T>
	void
	relocated(
			Scribe &scribe,
			const boost::optional<T> &relocated_optional_object,
			const boost::optional<T> &transcribed_optional_object)
	{
		// We don't need to relocate boost::optional containing a *non-reference* because its
		// internal object is stored directly (inline) in the boost::optional class and the Scribe
		// library handles this for us.
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::optional<T &> &optional_object_reference,
			bool transcribed_construct_data)
	{
		bool initialised;

		if (scribe.is_saving())
		{
			initialised = optional_object_reference;
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, initialised, "initialised", DONT_TRACK))
		{
			return scribe.get_transcribe_result();
		}

		if (initialised)
		{
			if (scribe.is_saving())
			{
				// Mirror the load path.
				scribe.save_reference(TRANSCRIBE_SOURCE, optional_object_reference.get(), "value");
			}
			else // loading...
			{
				// Set the boost::optional to reference the transcribed reference.
				LoadRef<T> value = scribe.load_reference<T>(TRANSCRIBE_SOURCE, "value");
				if (!value.is_valid())
				{
					return scribe.get_transcribe_result();
				}

				optional_object_reference = value.get();
			}
		}
		else // not initialised...
		{
			if (scribe.is_loading())
			{
				optional_object_reference = boost::none;
			}
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename T>
	void
	relocated(
			Scribe &scribe,
			const boost::optional<T &> &relocated_optional_object,
			const boost::optional<T &> &transcribed_optional_object)
	{
		// Nothing to do - a relocated optional does not mean the referenced object was relocated.
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::scoped_ptr<T> &scoped_ptr_object,
			bool transcribed_construct_data)
	{
		T *raw_ptr = NULL;

		if (scribe.is_saving())
		{
			raw_ptr = scoped_ptr_object.get();
		}

		TranscribeResult transcribe_result =
				transcribe_smart_pointer_protocol(TRANSCRIBE_SOURCE, scribe, raw_ptr, false/*shared_owner*/);
		if (transcribe_result != TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			scoped_ptr_object.reset(raw_ptr);
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::shared_ptr<T> &shared_ptr_object,
			bool transcribed_construct_data)
	{
		T *raw_ptr = NULL;

		if (scribe.is_saving())
		{
			raw_ptr = shared_ptr_object.get();
		}

		TranscribeResult transcribe_result =
				transcribe_smart_pointer_protocol(TRANSCRIBE_SOURCE, scribe, raw_ptr, true/*shared_owner*/);
		if (transcribe_result != TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			// Special helper function of class Scribe to ensure all boost::shared_ptr that
			// reference the same raw pointer will actually share it.
			ScribeInternalAccess::reset(scribe, shared_ptr_object, raw_ptr);
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::weak_ptr<T> &weak_ptr_object,
			bool transcribed_construct_data)
	{
		boost::shared_ptr<T> shared_ptr_object;
		
		if (scribe.is_saving())
		{
			// This could be NULL.
			shared_ptr_object = weak_ptr_object.lock();
		}

		// Delegate directly to the transcribe overload for boost::shared_ptr.
		//
		// Note: We could have instead transcribed the raw pointer in 'weak_ptr_object' but that
		// would have required at least one boost::shared_ptr<T> reference to the same object to
		// have already been transcribed otherwise the *non-owning* raw pointer transcribe would
		// have failed (because raw pointer is not tracked and cannot be updated later).
		TranscribeResult transcribe_result = transcribe(scribe, shared_ptr_object, transcribed_construct_data);
		if (transcribe_result != TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			// Note: If we are the first reference to the pointed-to 'T' object (ie, if not yet
			// transcribed any boost::shared_ptr<T> references) then normally the 'weak_ptr_object'
			// would become NULL as soon as 'shared_ptr_object' goes out of scope.
			// But the Scribe::reset() call made when transcribing 'shared_ptr_object' keeps a copy
			// of it around thus avoiding this problem.
			weak_ptr_object = shared_ptr_object;
		}

		return TRANSCRIBE_SUCCESS;
	}


	namespace Implementation
	{
		class SaveVariantVisitor :
				public boost::static_visitor<>
		{
		public:
			explicit
			SaveVariantVisitor(
					Scribe &scribe) :
				d_scribe(scribe)
			{  }

			template <typename BoundedType>
			void operator()(
					const BoundedType &value) const
			{
				// Mirror the load path.
				d_scribe.save(TRANSCRIBE_SOURCE, value, "stored_value");
			}

		private:
			Scribe &d_scribe;
		};

		template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
		void
		save_variant(
				Scribe &scribe,
				const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &variant_object)
		{
			// Find the export registered class type for the stored object.
			boost::optional<const ExportClassType &> export_class_type =
					ExportRegistry::instance().get_class_type(variant_object.type());

			// Throw exception if the stored object's type has not been export registered.
			//
			// If this assertion is triggered then it means:
			//   * The stored object's type was not export registered in 'ScribeExportRegistration.h'.
			//
			GPlatesGlobal::Assert<Exceptions::UnregisteredClassType>(
					export_class_type,
					GPLATES_ASSERTION_SOURCE,
					variant_object.type());

			// Transcribe the stored type name.
			const std::string &stored_type_name = export_class_type->type_id_name;
			scribe.transcribe(TRANSCRIBE_SOURCE, stored_type_name, "stored_type", DONT_TRACK);

			SaveVariantVisitor visitor(scribe);
			variant_object.apply_visitor(visitor);
		}


		template <class BoundedTypes, class ConstructObjectType>
		TranscribeResult
		load_variant(
				Scribe &scribe,
				ConstructObjectType &variant_object,
				const std::type_info &stored_type_info,
				boost::mpl::false_)
		{
			// End of instantiation chain.
			// We haven't found the requested stored type in any of the variant's stored types
			// so the requested stored type is an unknown type as far as the variant being
			// transcribed is concerned.
			return TRANSCRIBE_UNKNOWN_TYPE;
		}

		template <class BoundedTypes, class ConstructObjectType>
		TranscribeResult
		load_variant(
				Scribe &scribe,
				ConstructObjectType &variant_object,
				const std::type_info &stored_type_info,
				boost::mpl::true_)
		{
			typedef typename boost::mpl::front<BoundedTypes>::type stored_type;

			// If the requested stored type matches the currently visited stored type then
			// we've found a matching type.
			if (typeid(stored_type) == stored_type_info)
			{
				// Load the variant value.
				LoadRef<stored_type> stored_value = scribe.load<stored_type>(TRANSCRIBE_SOURCE, "stored_value");
				if (!stored_value.is_valid())
				{
					return scribe.get_transcribe_result();
				}

				// Store the value in the variant.
				variant_object.construct_object(stored_value.get());

				// The transcribed item now has a new address (inside the variant).
				scribe.relocated(
						TRANSCRIBE_SOURCE,
						boost::get<stored_type>(variant_object.get_object()),
						stored_value);

				return TRANSCRIBE_SUCCESS;
			}

			typedef typename boost::mpl::pop_front<BoundedTypes>::type popped_bounded_types;
			typedef typename boost::mpl::if_<
					boost::mpl::empty<popped_bounded_types>,
					boost::mpl::false_,
					boost::mpl::true_>::type
							not_empty_type;
			return load_variant<popped_bounded_types>(
					scribe,
					variant_object,
					stored_type_info,
					// Used to terminate the instantiation chain...
					not_empty_type());
		}

		template <class BoundedTypes, class ConstructObjectType>
		TranscribeResult
		load_variant(
				Scribe &scribe,
				ConstructObjectType &variant_object)
		{
			// Transcribe the stored type name.
			std::string stored_type_name;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, stored_type_name, "stored_type", DONT_TRACK))
			{
				return scribe.get_transcribe_result();
			}

			// Find the export registered class type associated with the stored object.
			boost::optional<const ExportClassType &> export_class_type =
					ExportRegistry::instance().get_class_type(stored_type_name);

			// If the store type name has not been export registered then it means either:
			//   * the archive was created by a future GPlates with a stored type we don't know about, or
			//   * the archive was created by an old GPlates with a stored type we have since removed.
			//
			if (!export_class_type)
			{
				return TRANSCRIBE_UNKNOWN_TYPE;
			}

			// Get the type info of the stored type.
			const std::type_info &stored_type_info = export_class_type->type_info;

			return load_variant<BoundedTypes>(
					scribe,
					variant_object,
					stored_type_info,
					boost::mpl::true_());
		}

		/**
		 * Class for loading into an *existing* (already constructed) variant.
		 *
		 * It has the same interface as 'ConstructObject' in order that both can use the same code path.
		 */
		template <class VariantType>
		class LoadVariant
		{
		public:

			explicit
			LoadVariant(
					VariantType &variant) :
				d_variant(variant)
			{  }

			VariantType &
			get_object()
			{
				return d_variant;
			}

			template <typename BoundedType>
			void
			construct_object(
					const BoundedType &value)
			{
				// It's actually assignment, not construction.
				d_variant = value;
			}

		private:
			VariantType &d_variant;
		};

		template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
		TranscribeResult
		load_variant(
				Scribe &scribe,
				LoadVariant< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> > &variant_object)
		{
			typedef typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types bounded_types;
			return load_variant<bounded_types>(scribe, variant_object);
		}
	}

	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &variant_object,
			bool transcribed_construct_data)
	{
		// If already transcribed using (non-default) constructor then nothing left to do.
		if (transcribed_construct_data)
		{
			return TRANSCRIBE_SUCCESS;
		}

		if (scribe.is_saving())
		{
			Implementation::save_variant(scribe, variant_object);
			return TRANSCRIBE_SUCCESS;
		}
		else
		{
			Implementation::LoadVariant< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
					load_variant_object(variant_object);
			return Implementation::load_variant(scribe, load_variant_object);
		}
	}

	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> > &variant_object)
	{
		if (scribe.is_saving())
		{
			Implementation::save_variant(scribe, variant_object.get_object());
			return TRANSCRIBE_SUCCESS;
		}
		else // loading...
		{
			typedef typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types bounded_types;
			return Implementation::load_variant<bounded_types>(scribe, variant_object);
		}
	}


	// We don't need to relocate boost::variant because its internal object is stored
	// directly (inline) in the variant class and the Scribe library handles this for us.
	// The following link explains why boost::variant storage is stack-based (except for exceptions)
	// http://www.boost.org/doc/libs/1_34_1/doc/html/variant/design.html#variant.design.never-empty
#if 0
	namespace Implementation
	{
		template <class VariantType>
		class RelocateVariantVisitor :
				public boost::static_visitor<>
		{
		public:
			RelocateVariantVisitor(
					Scribe &scribe,
					const VariantType &relocated_variant) :
				d_scribe(scribe),
				d_relocated_variant(relocated_variant)
			{  }

			template <typename T>
			void operator()(
					const T &transcribed_variant_value) const
			{
				const T &relocated_variant_value = boost::get<T>(d_relocated_variant);
				d_scribe.relocated(TRANSCRIBE_SOURCE, relocated_variant_value, transcribed_variant_value);
			}

		private:
			Scribe &d_scribe;
			const VariantType &d_relocated_variant;
		};
	}

	// We don't need to relocate boost::variant because its internal object is stored
	// directly (inline) in the variant class and the Scribe library handles this for us.
	// The following link explains why boost::variant storage is stack-based (except for exceptions)
	// http://www.boost.org/doc/libs/1_34_1/doc/html/variant/design.html#variant.design.never-empty
	template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
	void
	relocated(
			Scribe &scribe,
			const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &relocated_variant_object,
			const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &transcribed_variant_object)
	{
		// When a boost::variant gets relocated so does its internal object.
		Implementation::RelocateVariantVisitor< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
				visitor(scribe, relocated_variant_object);
		transcribed_variant_object.apply_visitor(visitor);
	}
#endif
}

#endif // GPLATES_SCRIBE_TRANSCRIBEBOOST_H
