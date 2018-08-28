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

#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "PyReconstructionGeometries.h"

#include "PyInformationModel.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionGeometryVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * Convert a GeometryOnSphere to a PolylineOnSphere (even if they are just single points - just duplicate).
		 *
		 * This will make it easier for the user who will expect sub-segments to be polylines, or
		 * RFG topological sections to be polylines (instead, eg, polygons).
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		convert_geometry_to_polyline(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &sub_segment_geometry)
		{
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> sub_segment_polyline =
					GPlatesAppLogic::GeometryUtils::convert_geometry_to_polyline(*sub_segment_geometry);
			if (!sub_segment_polyline)
			{
				// There were less than two points.
				// 
				// Retrieve the point.
				std::vector<GPlatesMaths::PointOnSphere> geometry_points;
				GPlatesAppLogic::GeometryUtils::get_geometry_points(*sub_segment_geometry, geometry_points);

				// There should be a single point.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						!geometry_points.empty(),
						GPLATES_ASSERTION_SOURCE);

				// Duplicate the point.
				geometry_points.push_back(geometry_points.back());

				sub_segment_polyline = GPlatesMaths::PolylineOnSphere::create_on_heap(geometry_points);
			}

			return sub_segment_polyline.get();
		}

		/**
		 * Get the polyline from the reconstruction geometry of a topological section feature.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*,
		 * otherwise returns none.
		 */
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		get_topological_section_geometry(
				const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &topological_section)
		{
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> topological_section_geometry =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_section_geometry(
							topological_section);
			if (!topological_section_geometry)
			{
				return boost::none;
			}

			return convert_geometry_to_polyline(topological_section_geometry.get());
		}

		/**
		 * Create a resolved feature by cloning the feature and setting the resolved geometry on it.
		 */
		bp::object
		create_resolved_feature(
				GPlatesModel::FeatureHandle::non_null_ptr_type feature,
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type resolved_reconstruction_geometry,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_geometry)
		{
			// Get the property name that the reconstruction geometry came from.
			// We need this to set the geometry on the correct geometry property name in the cloned feature.
			boost::optional<GPlatesModel::FeatureHandle::iterator> resolved_geometry_property_iterator =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
							resolved_reconstruction_geometry);
			if (!resolved_geometry_property_iterator)
			{
				return bp::object()/*Py_None*/;
			}
			const GPlatesModel::PropertyName &resolved_geometry_property_name =
					(*resolved_geometry_property_iterator.get())->get_property_name();

			// Clone the feature.
			// Currently we use Python to do this.
			// FIXME: When C++ FeatureHandle::clone() is implemented to be deep then switch to using it.
			bp::object resolved_feature_object = bp::object(feature).attr("clone")();

			// Get the C++ cloned feature.
			GPlatesModel::FeatureHandle::non_null_ptr_type resolved_feature =
					bp::extract<GPlatesModel::FeatureHandle::non_null_ptr_type>(resolved_feature_object);

			// Remove all geometry properties.
			// We need to do this because our resolved feature should really only contain the
			// single resolved geometry - any other geometries (with different property names) will
			// just confuse the user and look like spurious resolved geometries in the wrong position.
			std::vector<GPlatesModel::FeatureHandle::iterator> cloned_geometry_properties =
					GPlatesModel::ModelUtils::get_top_level_geometry_properties(resolved_feature->reference());
			BOOST_FOREACH(GPlatesModel::FeatureHandle::iterator cloned_geometry_property, cloned_geometry_properties)
			{
				resolved_feature->remove(cloned_geometry_property);
			}

			// Set the resolved geometry.
			//
			// NOTE: We don't verify the information model when setting the geometry.
			// When creating a resolved feature for a boundary sub-segment, or a shared sub-segment,
			// we convert to polyline. However if the segment was a point it's possible the feature type
			// (of the segment) only allows point geometries, such as 'gpml:position' properties, in which
			// case setting a polyline will raise an InformationModelError exception. Since we're not
			// giving the Python user a chance to disable this we will go ahead and disable internally here.
			// Note that when creating a resolved feature for a topological line, boundary or network
			// we don't really need this since there is no conversion to polyline (as is the case
			// with sub-segments) and so the original geometry type matches; but we'll go ahead
			// and disable for all cases anyway.
			resolved_feature_object.attr("set_geometry")(
					resolved_geometry,
					resolved_geometry_property_name,
					bp::object()/*Py_None*/,  // reverse_reconstruct
					VerifyInformationModel::NO);  // verify_information_model

			return resolved_feature_object;
		}
	}


	/**
	 * Returns the referenced feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	reconstruction_geometry_get_feature(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry)
	{
		// The feature reference could be invalid. It should normally be valid though.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(&reconstruction_geometry);
		if (!feature_ref ||
			!feature_ref->is_valid())
		{
			return boost::none;
		}

		return GPlatesModel::FeatureHandle::non_null_ptr_type(feature_ref->handle_ptr());
	}

	/**
	 * Returns the referenced feature property.
	 *
	 * The feature property reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
	reconstruction_geometry_get_property(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry)
	{
		// The property iterator could be invalid. It should normally be valid though.
		boost::optional<GPlatesModel::FeatureHandle::iterator> property_iter =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
						&reconstruction_geometry);
		if (!property_iter ||
			!property_iter->is_still_valid())
		{
			return boost::none;
		}

		return *property_iter.get();
	}

	/**
	 * ReconstructionGeometry visitor to create a derived reconstruction geometry type wrapper.
	 */
	class WrapReconstructionGeometryTypeVisitor :
			public GPlatesAppLogic::ReconstructionGeometryVisitor
	{
	public:
		// Bring base class visit methods into scope of current class.
		using GPlatesAppLogic::ReconstructionGeometryVisitor::visit;


		const boost::any &
		get_reconstruction_geometry_type_wrapper() const
		{
			return d_reconstruction_geometry_type_wrapper;
		}


		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<reconstructed_feature_geometry_type>(rfg));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<reconstructed_motion_path_type>(rmp));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<reconstructed_flowline_type>(rf));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_line_type> &rtl)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<resolved_topological_line_type>(rtl));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<resolved_topological_boundary_type>(rtb));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<resolved_topological_network_type>(rtn));
		}

	private:
		// We just need to store the wrapper - we don't need to access it.
		boost::any d_reconstruction_geometry_type_wrapper;
	};


	KeepReconstructionGeometryFeatureAndPropertyAlive::KeepReconstructionGeometryFeatureAndPropertyAlive(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) :
		d_feature(reconstruction_geometry_get_feature(reconstruction_geometry)),
		d_property(reconstruction_geometry_get_property(reconstruction_geometry))
	{  }


	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry) :
		d_reconstruction_geometry(
				// We wrap *non-const* reconstruction geometries and hence cast away const
				// (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ReconstructionGeometry>(reconstruction_geometry)),
		d_reconstruction_geometry_type_wrapper(
				create_reconstruction_geometry_type_wrapper(d_reconstruction_geometry))
	{  }


	boost::any
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>::create_reconstruction_geometry_type_wrapper(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry)
	{
		WrapReconstructionGeometryTypeVisitor visitor;
		reconstruction_geometry->accept_visitor(visitor);
		return visitor.get_reconstruction_geometry_type_wrapper();
	}


	/**
	 * To-python converter from a 'non_null_intrusive_ptr<ReconstructionGeometryType>'
	 * to a 'ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>'.
	 */
	template <class ReconstructionGeometryType>
	struct ToPythonConversionReconstructionGeometryWrapperType :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const typename ReconstructionGeometryType::non_null_ptr_type &rg)
			{
				// Convert to ReconstructionGeometryTypeWrapper<> first.
				// Then it'll get converted to python as part of bp::class_ wrapper.
				return bp::incref(bp::object(
						ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>(rg)).ptr());
			}
		};
	};


	/**
	 * Registers a to-python converter from a 'non_null_intrusive_ptr<ReconstructionGeometryType>'
	 * to a 'ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>'.
	 */
	template <class ReconstructionGeometryType>
	void
	register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper()
	{
		// To python conversion.
		bp::to_python_converter<
				typename ReconstructionGeometryType::non_null_ptr_type,
				typename ToPythonConversionReconstructionGeometryWrapperType<ReconstructionGeometryType>::Conversion>();
	}


	/**
	 * From-python converter from a 'ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>'
	 * to a 'ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>'.
	 */
	template <class ReconstructionGeometryType>
	struct FromPythonConversionBaseToDerivedReconstructionGeometryWrapperType :
			private boost::noncopyable
	{
		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> is created from a
			// ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>.
			bp::extract< ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> > extract_reconstruction_geometry_wrapper(obj);
			if (!extract_reconstruction_geometry_wrapper.check())
			{
				return NULL;
			}

			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &reconstruction_geometry_wrapper =
					extract_reconstruction_geometry_wrapper();

			// See if can convert to the derived reconstruction geometry type wrapper.
			boost::optional<const ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> &> wrapper =
					reconstruction_geometry_wrapper.get_reconstruction_geometry_type_wrapper<ReconstructionGeometryType>();

			return wrapper ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> > *>(
									data)->storage.bytes;

			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &reconstruction_geometry_wrapper =
					bp::extract< ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> >(obj)();

			// Extract theReconstructionGeometryTypeWrapper<ReconstructionGeometryType> wrapper.
			boost::optional<const ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> &> wrapper =
					reconstruction_geometry_wrapper.get_reconstruction_geometry_type_wrapper<ReconstructionGeometryType>();

			// The function 'convertible()' will have verified this.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					wrapper,
					GPLATES_ASSERTION_SOURCE);

			new (storage) ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>(wrapper.get());

			data->convertible = storage;
		}
	};


	/**
	 * Registers a converter from a 'ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>'
	 * to a 'ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>'.
	 *
	 * This is useful when, for example, a GPlatesAppLogic::ResolvedTopologicalLine is passed to Python
	 * as a GPlatesAppLogic::ReconstructionGeometry. Coming from Python, boost-python knows the
	 * GPlatesAppLogic::ReconstructionGeometry is a GPlatesAppLogic::ResolvedTopologicalLine but
	 * cannot convert from a...
	 * 
	 *   GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	 * 
	 * ...to a...
	 * 
	 *   GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>
	 * 
	 * ...so we need to provide that conversion.
	 * Note that this only applies when C++ needs an actual
	 * GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>.
	 * Usually it just needs a GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type in which
	 * case this conversion is not used.
	 */
	template <class ReconstructionGeometryType>
	void
	register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper()
	{
		namespace bp = boost::python;

		// From python conversion.
		bp::converter::registry::push_back(
				&FromPythonConversionBaseToDerivedReconstructionGeometryWrapperType<ReconstructionGeometryType>::convertible,
				&FromPythonConversionBaseToDerivedReconstructionGeometryWrapperType<ReconstructionGeometryType>::construct,
				bp::type_id< ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> >());
	}
}


void
export_reconstruction_geometry()
{
	//
	// ReconstructionGeometry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructionGeometry,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructionGeometry",
					"The base class inherited by all derived reconstruction geometry classes.\n"
					"\n"
					"The list of derived classes is:\n"
					"\n"
					"* :class:`ReconstructedFeatureGeometry`\n"
					"* :class:`ReconstructedMotionPath`\n"
					"* :class:`ReconstructedFlowline`\n"
					"* :class:`ResolvedTopologicalLine`\n"
					"* :class:`ResolvedTopologicalBoundary`\n"
					"* :class:`ResolvedTopologicalNetwork`\n",
					bp::no_init)
		.def("get_reconstruction_time",
				&GPlatesAppLogic::ReconstructionGeometry::get_reconstruction_time,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_reconstruction_time()\n"
				"  Returns the reconstruction time that this instance was created at.\n"
				"\n"
				"  :rtype: float\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ReconstructionGeometry>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ReconstructionGeometry>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ReconstructionGeometry>
	// to a GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ReconstructionGeometry pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ReconstructionGeometry pointer to a ReconstructionGeometry::non_null_ptr_type.

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructionGeometry
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the present day geometry.
	 *
	 * boost::none could be returned but it normally shouldn't so we don't document that Py_None
	 * could be returned to the caller.
	 */
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	reconstructed_feature_geometry_get_present_day_geometry(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry)
	{
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> property =
				reconstruction_geometry_get_property(reconstructed_feature_geometry);
		if (!property)
		{
			return boost::none;
		}

		// Call python since Property.get_value is implemented in python code...
		bp::object property_value_object = bp::object(property.get()).attr("get_value")();
		if (property_value_object == bp::object()/*Py_None*/)
		{
			return boost::none;
		}

		// Get the property value.
		GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type>(property_value_object);

		// Extract the geometry from the property value.
		return GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(*property_value);
	}

	/**
	 * Returns the reconstructed geometry.
	 */
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
	reconstructed_feature_geometry_get_reconstructed_geometry(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry)
	{
		return reconstructed_feature_geometry.reconstructed_geometry();
	}
}


void
export_reconstructed_feature_geometry()
{
	//
	// ReconstructedFeatureGeometry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedFeatureGeometry,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedFeatureGeometry>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructedFeatureGeometry",
					"The geometry of a feature reconstructed to a geological time.\n"
					"\n"
					"The :func:`reconstruct` function can be used to generate *ReconstructedFeatureGeometry* instances.\n"
					"\n"
					".. note:: | A single feature can have multiple geometry properties, and hence multiple "
					"reconstructed feature geometries, associated with it.\n"
					"          | Therefore each :class:`ReconstructedFeatureGeometry` references a different property of "
					"the feature via :meth:`get_property`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: Multiple :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` can "
				"be associated with the same :class:`feature<Feature>` if that feature has multiple geometry properties.\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the present day (unreconstructed) geometry "
				"associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day geometry<get_present_day_geometry>` "
				"and the :meth:`reconstructed geometry<get_reconstructed_geometry>` are obtained from.\n")
		.def("get_present_day_geometry",
				&GPlatesApi::reconstructed_feature_geometry_get_present_day_geometry,
				"get_present_day_geometry()\n"
				"  Returns the present day geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		.def("get_reconstructed_geometry",
				&GPlatesApi::reconstructed_feature_geometry_get_reconstructed_geometry,
				"get_reconstructed_geometry()\n"
				"  Returns the reconstructed geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ReconstructedFeatureGeometry>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ReconstructedFeatureGeometry>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ReconstructedFeatureGeometry>
	// to a GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ReconstructedFeatureGeometry pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ReconstructedFeatureGeometry pointer to a ReconstructionGeometry::non_null_ptr_type.

	// Enable a python-wrapped ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	// to be from-python converted to a ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedFeatureGeometry>.
	GPlatesApi::register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper<
			GPlatesAppLogic::ReconstructedFeatureGeometry>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedFeatureGeometry
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ReconstructedFeatureGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the motion path points.
	 */
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	reconstructed_motion_path_get_motion_path(
			const GPlatesAppLogic::ReconstructedMotionPath &reconstructed_motion_path)
	{
		return reconstructed_motion_path.motion_path_points();
	}

	/**
	 * Returns the reconstructed seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_motion_path_get_reconstructed_seed_point(
			const GPlatesAppLogic::ReconstructedMotionPath &reconstructed_motion_path)
	{
		return reconstructed_motion_path.reconstructed_seed_point();
	}

	/**
	 * Returns the present day seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_motion_path_get_present_day_seed_point(
			const GPlatesAppLogic::ReconstructedMotionPath &reconstructed_motion_path)
	{
		return reconstructed_motion_path.present_day_seed_point();
	}
}


void
export_reconstructed_motion_path()
{
	//
	// ReconstructedMotionPath - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedMotionPath,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedMotionPath>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructedMotionPath",
					"The reconstructed history of a plate's motion in the form of a path of points "
					"over geological time.\n"
					"\n"
					"The :func:`reconstruct` function can be used to generate *ReconstructedMotionPath* instances.\n"
					"\n"
					".. note:: | Although a single motion path :class:`feature<Feature>` has only a single "
					"seed geometry that seed geometry can be either a :class:`PointOnSphere` or a "
					":class:`MultiPointOnSphere`.\n"
					"          | And since there is one :class:`reconstructed motion path<ReconstructedMotionPath>` "
					"per seed point there can be, in the case of a :class:`MultiPointOnSphere`, multiple "
					":class:`reconstructed motion paths<ReconstructedMotionPath>` per motion path "
					":class:`feature<Feature>`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ReconstructedMotionPath`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: Multiple :class:`reconstructed motion paths<ReconstructedMotionPath>` "
				"can be associated with the same motion path :class:`feature<Feature>` if its seed geometry "
				"is a :class:`MultiPointOnSphere`.\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the seed point associated with this "
				":class:`ReconstructedMotionPath`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day seed point<get_present_day_seed_point>` "
				"and the :meth:`reconstructed seed point<get_reconstructed_seed_point>` are obtained from.\n")
		.def("get_present_day_seed_point",
				&GPlatesApi::reconstructed_motion_path_get_present_day_seed_point,
				"get_present_day_seed_point()\n"
				"  Returns the present day seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedMotionPath` instances.\n")
		.def("get_reconstructed_seed_point",
				&GPlatesApi::reconstructed_motion_path_get_reconstructed_seed_point,
				"get_reconstructed_seed_point()\n"
				"  Returns the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedMotionPath` instances.\n")
		.def("get_motion_path",
				&GPlatesApi::reconstructed_motion_path_get_motion_path,
				"get_motion_path()\n"
				"  Returns the motion path.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  The returned points plot the history of motion of the "
				":meth:`seed point<get_present_day_seed_point>` on the plate associated with "
				"``get_feature().get_reconstruction_plate_id()`` relative to the plate associated "
				"with ``get_feature().get_relative_plate()``.\n"
				"\n"
				"  The first point in the returned :class:`PolylineOnSphere` is the furthest in the "
				"geological past and subsequent points are progressively more recent with the last "
				"point being the :meth:`reconstructed seed point<get_reconstructed_seed_point>`.\n"
				"\n"
				"  .. note:: This is just one of the motion paths associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the motion path points:\n"
				"  ::\n"
				"\n"
				"    for point in reconstructed_motion_path.get_motion_path():\n"
				"      ...\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ReconstructedMotionPath>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ReconstructedMotionPath>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ReconstructedMotionPath>
	// to a GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ReconstructedMotionPath pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ReconstructedMotionPath pointer to a ReconstructionGeometry::non_null_ptr_type.

	// Enable a python-wrapped ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	// to be from-python converted to a ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedMotionPath>.
	GPlatesApi::register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper<
			GPlatesAppLogic::ReconstructedMotionPath>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedMotionPath
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ReconstructedMotionPath>();
}


namespace GPlatesApi
{
	/**
	 * Returns the left flowline points.
	 */
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_left_flowline(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.left_flowline_points();
	}

	/**
	 * Returns the right flowline points.
	 */
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_right_flowline(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.right_flowline_points();
	}

	/**
	 * Returns the reconstructed seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_reconstructed_seed_point(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.reconstructed_seed_point();
	}

	/**
	 * Returns the present day seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_present_day_seed_point(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.present_day_seed_point();
	}
}


void
export_reconstructed_flowline()
{
	//
	// ReconstructedFlowline - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedFlowline,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedFlowline>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructedFlowline",
					"The reconstructed history of plate motion away from a spreading ridge in the form of "
					"a path of points over geological time.\n"
					"\n"
					"The :func:`reconstruct` function can be used to generate *ReconstructedFlowline* instances.\n"
					"\n"
					".. note:: | Although a single flowline :class:`feature<Feature>` has only a single "
					"seed geometry that seed geometry can be either a :class:`PointOnSphere` or a "
					":class:`MultiPointOnSphere`.\n"
					"          | And since there is one :class:`reconstructed flowline<ReconstructedFlowline>` "
					"per seed point there can be, in the case of a :class:`MultiPointOnSphere`, multiple "
					":class:`reconstructed flowlines<ReconstructedFlowline>` per flowline "
					":class:`feature<Feature>`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ReconstructedFlowline`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: Multiple :class:`reconstructed flowlines<ReconstructedFlowline>` "
				"can be associated with the same flowline :class:`feature<Feature>` if its seed geometry "
				"is a :class:`MultiPointOnSphere`.\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the seed point associated with this "
				":class:`ReconstructedFlowline`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day seed point<get_present_day_seed_point>` "
				"and the :meth:`reconstructed seed point<get_reconstructed_seed_point>` are obtained from.\n")
		.def("get_present_day_seed_point",
				&GPlatesApi::reconstructed_flowline_get_present_day_seed_point,
				"get_present_day_seed_point()\n"
				"  Returns the present day seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedFlowline` instances.\n")
		.def("get_reconstructed_seed_point",
				&GPlatesApi::reconstructed_flowline_get_reconstructed_seed_point,
				"get_reconstructed_seed_point()\n"
				"  Returns the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedFlowline` instances.\n")
		.def("get_left_flowline",
				&GPlatesApi::reconstructed_flowline_get_left_flowline,
				"get_left_flowline()\n"
				"  Returns the flowline spread along the *left* plate from the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  The returned points are on the *left* plate associated with "
				"``get_feature().get_left_plate()``\n"
				"\n"
				"  The first point in the returned :class:`PolylineOnSphere` is the "
				":meth:`reconstructed seed point<get_reconstructed_seed_point>` and subsequent points "
				"are progressively further in the geological past.\n"
				"\n"
				"  .. note:: This is just one of the *left* flowlines associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the left flowline points:\n"
				"  ::\n"
				"\n"
				"    for left_point in reconstructed_flowline.get_left_flowline():\n"
				"      ...\n")
		.def("get_right_flowline",
				&GPlatesApi::reconstructed_flowline_get_right_flowline,
				"get_right_flowline()\n"
				"  Returns the flowline spread along the *right* plate from the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  The returned points are on the *right* plate associated with "
				"``get_feature().get_right_plate()``\n"
				"\n"
				"  The first point in the returned :class:`PolylineOnSphere` is the "
				":meth:`reconstructed seed point<get_reconstructed_seed_point>` and subsequent points "
				"are progressively further in the geological past.\n"
				"\n"
				"  .. note:: This is just one of the *right* flowlines associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the right flowline points:\n"
				"  ::\n"
				"\n"
				"    for right_point in reconstructed_flowline.get_right_flowline():\n"
				"      ...\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ReconstructedFlowline>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ReconstructedFlowline>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ReconstructedFlowline>
	// to a GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ReconstructedFlowline pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ReconstructedFlowline pointer to a ReconstructionGeometry::non_null_ptr_type.

	// Enable a python-wrapped ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	// to be from-python converted to a ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedFlowline>.
	GPlatesApi::register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper<
			GPlatesAppLogic::ReconstructedFlowline>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedFlowline
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ReconstructedFlowline>();
}


namespace GPlatesApi
{
	/**
	 * Returns the resolved line geometry.
	 */
	GPlatesAppLogic::ResolvedTopologicalLine::resolved_topology_line_ptr_type
	resolved_topological_line_get_resolved_line(
			const GPlatesAppLogic::ResolvedTopologicalLine &resolved_topological_line)
	{
		return resolved_topological_line.resolved_topology_line();
	}

	bp::list
	resolved_topological_line_get_line_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalLine &resolved_topological_line)
	{
		bp::list line_sub_segments_list;

		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_line.get_sub_segment_sequence();
		BOOST_FOREACH(const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment, sub_segments)
		{
			line_sub_segments_list.append(sub_segment);
		}

		return line_sub_segments_list;
	}

	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_to_const_type resolved_topological_line) :
		d_resolved_topological_line(
				// We wrap *non-const* reconstruction geometries and hence cast away const
				// (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalLine>(resolved_topological_line)),
		d_keep_feature_property_alive(*resolved_topological_line)
	{
		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_line->get_sub_segment_sequence();

		d_sub_segments.reserve(sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment,
				sub_segments)
		{
			d_sub_segments.push_back(
					ResolvedTopologicalGeometrySubSegmentWrapper(sub_segment));
		}
	}

	bp::object
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature =
					reconstruction_geometry_get_feature(*d_resolved_topological_line);
			if (!feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_geometry =
					d_resolved_topological_line->resolved_topology_line();

			// Create the resolved feature.
			bp::object resolved_feature_object =
					create_resolved_feature(feature.get(), d_resolved_topological_line, resolved_geometry);
			if (resolved_feature_object == bp::object()/*Py_None*/)
			{
				return bp::object()/*Py_None*/;
			}

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}

	/**
	 * For some reason we need to wrap...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>::get_resolved_feature()
	 * 
	 * ...in a function for boost-python to recognise the 'this' pointer type...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>
	 * 
	 * ...so it can convert from...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	 */
	bp::object
	resolved_topological_line_get_resolved_feature(
			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine> &resolved_topological_line)
	{
		return resolved_topological_line.get_resolved_feature();
	}
}


void
export_resolved_topological_line()
{
	//
	// ResolvedTopologicalLine - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalLine,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ResolvedTopologicalLine",
					"The geometry of a topological *line* feature resolved to a geological time.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalLine* instances.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ResolvedTopologicalLine`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The returned feature is what was used to generate this :class:`ResolvedTopologicalLine` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the topological line property associated with "
				"this :class:`ResolvedTopologicalLine`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`get_resolved_line` and "
				":meth:`get_resolved_geometry` are obtained from.\n")
		.def("get_resolved_line",
				&GPlatesApi::resolved_topological_line_get_resolved_line,
				"get_resolved_line()\n"
				"  Returns the resolved line geometry.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_line_get_resolved_line,
				"get_resolved_geometry()\n"
				"  Same as :meth:`get_resolved_line`.\n")
		.def("get_resolved_feature",
				&GPlatesApi::resolved_topological_line_get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved line geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the static :meth:`resolved geometry<get_resolved_geometry>`. "
				"Unlike :meth:`get_feature` it cannot be used to generate a :class:`ResolvedTopologicalLine` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_feature`\n")
		.def("get_line_sub_segments",
				&GPlatesApi::resolved_topological_line_get_line_sub_segments,
				"get_line_sub_segments()\n"
				"  Returns the :class:`sub-segments<ResolvedTopologicalSubSegment>` that make up the "
				"line of this resolved topological line.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSubSegment`\n"
				"\n"
				"  To get a list of the *unreversed* sub-segment geometries:\n"
				"  ::\n"
				"\n"
				"    sub_segment_geometries = []\n"
				"    for sub_segment in resolved_topological_line.get_line_sub_segments():\n"
				"        sub_segment_geometries.append(sub_segment.get_resolved_geometry())\n")
		.def("get_geometry_sub_segments",
				&GPlatesApi::resolved_topological_line_get_line_sub_segments,
				"get_geometry_sub_segments()\n"
				"  Same as :meth:`get_line_sub_segments`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ResolvedTopologicalLine>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ResolvedTopologicalLine>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ResolvedTopologicalLine>
	// to a GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ResolvedTopologicalLine pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ResolvedTopologicalLine pointer to a ReconstructionGeometry::non_null_ptr_type.

	// Enable a python-wrapped ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	// to be from-python converted to a ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>.
	GPlatesApi::register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper<
			GPlatesAppLogic::ResolvedTopologicalLine>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalLine
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ResolvedTopologicalLine>();
}


namespace GPlatesApi
{
	/**
	 * Returns the resolved boundary geometry.
	 */
	GPlatesAppLogic::ResolvedTopologicalBoundary::resolved_topology_boundary_ptr_type
	resolved_topological_boundary_get_resolved_boundary(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_topological_boundary)
	{
		return resolved_topological_boundary.resolved_topology_boundary();
	}

	bp::list
	resolved_topological_boundary_get_boundary_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_topological_boundary)
	{
		bp::list boundary_sub_segments_list;

		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_boundary.get_sub_segment_sequence();
		BOOST_FOREACH(const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment, sub_segments)
		{
			boundary_sub_segments_list.append(sub_segment);
		}

		return boundary_sub_segments_list;
	}

	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary) :
		d_resolved_topological_boundary(
				// We wrap *non-const* reconstruction geometries and hence cast away const
				// (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalBoundary>(resolved_topological_boundary)),
		d_keep_feature_property_alive(*resolved_topological_boundary)
	{
		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_boundary->get_sub_segment_sequence();

		d_sub_segments.reserve(sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment,
				sub_segments)
		{
			d_sub_segments.push_back(
					ResolvedTopologicalGeometrySubSegmentWrapper(sub_segment));
		}
	}

	bp::object
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature =
					reconstruction_geometry_get_feature(*d_resolved_topological_boundary);
			if (!feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_geometry =
					d_resolved_topological_boundary->resolved_topology_boundary();

			// Create the resolved feature.
			bp::object resolved_feature_object =
					create_resolved_feature(feature.get(), d_resolved_topological_boundary, resolved_geometry);
			if (resolved_feature_object == bp::object()/*Py_None*/)
			{
				return bp::object()/*Py_None*/;
			}

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}

	/**
	 * For some reason we need to wrap...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>::get_resolved_feature()
	 * 
	 * ...in a function for boost-python to recognise the 'this' pointer type...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>
	 * 
	 * ...so it can convert from...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	 */
	bp::object
	resolved_topological_boundary_get_resolved_feature(
			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary> &resolved_topological_boundary)
	{
		return resolved_topological_boundary.get_resolved_feature();
	}
}


void
export_resolved_topological_boundary()
{
	//
	// ResolvedTopologicalBoundary - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalBoundary,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ResolvedTopologicalBoundary",
					"The geometry of a topological *boundary* feature resolved to a geological time.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalBoundary* instances.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ResolvedTopologicalBoundary`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The returned feature is what was used to generate this :class:`ResolvedTopologicalBoundary` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the topological boundary property associated with "
				"this :class:`ResolvedTopologicalBoundary`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`get_resolved_boundary` and "
				":meth:`get_resolved_geometry` are obtained from.\n")
		.def("get_resolved_boundary",
				&GPlatesApi::resolved_topological_boundary_get_resolved_boundary,
				"get_resolved_boundary()\n"
				"  Returns the resolved boundary geometry.\n"
				"\n"
				"  :rtype: :class:`PolygonOnSphere`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_boundary_get_resolved_boundary,
				"get_resolved_geometry()\n"
				"  Same as :meth:`get_resolved_boundary`.\n")
		.def("get_resolved_feature",
				&GPlatesApi::resolved_topological_boundary_get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved boundary geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the static :meth:`resolved geometry<get_resolved_geometry>`. "
				"Unlike :meth:`get_feature` it cannot be used to generate a :class:`ResolvedTopologicalBoundary` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_feature`\n")
		.def("get_boundary_sub_segments",
				&GPlatesApi::resolved_topological_boundary_get_boundary_sub_segments,
				"get_boundary_sub_segments()\n"
				"  Returns the :class:`sub-segments<ResolvedTopologicalSubSegment>` that make up the "
				"boundary of this resolved topological boundary.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSubSegment`\n"
				"\n"
				"  To get a list of the *unreversed* boundary sub-segment geometries:\n"
				"  ::\n"
				"\n"
				"    sub_segment_geometries = []\n"
				"    for sub_segment in resolved_topological_boundary.get_boundary_sub_segments():\n"
				"        sub_segment_geometries.append(sub_segment.get_resolved_geometry())\n")
		.def("get_geometry_sub_segments",
				&GPlatesApi::resolved_topological_boundary_get_boundary_sub_segments,
				"get_geometry_sub_segments()\n"
				"  Same as :meth:`get_boundary_sub_segments`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ResolvedTopologicalBoundary>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ResolvedTopologicalBoundary>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ResolvedTopologicalBoundary>
	// to a GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ResolvedTopologicalBoundary pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ResolvedTopologicalBoundary pointer to a ReconstructionGeometry::non_null_ptr_type.

	// Enable a python-wrapped ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	// to be from-python converted to a ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>.
	GPlatesApi::register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper<
			GPlatesAppLogic::ResolvedTopologicalBoundary>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalBoundary
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ResolvedTopologicalBoundary>();
}


namespace GPlatesApi
{
	/**
	 * Returns the resolved boundary of this network.
	 */
	GPlatesAppLogic::ResolvedTopologicalNetwork::boundary_polygon_ptr_type
	resolved_topological_network_get_resolved_boundary(
			const GPlatesAppLogic::ResolvedTopologicalNetwork &resolved_topological_network)
	{
		return resolved_topological_network.boundary_polygon();
	}

	bp::list
	resolved_topological_network_get_boundary_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalNetwork &resolved_topological_network)
	{
		bp::list boundary_sub_segments_list;

		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_network.get_boundary_sub_segment_sequence();
		BOOST_FOREACH(const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment, sub_segments)
		{
			boundary_sub_segments_list.append(sub_segment);
		}

		return boundary_sub_segments_list;
	}

	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network) :
		d_resolved_topological_network(
				// We wrap *non-const* reconstruction geometries and hence cast away const
				// (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalNetwork>(resolved_topological_network)),
		d_keep_feature_property_alive(*resolved_topological_network)
	{
		const GPlatesAppLogic::sub_segment_seq_type &boundary_sub_segments =
				resolved_topological_network->get_boundary_sub_segment_sequence();

		d_boundary_sub_segments.reserve(boundary_sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &boundary_sub_segment,
				boundary_sub_segments)
		{
			d_boundary_sub_segments.push_back(
					ResolvedTopologicalGeometrySubSegmentWrapper(boundary_sub_segment));
		}
	}

	bp::object
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature =
					reconstruction_geometry_get_feature(*d_resolved_topological_network);
			if (!feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_geometry =
					d_resolved_topological_network->boundary_polygon();

			// Create the resolved feature.
			bp::object resolved_feature_object =
					create_resolved_feature(feature.get(), d_resolved_topological_network, resolved_geometry);
			if (resolved_feature_object == bp::object()/*Py_None*/)
			{
				return bp::object()/*Py_None*/;
			}

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}

	/**
	 * For some reason we need to wrap...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>::get_resolved_feature()
	 * 
	 * ...in a function for boost-python to recognise the 'this' pointer type...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>
	 * 
	 * ...so it can convert from...
	 * 
	 *   ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	 */
	bp::object
	resolved_topological_network_get_resolved_feature(
			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork> &resolved_topological_network)
	{
		return resolved_topological_network.get_resolved_feature();
	}
}


void
export_resolved_topological_network()
{
	//
	// ResolvedTopologicalNetwork - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalNetwork,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ResolvedTopologicalNetwork",
					"The geometry of a topological *network* feature resolved to a geological time.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalNetwork* instances.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The returned feature is what was used to generate this :class:`ResolvedTopologicalNetwork` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the topological network property associated with "
				"this :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`get_resolved_boundary` is obtained from.\n")
		.def("get_resolved_boundary",
				&GPlatesApi::resolved_topological_network_get_resolved_boundary,
				"get_resolved_boundary()\n"
				"  Returns the resolved boundary of this network.\n"
				"\n"
				"  :rtype: :class:`PolygonOnSphere`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_network_get_resolved_boundary,
				"get_resolved_geometry()\n"
				"  Same as :meth:`get_resolved_boundary`.\n")
		.def("get_resolved_feature",
				&GPlatesApi::resolved_topological_network_get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved boundary geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the static :meth:`resolved geometry<get_resolved_geometry>`. "
				"Unlike :meth:`get_feature` it cannot be used to generate a :class:`ResolvedTopologicalNetwork` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_feature`\n")
		.def("get_boundary_sub_segments",
				&GPlatesApi::resolved_topological_network_get_boundary_sub_segments,
				"get_boundary_sub_segments()\n"
				"  Returns the :class:`sub-segments<ResolvedTopologicalSubSegment>` that make up the "
				"boundary of this resolved topological network.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSubSegment`\n"
				"\n"
				"  To get a list of the *unreversed* boundary sub-segment geometries:\n"
				"  ::\n"
				"\n"
				"    sub_segment_geometries = []\n"
				"    for sub_segment in resolved_topological_network.get_boundary_sub_segments():\n"
				"        sub_segment_geometries.append(sub_segment.get_resolved_geometry())\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type to be to-python converted to
	// a python-wrapped ReconstructionGeometryTypeWrapper<ResolvedTopologicalNetwork>.
	GPlatesApi::register_to_python_conversion_reconstruction_geometry_type_non_null_intrusive_ptr_to_wrapper<
			GPlatesAppLogic::ResolvedTopologicalNetwork>();
	//
	// From-python conversion from python-wrapped ReconstructionGeometryTypeWrapper<ResolvedTopologicalNetwork>
	// to a GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type is provided by
	// get_pointer(ReconstructionGeometryTypeWrapper) in header to create a ResolvedTopologicalNetwork pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ResolvedTopologicalNetwork pointer to a ReconstructionGeometry::non_null_ptr_type.

	// Enable a python-wrapped ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	// to be from-python converted to a ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>.
	GPlatesApi::register_from_python_conversion_base_to_derived_reconstruction_geometry_wrapper<
			GPlatesAppLogic::ResolvedTopologicalNetwork>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalNetwork
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ResolvedTopologicalNetwork>();
}


namespace GPlatesApi
{
	/**
	 * Returns the referenced feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	resolved_topological_geometry_sub_segment_get_topological_section_feature(
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment)
	{
		return reconstruction_geometry_get_feature(
				*resolved_topological_geometry_sub_segment.get_reconstruction_geometry());
	}

	// Convert sub-segment geometries to polylines (even if they are just single points - just duplicate).
	// This will make it easier for the user who will expect sub-segments to be polylines.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	resolved_topological_geometry_sub_segment_get_resolved_geometry(
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment)
	{
		return convert_geometry_to_polyline(
				resolved_topological_geometry_sub_segment.get_geometry());
	}

	// The topological section might not be a reconstructed feature geometry or a resolved topological *line*.
	// It should normally be one though so we don't document that Py_None could be returned to the caller.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	resolved_topological_geometry_sub_segment_get_topological_section_geometry(
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment)
	{
		return get_topological_section_geometry(
				resolved_topological_geometry_sub_segment.get_reconstruction_geometry());
	}

	bp::object
	ResolvedTopologicalGeometrySubSegmentWrapper::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> topological_section_feature =
					resolved_topological_geometry_sub_segment_get_topological_section_feature(d_resolved_topological_geometry_sub_segment);
			if (!topological_section_feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_sub_segment_geometry =
					resolved_topological_geometry_sub_segment_get_resolved_geometry(d_resolved_topological_geometry_sub_segment);

			// Create the resolved feature.
			bp::object resolved_feature_object =
					create_resolved_feature(
							topological_section_feature.get(),
							d_resolved_topological_geometry_sub_segment.get_reconstruction_geometry(),
							resolved_sub_segment_geometry);
			if (resolved_feature_object == bp::object()/*Py_None*/)
			{
				return bp::object()/*Py_None*/;
			}

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}


	/**
	 * To-python converter from a 'ResolvedTopologicalGeometrySubSegment' to a
	 * 'ResolvedTopologicalGeometrySubSegmentWrapper'.
	 */
	struct ToPythonConversionResolvedTopologicalGeometrySubSegment :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment)
			{
				// Convert to ResolvedTopologicalGeometrySubSegmentWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ResolvedTopologicalGeometrySubSegmentWrapper(sub_segment)).ptr());
			}
		};
	};


	/**
	 * Registers converter from a 'ResolvedTopologicalGeometrySubSegment' to a
	 * 'ResolvedTopologicalGeometrySubSegmentWrapper'.
	 */
	void
	register_to_python_conversion_resolved_topological_geometry_sub_segment()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment,
				ToPythonConversionResolvedTopologicalGeometrySubSegment::Conversion>();
	}
}


void
export_resolved_topological_sub_segment()
{
	//
	// ResolvedTopologicalSubSegment - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment,
			GPlatesApi::ResolvedTopologicalGeometrySubSegmentWrapper,
			// NOTE: We must specify 'boost::noncopyable' otherwise boost-python will expect our wrapper
			// to have a constructor accepting a 'pointer to non-const' and our wrapper will be
			// responsible for deleting the pointer when it's done...
			boost::noncopyable>(
					"ResolvedTopologicalSubSegment",
					"The subset of vertices of a reconstructed topological section that contribute to the "
					"geometry of a resolved topology.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate resolved topologies "
					"(such as :class:`ResolvedTopologicalLine`, :class:`ResolvedTopologicalBoundary` and "
					":class:`ResolvedTopologicalNetwork`) which, in turn, reference these "
					"*ResolvedTopologicalSubSegment* instances.\n"
					"\n"
					".. note:: | Each *ResolvedTopologicalSubSegment* instance belongs to a *single* resolved topology.\n"
					"          | In contrast, a :class:`ResolvedTopologicalSharedSubSegment` instance can be shared "
					"by one or more resolved topologies.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("get_resolved_feature",
				&GPlatesApi::ResolvedTopologicalGeometrySubSegmentWrapper::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved sub-segment geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the :meth:`resolved geometry<get_resolved_geometry>`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because the topological section might be a :class:`ResolvedTopologicalLine` "
				"which is a topology and topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section_feature`\n")
		.def("get_feature",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_topological_section_feature,
				"get_feature()\n"
				"  Same as :meth:`get_topological_section_feature`.\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_resolved_geometry,
				"get_resolved_geometry()\n"
				"  Returns the geometry containing the sub-segment vertices.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: These are the *unreversed* vertices. They are in the same order as the "
				"geometry of :meth:`get_topological_section_geometry`.\n"
				"\n"
				"  .. seealso:: :meth:`was_geometry_reversed_in_topology`\n")
		.def("get_geometry",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_resolved_geometry,
				"get_geometry()\n"
				"  Same as :meth:`get_resolved_geometry`.\n")
		.def("get_topological_section",
				&GPlatesApi::ResolvedTopologicalGeometrySubSegmentWrapper::get_reconstruction_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_topological_section()\n"
				"  Returns the topological section that the sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: This represents the **entire** geometry of the topological section, not just "
				"the part that contributes to the sub-segment.\n"
				"\n"
				"  .. note:: | If the resolved topology (that this sub-segment is a part of) is a "
				":class:`ResolvedTopologicalLine` then the topological section will be a "
				":class:`ReconstructedFeatureGeometry`.\n"
				"            | If the resolved topology (that this sub-segment is a part of) is a "
				":class:`ResolvedTopologicalBoundary` or a :class:`ResolvedTopologicalNetwork` then "
				"the topological section can be either a :class:`ReconstructedFeatureGeometry` or "
				"a :class:`ResolvedTopologicalLine`.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section_feature`\n")
		.def("get_topological_section_geometry",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_topological_section_geometry,
				"get_topological_section_geometry()\n"
				"  Returns the topological section *geometry* that the sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: This is the **entire** geometry of the topological section, not just "
				"the part that contributes to the sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section`\n")
		.def("get_topological_section_feature",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_topological_section_feature,
				"get_topological_section_feature()\n"
				"  Returns the feature referenced by the topological section.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The geometry in the returned feature represents the **entire** "
				"geometry of the topological section, not just the "
				"part that contributes to the sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("was_geometry_reversed_in_topology",
				&GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment::get_use_reverse,
				"was_geometry_reversed_in_topology()\n"
				"  Whether a copy of the points in :meth:`get_resolved_geometry` were reversed in order to "
				"contribute to the resolved topology that this sub-segment is a part of.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  .. note:: A reversed version of the points of :meth:`get_resolved_geometry` is equivalent "
				"``sub_segment.get_resolved_geometry().get_points()[::-1]``.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_geometry`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable a GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment to be converted
	// to-python as a python-wrapped ResolvedTopologicalGeometrySubSegmentWrapper.
	GPlatesApi::register_to_python_conversion_resolved_topological_geometry_sub_segment();
	//
	// From-python conversion from python-wrapped ResolvedTopologicalGeometrySubSegmentWrapper
	// to a GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment reference is provided by
	// get_pointer(ResolvedTopologicalGeometrySubSegmentWrapper) in header to create a
	// ResolvedTopologicalGeometrySubSegment pointer.

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment
	// (not ResolvedTopologicalGeometrySubSegmentWrapper).
	//

	// Enable boost::optional<ResolvedTopologicalGeometrySubSegment> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment>();
}


namespace GPlatesApi
{
	/**
	 * Returns the referenced topological section feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	resolved_topological_shared_sub_segment_get_topological_section_feature(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		return reconstruction_geometry_get_feature(
				*resolved_topological_shared_sub_segment.get_reconstruction_geometry());
	}

	// Convert sub-segment geometries to polylines (even if they are just single points - just duplicate).
	// This will make it easier for the user who will expect sub-segments to be polylines.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	resolved_topological_shared_sub_segment_get_resolved_geometry(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		return convert_geometry_to_polyline(
				resolved_topological_shared_sub_segment.get_geometry());
	}

	// The topological section might not be a reconstructed feature geometry or a resolved topological *line*.
	// It should normally be one though so we don't document that Py_None could be returned to the caller.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	resolved_topological_shared_sub_segment_get_topological_section_geometry(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		return get_topological_section_geometry(
				resolved_topological_shared_sub_segment.get_reconstruction_geometry());
	}

	/**
	 * Get the geometry reversal flags of the resolved topologies sharing the sub-segment (for passing to Python).
	 */
	bp::list
	resolved_topological_shared_sub_segment_get_sharing_resolved_topology_geometry_reversal_flags(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		bp::list geometry_reversal_flags_list;

		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> &
				sharing_resolved_topologies =
						resolved_topological_shared_sub_segment.get_sharing_resolved_topologies();

		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo &sharing_resolved_topology,
				sharing_resolved_topologies)
		{
			geometry_reversal_flags_list.append(
					sharing_resolved_topology.is_sub_segment_geometry_reversed);
		}

		return geometry_reversal_flags_list;
	}


	ResolvedTopologicalSharedSubSegmentWrapper::ResolvedTopologicalSharedSubSegmentWrapper(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment) :
		d_resolved_topological_shared_sub_segment(resolved_topological_shared_sub_segment),
		d_reconstruction_geometry(resolved_topological_shared_sub_segment.get_reconstruction_geometry())
	{
		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> &
				sharing_resolved_topologies =
						resolved_topological_shared_sub_segment.get_sharing_resolved_topologies();

		// Wrap the resolved topologies to keep their feature/property alive.
		d_sharing_resolved_topologies.reserve(sharing_resolved_topologies.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo &resolved_topology_info,
				sharing_resolved_topologies)
		{
			d_sharing_resolved_topologies.push_back(
					ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>(
							resolved_topology_info.resolved_topology));
		}
	}


	bp::object
	ResolvedTopologicalSharedSubSegmentWrapper::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> topological_section_feature =
					resolved_topological_shared_sub_segment_get_topological_section_feature(d_resolved_topological_shared_sub_segment);
			if (!topological_section_feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_sub_segment_geometry =
					resolved_topological_shared_sub_segment_get_resolved_geometry(d_resolved_topological_shared_sub_segment);

			// Create the resolved feature.
			bp::object resolved_feature_object =
					create_resolved_feature(
							topological_section_feature.get(),
							d_resolved_topological_shared_sub_segment.get_reconstruction_geometry(),
							resolved_sub_segment_geometry);
			if (resolved_feature_object == bp::object()/*Py_None*/)
			{
				return bp::object()/*Py_None*/;
			}

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}


	bp::list
	ResolvedTopologicalSharedSubSegmentWrapper::get_sharing_resolved_topologies() const
	{
		bp::list sharing_resolved_topologies_list;

		BOOST_FOREACH(
				const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &resolved_topology,
				d_sharing_resolved_topologies)
		{
			sharing_resolved_topologies_list.append(resolved_topology);
		}

		return sharing_resolved_topologies_list;
	}


	/**
	 * To-python converter from a 'ResolvedTopologicalSharedSubSegment' to a
	 * 'ResolvedTopologicalSharedSubSegmentWrapper'.
	 */
	struct ToPythonConversionResolvedTopologicalSharedSubSegment :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &sub_segment)
			{
				// Convert to ResolvedTopologicalSharedSubSegmentWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ResolvedTopologicalSharedSubSegmentWrapper(sub_segment)).ptr());
			}
		};
	};


	/**
	 * Registers converter from a 'ResolvedTopologicalSharedSubSegment' to a
	 * 'ResolvedTopologicalSharedSubSegmentWrapper'.
	 */
	void
	register_to_python_conversion_resolved_topological_shared_sub_segment()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ResolvedTopologicalSharedSubSegment,
				ToPythonConversionResolvedTopologicalSharedSubSegment::Conversion>();
	}
}


void
export_resolved_topological_shared_sub_segment()
{
	//
	// ResolvedTopologicalSharedSubSegment - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalSharedSubSegment,
			GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper,
			// NOTE: We must specify 'boost::noncopyable' otherwise boost-python will expect our wrapper
			// to have a constructor accepting a 'pointer to non-const' and our wrapper will be
			// responsible for deleting the pointer when it's done...
			boost::noncopyable>(
					"ResolvedTopologicalSharedSubSegment",
					"The shared subset of vertices of a reconstructed topological section that uniquely "
					"contribute to the *boundaries* of one or more resolved topologies.\n"
					"\n"
					".. note:: | Only :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork` "
					"have *boundaries* and hence will share *ResolvedTopologicalSharedSubSegment* instances.\n"
					"          | :class:`ResolvedTopologicalLine` is excluded since it does not have a *boundary*.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate resolved topology *boundaries* "
					"(:class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`) and "
					"the shared *ResolvedTopologicalSharedSubSegment* instances.\n"
					"\n"
					".. note:: | Each *ResolvedTopologicalSharedSubSegment* instance can be shared "
					"by one or more resolved topologies.\n"
					"          | In contrast, a :class:`ResolvedTopologicalSubSegment` instance "
					"belongs to a *single* resolved topology.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("get_resolved_feature",
				&GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved sub-segment geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the :meth:`resolved geometry<get_resolved_geometry>`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because the topological section might be a :class:`ResolvedTopologicalLine` "
				"which is a topology and topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section_feature`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_resolved_geometry,
				"get_resolved_geometry()\n"
				"  Returns the geometry containing the shared sub-segment vertices.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: These are the *unreversed* vertices. They are in the same order as the "
				"geometry of :meth:`get_topological_section_geometry`.\n")
		.def("get_geometry",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_resolved_geometry,
				"get_geometry()\n"
				"  Same as :meth:`get_resolved_geometry`.\n")
		.def("get_topological_section",
				&GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper::get_reconstruction_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_topological_section()\n"
				"  Returns the topological section that the shared sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: This represents the **entire** geometry of the topological section, not just "
				"the part that contributes to the shared sub-segment.\n"
				"\n"
				"  .. note:: | This topological section can be either a "
				":class:`ReconstructedFeatureGeometry` or a :class:`ResolvedTopologicalLine`.\n"
				"            | The resolved topologies that share the topological section can be "
				":class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  .. seealso:: :class:`get_topological_section_feature`\n")
		.def("get_topological_section_geometry",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_topological_section_geometry,
				"get_topological_section_geometry()\n"
				"  Returns the topological section *geometry* that the shared sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: This is the **entire** geometry of the topological section, not just "
				"the part that contributes to the shared sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section`\n")
		.def("get_topological_section_feature",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_topological_section_feature,
				"get_topological_section_feature()\n"
				"  Returns the feature referenced by the topological section.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The geometry in the returned feature represents the **entire** "
				"geometry of the topological section, not just the "
				"part that contributes to the shared sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_feature",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_topological_section_feature,
				"get_feature()\n"
				"  Same as :meth:`get_topological_section_feature`.\n")
		.def("get_sharing_resolved_topologies",
				&GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper::get_sharing_resolved_topologies,
				"get_sharing_resolved_topologies()\n"
				"  Returns a list of resolved topologies sharing this sub-segment.\n"
				"\n"
				"  :rtype: list of :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: | The resolved topologies (that share this sub-segment) can be "
				":class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.\n"
				"            | :class:`ResolvedTopologicalLine` is excluded since it does not have a *boundary*.\n"
				"\n"
				"  .. seealso:: :meth:`get_sharing_resolved_topology_geometry_reversal_flags`\n")
		.def("get_sharing_resolved_topology_geometry_reversal_flags",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_sharing_resolved_topology_geometry_reversal_flags,
				"get_sharing_resolved_topology_geometry_reversal_flags()\n"
				"  Returns a list of flags indicating whether a copy of the sub-segment geometry was reversed "
				"when contributing to each resolved topology sharing this sub-segment.\n"
				"\n"
				"  :rtype: list of bool\n"
				"\n"
				"  .. seealso:: :meth:`ResolvedTopologicalSubSegment.was_geometry_reversed_in_topology`\n"
				"  .. note::\n"
				"     The returned list is in the same order (and has the same number of elements) as the "
				"list of sharing resolved topologies returned in :meth:`get_sharing_resolved_topologies`.\n"
				"\n"
				"     ::\n"
				"\n"
				"       sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()\n"
				"       geometry_reversal_flags = shared_sub_segment.get_sharing_resolved_topology_geometry_reversal_flags()\n"
				"       for index in range(len(sharing_resolved_topologies)):\n"
				"           sharing_resolved_topology = sharing_resolved_topologies[index]\n"
				"           geometry_reversal_flag = geometry_reversal_flags[index]\n"
				"\n"
				"  .. seealso:: :meth:`get_sharing_resolved_topologies`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable a GPlatesAppLogic::ResolvedTopologicalSharedSubSegment to be converted
	// to-python as a python-wrapped ResolvedTopologicalSharedSubSegmentWrapper.
	GPlatesApi::register_to_python_conversion_resolved_topological_shared_sub_segment();
	//
	// From-python conversion from python-wrapped ResolvedTopologicalSharedSubSegmentWrapper
	// to a GPlatesAppLogic::ResolvedTopologicalSharedSubSegment reference is provided by
	// get_pointer(ResolvedTopologicalSharedSubSegmentWrapper) in header to create a
	// ResolvedTopologicalSharedSubSegment pointer.

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalSharedSubSegment
	// (not ResolvedTopologicalSharedSubSegmentWrapper).
	//

	// Enable boost::optional<ResolvedTopologicalSharedSubSegment> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ResolvedTopologicalSharedSubSegment>();
}


namespace GPlatesApi
{
	/**
	 * Returns the referenced feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	resolved_topological_section_get_topological_section_feature(
			const GPlatesAppLogic::ResolvedTopologicalSection &resolved_topological_section)
	{
		return reconstruction_geometry_get_feature(
				*resolved_topological_section.get_reconstruction_geometry());
	}

	// The topological section might not be a reconstructed feature geometry or a resolved topological *line*.
	// It should normally be one though so we don't document that Py_None could be returned to the caller.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	resolved_topological_section_get_topological_section_geometry(
			const GPlatesAppLogic::ResolvedTopologicalSection &resolved_topological_section)
	{
		return get_topological_section_geometry(
				resolved_topological_section.get_reconstruction_geometry());
	}


	ResolvedTopologicalSectionWrapper::ResolvedTopologicalSectionWrapper(
			const GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_to_const_type &resolved_topological_section) :
		d_resolved_topological_section(
				// We wrap *non-const* resolved topological sections and hence cast away const
				// (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalSection>(resolved_topological_section)),
		d_reconstruction_geometry(resolved_topological_section->get_reconstruction_geometry())
	{
		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment> &shared_sub_segments =
				resolved_topological_section->get_shared_sub_segments();

		// Wrap the resolved topologies to keep their feature/property alive.
		d_shared_sub_segments.reserve(shared_sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &shared_sub_segment,
				shared_sub_segments)
		{
			d_shared_sub_segments.push_back(
					ResolvedTopologicalSharedSubSegmentWrapper(shared_sub_segment));
		}
	}

	
	bp::list
	ResolvedTopologicalSectionWrapper::get_shared_sub_segments() const
	{
		bp::list shared_sub_segments_list;

		BOOST_FOREACH(
				const ResolvedTopologicalSharedSubSegmentWrapper &shared_sub_segment,
				d_shared_sub_segments)
		{
			shared_sub_segments_list.append(shared_sub_segment);
		}

		return shared_sub_segments_list;
	}


	/**
	 * To-python converter from a 'non_null_intrusive_ptr<ResolvedTopologicalSection>' to a
	 * 'ResolvedTopologicalSectionWrapper'.
	 */
	struct ToPythonConversionResolvedTopologicalSection :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type &section)
			{
				// Convert to ResolvedTopologicalSectionWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ResolvedTopologicalSectionWrapper(section)).ptr());
			}
		};
	};


	/**
	 * Registers converter from a 'non_null_intrusive_ptr<ResolvedTopologicalSection>' to a
	 * 'ResolvedTopologicalSectionWrapper'.
	 */
	void
	register_to_python_conversion_resolved_topological_section()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type,
				ToPythonConversionResolvedTopologicalSection::Conversion>();
	}
}


void
export_resolved_topological_section()
{
	//
	// ResolvedTopologicalSection - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalSection,
			GPlatesApi::ResolvedTopologicalSectionWrapper,
			boost::noncopyable>(
					"ResolvedTopologicalSection",
					"The sequence of shared sub-segments of a reconstructed topological section that uniquely "
					"contribute to the *boundaries* of one or more resolved topologies.\n"
					"\n"
					".. note:: | Only :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork` "
					"have *boundaries* and hence will share sub-segments.\n"
					"          | :class:`ResolvedTopologicalLine` is excluded since it does not have a *boundary*.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate resolved topology *boundaries* "
					"(:class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`) and "
					"*ResolvedTopologicalSection* instances.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("get_topological_section_feature",
				&GPlatesApi::resolved_topological_section_get_topological_section_feature,
				"get_topological_section_feature()\n"
				"  Returns the feature referenced by the topological section.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The geometry in the returned feature represents the **entire** geometry of the "
				"topological section, not just the parts that contribute to resolved topological boundaries.\n")
		.def("get_feature",
				&GPlatesApi::resolved_topological_section_get_topological_section_feature,
				"get_feature()\n"
				"  Same as :meth:`get_topological_section_feature`.\n")
		.def("get_topological_section",
				&GPlatesApi::ResolvedTopologicalSectionWrapper::get_reconstruction_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_topological_section()\n"
				"  Returns the topological section.\n"
				"\n"
				"  :rtype: :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: This represents the **entire** geometry of the topological section, not just "
				"the parts that contribute to resolved topological boundaries.\n"
				"\n"
				"  .. note:: | This topological section can be either a "
				":class:`ReconstructedFeatureGeometry` or a :class:`ResolvedTopologicalLine`.\n"
				"            | The resolved topologies that share the topological section can be "
				":class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  .. seealso:: :class:`get_topological_section_feature`\n")
		.def("get_topological_section_geometry",
				&GPlatesApi::resolved_topological_section_get_topological_section_geometry,
				"get_topological_section_geometry()\n"
				"  Returns the topological section *geometry*.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: This is the **entire** geometry of the topological section, not just "
				"the parts that contribute to resolved topological boundaries.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section`\n")
		.def("get_shared_sub_segments",
				&GPlatesApi::ResolvedTopologicalSectionWrapper::get_shared_sub_segments,
				"get_shared_sub_segments()\n"
				"  Returns a list of sub-segments on this topological section that are *shared* by "
				"one or more resolved topologies.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSharedSubSegment`\n"
				"\n"
				"  Get the length of the shared sub-segments of a resolved topological section:\n"
				"  ::\n"
				"\n"
				"    length = 0\n"
				"    for shared_sub_segment in resolved_topological_section.get_shared_sub_segments():\n"
				"        length += shared_sub_segment.get_resolved_geometry().get_arc_length()\n"
				"\n"
				"    length_in_kms = length * pygplates.Earth.mean_radius_in_kms\n"
				"\n"
				"  .. note:: | The returned objects are :class:`ResolvedTopologicalSharedSubSegment` instances, "
				"**not** :class:`ResolvedTopologicalSubSegment` instances.\n"
				"            | :class:`ResolvedTopologicalSubSegment` instances are *not* shared by  one or "
				"more resolved topologies - each instance is associated with a *single* resolved topology.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable a GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type to be converted
	// to-python as a python-wrapped ResolvedTopologicalSectionWrapper.
	GPlatesApi::register_to_python_conversion_resolved_topological_section();
	//
	// From-python conversion from python-wrapped ResolvedTopologicalSectionWrapper
	// to a GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type is provided by
	// get_pointer(ResolvedTopologicalSectionWrapper) in header to create a ResolvedTopologicalSection pointer
	// combined with PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<>()
	// below to convert ResolvedTopologicalSection pointer to a ResolvedTopologicalSection::non_null_ptr_type.

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalSection
	// (not ResolvedTopologicalSectionWrapper).
	//

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ResolvedTopologicalSection>();
}


void
export_reconstruction_geometries()
{
	export_reconstruction_geometry();

	export_reconstructed_feature_geometry();
	export_reconstructed_motion_path();
	export_reconstructed_flowline();

	export_resolved_topological_sub_segment();
	export_resolved_topological_shared_sub_segment();
	export_resolved_topological_section();

	export_resolved_topological_line();
	export_resolved_topological_boundary();
	export_resolved_topological_network();
}

#endif // GPLATES_NO_PYTHON
