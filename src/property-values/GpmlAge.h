/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
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

#ifndef GPLATES_PROPERTYVALUES_GPMLAGE_H
#define GPLATES_PROPERTYVALUES_GPMLAGE_H

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <QString>

#include "TimescaleBand.h"
#include "TimescaleName.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlAge, visit_gpml_age)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:Age".
	 * Unlike the simple doubles we've been using via e.g. gpml:validTime, gpml:Age
	 * hopes to be the One True Age Property Value capable of representing actual
	 * scientific geological data rather than boiling it down to an absolute age
	 * expressed as some floating-point value because FORTRAN.
	 *
	 * Unfortunately, we cannot approach this as some pure, idealised Age representation
	 * that can either be a stratigraphic age xor absolute age (with associated timescale
	 * and uncertainties); Because legacy support is *still* something that certain
	 * people have expressed as necessary, and because pragmatically data may not be in
	 * an ideal form, gpml:Age must support an Age representation that can use both
	 * stratigraphic (or magnetic) timescale names *and* user-assigned absolute ages
	 * simultaneously.
	 *
	 * This means a lot of boost::optional use and some potentially contradictory data.
	 *
	 * Named (Stratigraphic, or Geomagnetic, or who knows what else) ages within a timescale
	 * and names of timescales used are stored using the StringSets TimescaleBand
	 * and TimescaleName respectively.
	 */
	class GpmlAge:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * For convenience methods to use to indicate to callers what format the user has
		 * defined this GpmlAge with.
		 */
		struct AgeDefinition {
			enum AgeDefinitionType {
				AGE_ABSOLUTE, AGE_NAMED, AGE_BOTH, AGE_NONE
			};
		};

		/**
		 * For convenience methods to use to indicate to callers what format the user has
		 * defined this GpmlAge's uncertainty values with.
		 */
		struct UncertaintyDefinition {
			enum UncertaintyDefinitionType {
				UNC_PLUS_OR_MINUS, UNC_RANGE, UNC_NONE
			};
		};
		
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlAge>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlAge> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlAge>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlAge> non_null_ptr_to_const_type;

		virtual
		~GpmlAge()
		{  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		/**
		 * Create a GpmlAge instance.  Note that all of the parameters are boost::optional.
		 * This is because unlike gml:validTime, gpml:Age-type properties are intended for
		 * the actual geological age information that might be attached to a feature, and
		 * as such the amount of information available can vary wildly.
		 */
		static
		const non_null_ptr_type
		create(
				boost::optional<double> age_absolute,
				boost::optional<TimescaleBand> age_named,
				boost::optional<TimescaleName> timescale,
				boost::optional<double> uncertainty_plusminus,
				boost::optional<double> uncertainty_youngest_absolute,
				boost::optional<TimescaleBand> uncertainty_youngest_named,
				boost::optional<double> uncertainty_oldest_absolute,
				boost::optional<TimescaleBand> uncertainty_oldest_named);


		/**
		 * Create a GpmlAge instance.  Note that all of the parameters are boost::optional.
		 * This is because unlike gml:validTime, gpml:Age-type properties are intended for
		 * the actual geological age information that might be attached to a feature, and
		 * as such the amount of information available can vary wildly.
		 *
		 * This version takes plain QStrings instead of the specific StringSet iterator instances,
		 * because that's what you get from the parser and UI.
		 */
		static
		const non_null_ptr_type
		create(
				boost::optional<double> age_absolute,
				boost::optional<QString> age_named,
				boost::optional<QString> timescale,
				boost::optional<double> uncertainty_plusminus,
				boost::optional<double> uncertainty_youngest_absolute,
				boost::optional<QString> uncertainty_youngest_named,
				boost::optional<double> uncertainty_oldest_absolute,
				boost::optional<QString> uncertainty_oldest_named);


		/**
		 * Create a GpmlAge instance.
		 * This version accepts that sometimes you just don't know right now.
		 */
		static
		const non_null_ptr_type
		create();
		

		const GpmlAge::non_null_ptr_type
		clone() const
		{
			GpmlAge::non_null_ptr_type dup(new GpmlAge(*this));
			return dup;
		}

		const GpmlAge::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Return the absolute age, if such data is explicitly present, of this GpmlAge.
		 */
		const boost::optional<double> &
		get_age_absolute() const;

		/**
		 * Set the absolute age of this GpmlAge.
		 * This does not unset any named age if present; it is possible for a GpmlAge
		 * to contain both absolute and named ages simultaneously.
		 * It is possible to unset the absolute age by passing boost::none.
		 */
		void
		set_age_absolute(
				boost::optional<double> age_maybe);

		
		/**
		 * Return the named (stratigraphic,geomagnetic) age, if such data is explicitly present, of this GpmlAge.
		 */
		const boost::optional<TimescaleBand> &
		get_age_named() const;

		/**
		 * Set the named (stratigraphic,geomagnetic) age of this GpmlAge.
		 * This does not unset any absolute age if present; it is possible for a GpmlAge
		 * to contain both absolute and named ages simultaneously.
		 * It is possible to unset the named age by passing boost::none.
		 */
		void
		set_age_named(
				boost::optional<TimescaleBand> age_maybe);

		/**
		 * As set_age_named(TimescaleBand), but sometimes all you have is a QString...
		 */
		void
		set_age_named(
				const QString &age);
		
		
		/**
		 * Convenience method to quickly determine how this Age has been defined.
		 */
		AgeDefinition::AgeDefinitionType
		age_type() const;

		
		/**
		 * Return the name of the geological or geomagnetic (or who knows what else) timescale
		 * used by this GpmlAge.
		 */
		const boost::optional<TimescaleName> &
		get_timescale() const;

		/**
		 * Set the name of the timescale used by this GpmlAge.
		 * This does not automagically do any conversion of the absolute ages that may be in use
		 * by the GpmlAge, nor does it validate the names used by any named ages. It just records
		 * what timescale the age data is *supposed* to be in.
		 */
		void
		set_timescale(
				boost::optional<TimescaleName> timescale_maybe);

		/**
		 * As set_timescale(TimescaleName), but sometimes all you have is a QString...
		 */
		void
		set_timescale(
				const QString &timescale);
		
		
		/**
		 * A GpmlAge can express uncertainties in one of two ways; a simple plus-or-minus value
		 * expressed in My or an asymmetric young <=> old range.
		 * Presuming it has been set, this method returns the plus-or-minus value.
		 */
		const boost::optional<double> &
		get_uncertainty_plusminus() const;

		/**
		 * Set the uncertainty of this GpmlAge to a simple plus-or-minus value expressed in My.
		 *
		 * Although it makes absolutely no sense to have uncertainties expressed two different ways
		 * simultaneously (until someone tells me otherwise), setting the uncertainty via this
		 * method does not clear the 'range' uncertainty values. This is because generally speaking
		 * you are either populating a fresh GpmlAge from a file, or setting an existing GpmlAge's
		 * fields from the UI, and in such case you are almost certainly also setting every single
		 * field to a boost::none explicitly as appropriate based on what widgets contain what.
		 * To prevent any subtle bugs, no spooky action-at-a-distance happens.
		 */
		void
		set_uncertainty_plusminus(
				boost::optional<double> uncertainty_maybe);


		/**
		 * A GpmlAge can express uncertainties in one of two ways; a simple plus-or-minus value
		 * expressed in My or an asymmetric young <=> old range.
		 * Of course, the values of that range can also be either a name or an absolute age,
		 * just to complicate things.
		 * Presuming it has been set that way, this method returns the youngest part of the
		 * uncertainty as an absolute age.
		 */
		const boost::optional<double> &
		get_uncertainty_youngest_absolute() const;

		/**
		 * Set the youngest part of the uncertainty range of this GpmlAge to an absolute value in Ma.
		 *
		 * Although it makes absolutely no sense to have uncertainties expressed two different ways
		 * simultaneously (until someone tells me otherwise), setting the uncertainty via this
		 * method does not clear the 'range' uncertainty values. This is because generally speaking
		 * you are either populating a fresh GpmlAge from a file, or setting an existing GpmlAge's
		 * fields from the UI, and in such case you are almost certainly also setting every single
		 * field to a boost::none explicitly as appropriate based on what widgets contain what.
		 * To prevent any subtle bugs, no spooky action-at-a-distance happens.
		 */
		void
		set_uncertainty_youngest_absolute(
				boost::optional<double> uncertainty_maybe);


		/**
		 * A GpmlAge can express uncertainties in one of two ways; a simple plus-or-minus value
		 * expressed in My or an asymmetric young <=> old range.
		 * Of course, the values of that range can also be either a name or an absolute age,
		 * just to complicate things.
		 * Presuming it has been set that way, this method returns the youngest part of the
		 * uncertainty as a named age.
		 */
		const boost::optional<TimescaleBand> &
		get_uncertainty_youngest_named() const;

		/**
		 * Set the youngest part of the uncertainty range of this GpmlAge to a named value from some timescale.
		 *
		 * Although it makes absolutely no sense to have uncertainties expressed two different ways
		 * simultaneously (until someone tells me otherwise), setting the uncertainty via this
		 * method does not clear the 'range' uncertainty values. This is because generally speaking
		 * you are either populating a fresh GpmlAge from a file, or setting an existing GpmlAge's
		 * fields from the UI, and in such case you are almost certainly also setting every single
		 * field to a boost::none explicitly as appropriate based on what widgets contain what.
		 * To prevent any subtle bugs, no spooky action-at-a-distance happens.
		 */
		void
		set_uncertainty_youngest_named(
				boost::optional<TimescaleBand> uncertainty_maybe);
		
		/**
		 * As set_uncertainty_youngest_named(TimescaleBand), but sometimes all you have is a QString...
		 */
		void
		set_uncertainty_youngest_named(
				const QString &uncertainty);


		/**
		 * A GpmlAge can express uncertainties in one of two ways; a simple plus-or-minus value
		 * expressed in My or an asymmetric young <=> old range.
		 * Of course, the values of that range can also be either a name or an absolute age,
		 * just to complicate things.
		 * Presuming it has been set that way, this method returns the oldest part of the
		 * uncertainty as an absolute age.
		 */
		const boost::optional<double> &
		get_uncertainty_oldest_absolute() const;

		/**
		 * Set the oldest part of the uncertainty range of this GpmlAge to an absolute value in Ma.
		 *
		 * Although it makes absolutely no sense to have uncertainties expressed two different ways
		 * simultaneously (until someone tells me otherwise), setting the uncertainty via this
		 * method does not clear the 'range' uncertainty values. This is because generally speaking
		 * you are either populating a fresh GpmlAge from a file, or setting an existing GpmlAge's
		 * fields from the UI, and in such case you are almost certainly also setting every single
		 * field to a boost::none explicitly as appropriate based on what widgets contain what.
		 * To prevent any subtle bugs, no spooky action-at-a-distance happens.
		 */
		void
		set_uncertainty_oldest_absolute(
				boost::optional<double> uncertainty_maybe);


		/**
		 * A GpmlAge can express uncertainties in one of two ways; a simple plus-or-minus value
		 * expressed in My or an asymmetric young <=> old range.
		 * Of course, the values of that range can also be either a name or an absolute age,
		 * just to complicate things.
		 * Presuming it has been set that way, this method returns the oldest part of the
		 * uncertainty as a named age.
		 */
		const boost::optional<TimescaleBand> &
		get_uncertainty_oldest_named() const;

		/**
		 * Set the oldest part of the uncertainty range of this GpmlAge to a named value from some timescale.
		 *
		 * Although it makes absolutely no sense to have uncertainties expressed two different ways
		 * simultaneously (until someone tells me otherwise), setting the uncertainty via this
		 * method does not clear the 'range' uncertainty values. This is because generally speaking
		 * you are either populating a fresh GpmlAge from a file, or setting an existing GpmlAge's
		 * fields from the UI, and in such case you are almost certainly also setting every single
		 * field to a boost::none explicitly as appropriate based on what widgets contain what.
		 * To prevent any subtle bugs, no spooky action-at-a-distance happens.
		 */
		void
		set_uncertainty_oldest_named(
				boost::optional<TimescaleBand> uncertainty_maybe);

		/**
		 * As set_uncertainty_oldest_named(TimescaleBand), but sometimes all you have is a QString...
		 */
		void
		set_uncertainty_oldest_named(
				const QString &uncertainty);


		/**
		 * Convenience method to quickly determine how this Age's uncertainty data has been defined.
		 */
		UncertaintyDefinition::UncertaintyDefinitionType
		uncertainty_type() const;

		
		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("Age");
			return STRUCTURAL_TYPE;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_age(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gpml_age(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlAge(
					boost::optional<double> age_absolute,
					boost::optional<TimescaleBand> age_named,
					boost::optional<TimescaleName> timescale,
					boost::optional<double> uncertainty_plusminus,
					boost::optional<double> uncertainty_youngest_absolute,
					boost::optional<TimescaleBand> uncertainty_youngest_named,
					boost::optional<double> uncertainty_oldest_absolute,
					boost::optional<TimescaleBand> uncertainty_oldest_named):
			PropertyValue(),
			d_age_absolute(age_absolute),
			d_age_named(age_named),
			d_timescale(timescale),
			d_uncertainty_plusminus(uncertainty_plusminus),
			d_uncertainty_youngest_absolute(uncertainty_youngest_absolute),
			d_uncertainty_youngest_named(uncertainty_youngest_named),
			d_uncertainty_oldest_absolute(uncertainty_oldest_absolute),
			d_uncertainty_oldest_named(uncertainty_oldest_named)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlAge(
				const GpmlAge &other) :
			PropertyValue(other), /* share instance id */
			d_age_absolute(other.d_age_absolute),
			d_age_named(other.d_age_named),
			d_timescale(other.d_timescale),
			d_uncertainty_plusminus(other.d_uncertainty_plusminus),
			d_uncertainty_youngest_absolute(other.d_uncertainty_youngest_absolute),
			d_uncertainty_youngest_named(other.d_uncertainty_youngest_named),
			d_uncertainty_oldest_absolute(other.d_uncertainty_oldest_absolute),
			d_uncertainty_oldest_named(other.d_uncertainty_oldest_named)
		{  }

	private:

		/**
		 * A gpml:Age can have its age specified as an absolute (numeric) age in Ma.
		 */
		boost::optional<double> d_age_absolute;

		/**
		 * A gpml:Age can also have its age specified as a named (stratigraphic or
		 * otherwise) age, such as "Paleogene" or "Late Triassic".
		 *
		 * Both @a d_age_absolute and @a d_age_named can be present in the data,
		 * and (sadly) there is potential for conflicting information there.
		 * While a named stratigraphic age may represent data we are more certain about
		 * (i.e. "this fossil was found x metres down in the Permian layer"), we cannot
		 * discount the fact that the user manually assigning an absolute age is a very
		 * explicit action and they clearly want to use that absolute age. But we don't
		 * want to just throw out the stratigraphic data either, because that can be
		 * important metadata. Fearless Leader has also expressed a concern that we must have
		 * numeric ages available for legacy programs to use.
		 * In conclusion, no, there is no easy way to say what should be used in the event
		 * that GPlates gains awareness of timescale bands' age ranges, unless exactly
		 * one of @a d_age_absolute or @a d_age_named are present.
		 */
		boost::optional<TimescaleBand> d_age_named;

		/**
		 * A gpml:Age can (and is strongly encouraged to) have a stratigraphic or geomagnetic
		 * timescale associated with it. This member stores the "well known" name of the timescale,
		 * such as ICC2012 or GTS2004.
		 */
		boost::optional<TimescaleName> d_timescale;

		/**
		 * A gpml:Age can have an associated uncertainty. It can be expressed as a plus-or-minus
		 * value measured in My.
		 */
		boost::optional<double> d_uncertainty_plusminus;

		/**
		 * A gpml:Age can alternatively represent uncertainty information as an asymmetric age
		 * range, with a 'youngest' and 'oldest' age estimate. Just as with the principal age,
		 * these can be either absolute ages or named ages.
		 *
		 * I'm putting my foot down and saying that this representation of uncertainty will only
		 * have an (absolute xor named) age for each end of the range; Mostly this is just to retain
		 * some degree of sanity for the EditAgeWidget UI. --jclark 20150303
		 */
		boost::optional<double> d_uncertainty_youngest_absolute;
		boost::optional<TimescaleBand> d_uncertainty_youngest_named;
		boost::optional<double> d_uncertainty_oldest_absolute;
		boost::optional<TimescaleBand> d_uncertainty_oldest_named;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlAge &
		operator=(const GpmlAge &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLAGE_H
