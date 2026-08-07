/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "app-logic/DeformationStrain.h"
#include "app-logic/DeformationStrainRate.h"

#include "maths/MathsUtils.h"

#include "global/python.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	// Zero strain rate.
	const GPlatesAppLogic::DeformationStrainRate zero_strain_rate;

	bp::tuple
	strain_rate_get_rate_of_deformation(
			const GPlatesAppLogic::DeformationStrainRate &strain_rate)
	{
		const GPlatesAppLogic::DeformationStrainRate::RateOfDeformation &rate_of_deformation =
				strain_rate.get_rate_of_deformation();

		return bp::make_tuple(
				rate_of_deformation.theta_theta,
				rate_of_deformation.theta_phi,
				rate_of_deformation.phi_theta,
				rate_of_deformation.phi_phi);
	}

	bp::tuple
	strain_rate_get_velocity_spatial_gradient(
			const GPlatesAppLogic::DeformationStrainRate &strain_rate)
	{
		const GPlatesAppLogic::DeformationStrainRate::VelocitySpatialGradient &velocity_spatial_gradient =
				strain_rate.get_velocity_spatial_gradient();

		return bp::make_tuple(
				velocity_spatial_gradient.theta_theta,
				velocity_spatial_gradient.theta_phi,
				velocity_spatial_gradient.phi_theta,
				velocity_spatial_gradient.phi_phi);
	}


	/**
	 * How to define a principal angle relative to the local spherical polar coordinate system.
	 */
	enum class PrincipalAngleType
	{
		//! -180 to +180 degrees anti-clockwise from North; 0 is South.
		MAJOR_SOUTH,

		//! -180 to +180 degrees anti-clockwise from West; 0 is East.
		MAJOR_EAST,

		//! 0 to 360 degrees clockwise from North; 0 is North.
		MAJOR_AZIMUTH
	};

	const GPlatesAppLogic::DeformationStrain identity_strain;

	bp::tuple
	strain_get_principal_strain(
			const GPlatesAppLogic::DeformationStrain &strain,
			PrincipalAngleType principal_angle_type)
	{
		const GPlatesAppLogic::DeformationStrain::StrainPrincipal &strain_principal =
				strain.get_strain_principal();

		double major_principal_angle;
		switch (principal_angle_type)
		{
		case PrincipalAngleType::MAJOR_SOUTH:
			// Angle remains unchanged.
			major_principal_angle = strain_principal.angle;
			break;
		case PrincipalAngleType::MAJOR_EAST:
			// Convert angle such that -pi to +pi radians is counter-clockwise from West and 0 is East.
			major_principal_angle = strain_principal.angle - GPlatesMaths::HALF_PI;
			// Make sure in range [-pi, pi].
			if (major_principal_angle > GPlatesMaths::PI)
			{
				major_principal_angle -= 2 * GPlatesMaths::PI;
			}
			else if (major_principal_angle < -GPlatesMaths::PI)
			{
				major_principal_angle += 2 * GPlatesMaths::PI;
			}
			break;
		case PrincipalAngleType::MAJOR_AZIMUTH:
			// Convert angle such that 0 to 2pi radians clockwise from North and 0 is North.
			major_principal_angle = GPlatesMaths::PI - strain_principal.angle;
			// Make sure in range [0, 2pi].
			if (major_principal_angle > 2 * GPlatesMaths::PI)
			{
				major_principal_angle -= 2 * GPlatesMaths::PI;
			}
			else if (major_principal_angle < 0.0)
			{
				major_principal_angle += 2 * GPlatesMaths::PI;
			}
			break;
		}

		return bp::make_tuple(
				strain_principal.principal1,
				strain_principal.principal2,
				major_principal_angle);
	}

	bp::tuple
	strain_get_deformation_gradient(
			const GPlatesAppLogic::DeformationStrain &strain)
	{
		const GPlatesAppLogic::DeformationStrain::DeformationGradient &deformation_gradient =
				strain.get_deformation_gradient();

		return bp::make_tuple(
				deformation_gradient.theta_theta,
				deformation_gradient.theta_phi,
				deformation_gradient.phi_theta,
				deformation_gradient.phi_phi);
	}
}


void
export_strain()
{
	// An enumeration for the type of principal angle.
	bp::enum_<GPlatesApi::PrincipalAngleType>("PrincipalAngleType")
			.value("major_south", GPlatesApi::PrincipalAngleType::MAJOR_SOUTH)
			.value("major_east", GPlatesApi::PrincipalAngleType::MAJOR_EAST)
			.value("major_azimuth", GPlatesApi::PrincipalAngleType::MAJOR_AZIMUTH);

	// Enable boost::optional<GPlatesApi::PrincipalAngleType> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::PrincipalAngleType>();


	//
	// StrainRate - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::DeformationStrainRate>(
					"StrainRate",
					"The strain rate at a particular location (parcel of crust) represents the rate at which deformation occurs at that parcel of crust.\n"
					"\n"
					"The strain rate is represented internally by the spatial gradients of velocity :math:`\\boldsymbol L` (in units of :math:`second^{-1}`) calculated in "
					"`spherical polar coordinates <https://www.brown.edu/Departments/Engineering/Courses/En221/Notes/Polar_Coords/Polar_Coords.htm>`_ (ignoring radial dimension):\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\boldsymbol L &= \\boldsymbol v \\boldsymbol \\nabla\\\\\n"
					"   \\begin{bmatrix} L_{\\theta\\theta} & L_{\\theta\\phi} \\\\ L_{\\phi\\theta} & L_{\\phi\\phi} \\end{bmatrix} &= "
					"\\begin{bmatrix} \\frac{1}{R} \\frac{\\partial v_\\theta}{\\partial \\theta} & "
					"\\frac{1}{R \\, \\sin{\\theta}} \\frac{\\partial v_\\theta}{\\partial \\phi} - \\cot{\\theta} \\frac{v_\\phi}{R}\\\\ "
					"\\frac{1}{R} \\frac{\\partial v_\\phi}{\\partial \\theta} & "
					"\\frac{1}{R \\, \\sin{\\theta}} \\frac{\\partial v_\\phi}{\\partial \\phi} + \\cot{\\theta} \\frac{v_\\theta}{R}\\end{bmatrix}\n"
					"\n"
					"...where :math:`\\boldsymbol v = (v_\\theta, v_\\phi)` is the velocity (in :math:`m / s`) in the local South-East coordinate system "
					"at a location (:math:`\\theta, \\phi`), and :math:`R` is the :class:`Earth mean radius<Earth>` (in :math:`m`).\n"
					"\n"
					".. note:: | The velocity spatial gradient :math:`\\boldsymbol L` is calculated at an arbitrary location in a deforming network using the following procedure:\n"
					"\n"
					"          | For each triangle in a deforming network's triangulation, an :math:`\\boldsymbol L` is calculated using velocities at the triangle's three vertices "
					"(and, in the above gradient calculation, :math:`\\cot{\\theta}`, :math:`v_\\theta` and :math:`v_\\phi` are calculated at the triangle's centroid location). "
					"Then each vertex in the entire triangulation is assigned an :math:`\\boldsymbol L` that is an area-weighted average of :math:`\\boldsymbol L`'s from faces incident to the vertex. "
					"Finally, :math:`\\boldsymbol L` at the arbitrary location is either assigned the :math:`\\boldsymbol L` of the triangle containing the location, or calculated using "
					"barycentric or natural neighbour interpolation of the :math:`\\boldsymbol L`'s from nearby vertices "
					"(depending on the :attr:`strain rate smoothing parameter <ResolveTopologyParameters.strain_rate_smoothing>` used to resolve the "
					":class:`deforming network <ResolvedTopologicalNetwork>`).\n"
					"\n"
					"The spatial gradients of velocity tensor (:math:`\\boldsymbol L`) can be decomposed into the rate-of-deformation tensor (:math:`\\boldsymbol D`) "
					"and the vorticity (or spin) tensor (:math:`\\boldsymbol W`):\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\boldsymbol L &= \\boldsymbol D + \\boldsymbol W\\\\\n"
					"   \\boldsymbol D &= \\frac{\\boldsymbol L + \\boldsymbol{L}^T}{2}\\\\\n"
					"   \\boldsymbol W &= \\frac{\\boldsymbol L - \\boldsymbol{L}^T}{2}\n"
					"\n"
					"...where :math:`D_{\\theta\\phi} = D_{\\phi\\theta}` (since the rate-of-deformation tensor :math:`\\boldsymbol D` is symmetric).\n"
					"\n"
					"If :math:`\\Lambda` is the stretch factor along current direction :math:`\\hat{\\boldsymbol n}` then the *rate of stretching per unit stretch* is given by:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\frac{\\dot{\\Lambda}}{\\Lambda} = \\hat{\\boldsymbol n} \\cdot \\boldsymbol D \\cdot \\hat{\\boldsymbol n}\n"
					"\n"
					"...where :math:`\\hat{\\boldsymbol n}` is a 2D unit vector in the local South-East coordinate system at a location (:math:`\\theta, \\phi`).\n"
					"\n"
					"So that means, in the local South direction (ie, :math:`\\hat{\\boldsymbol n} = (1,0)`) the *rate of stretching per unit stretch* is :math:`D_{\\theta\\theta}`, and "
					" in the local East direction (ie, :math:`\\hat{\\boldsymbol n} = (0,1)`) it is :math:`D_{\\phi\\phi}`. These are the *diagonal* elements of :math:`\\boldsymbol D`.\n"
					"\n"
					"The *rate of change of angle* :math:`\\alpha` between two current directions :math:`\\hat{\\boldsymbol n}_1` and :math:`\\hat{\\boldsymbol n}_2` is given by:\n"
					"\n"
					".. math::\n"
					"\n"
					"   -\\dot{\\alpha} = \\hat{\\boldsymbol n}_1 \\cdot 2 \\boldsymbol D \\cdot \\hat{\\boldsymbol n}_2\n"
					"\n"
					"The *shear rate* is commonly defined as half the *rate of change of angle* between two directions that are currently *perpendicular*. "
					"And if those directions are aligned with the local coordinate system (ie, :math:`\\hat{\\boldsymbol n}_1 = (1,0)` and :math:`\\hat{\\boldsymbol n}_2 = (0,1)`) then "
					"the *shear rate* is :math:`D_{\\phi\\theta}` (which is the same as :math:`D_{\\theta\\phi}` since :math:`\\boldsymbol D` is symmetric). "
					"So the symmetric *off-diagonal* elements represent the shear rate between the local coordinate axes (between South and East).\n"
					"\n"
					".. note:: The derivative of the Lagrangian finite strain tensor :math:`\\boldsymbol E` (see :class:`Strain`) is related to the rate-of-deformation tensor "
					":math:`\\boldsymbol D` (and the deformation gradient tensor :math:`\\boldsymbol F` - see :class:`Strain`):\n"
					"\n"
					"          .. math::\n"
					"\n"
					"             \\dot{\\boldsymbol E} = \\boldsymbol{F}^T \\cdot \\boldsymbol D \\cdot \\boldsymbol F\n"
					"\n"
					"          And so the *rate of stretching per unit stretch* :math:`\\frac{\\dot{\\Lambda}_{(\\hat{\\boldsymbol N})}}{\\Lambda_{(\\hat{\\boldsymbol N})}}` "
					"can be specified using a direction :math:`\\hat{\\boldsymbol N}` in the *initial* configuration (eg, at a time before deformation began):.\n"
					"\n"
					"          .. math::\n"
					"\n"
					"             \\frac{\\dot{\\Lambda}_{(\\hat{\\boldsymbol N})}}{\\Lambda_{(\\hat{\\boldsymbol N})}} = "
					"\\frac{\\hat{\\boldsymbol N} \\cdot \\dot{\\boldsymbol E} \\cdot \\hat{\\boldsymbol N}}{\\hat{\\boldsymbol N} \\cdot \\boldsymbol C \\cdot \\hat{\\boldsymbol N}}\n"
					"\n"
					"          ...rather than using a direction :math:`\\hat{\\boldsymbol n}` in the *current* configuration with "
					":math:`\\frac{\\dot{\\Lambda}_{(\\hat{\\boldsymbol n})}}{\\Lambda_{(\\hat{\\boldsymbol n})}} = "
					"\\hat{\\boldsymbol n} \\cdot \\boldsymbol D \\cdot \\hat{\\boldsymbol n}`. "
					"Note that :math:`\\boldsymbol C` is the Lagrangian deformation tensor (see :class:`Strain`).\n"
					"\n"
					"References:\n"
					"\n"
					"- Malvern, L. E. (1969). `Introduction to the mechanics of a continuous medium. <http://books.google.com/books?id=IIMpAQAAMAAJ>`_ Prentice-Hall.\n"
					"- Mase, G.T., Smelser, R.E., & Mase, G.E. (2010). `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ (3rd ed.). CRC Press.\n"
					"\n"
					"Strain rates are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
					"\n"
					"Convenience class static data is available for the zero strain rate:\n"
					"\n"
					"* ``pygplates.StrainRate.zero``\n"
					"\n"
					"A *StrainRate* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.46\n",
					bp::init<>(
							// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
							"__init__(...)\n"
							"A *StrainRate* object can be constructed in more than one way...\n"
							"\n"
							// Specific overload signature...
							"__init__()\n"
							"  Construct a zero strain rate (non-deforming).\n"
							"\n"
							"  ::\n"
							"\n"
							"    zero_strain_rate = pygplates.StrainRate()\n"
							"\n"
							"  .. note:: Alternatively you can use ``zero_strain_rate = pygplates.StrainRate.zero``.\n"))
		.def(bp::init<double,double,double,double>(
				(bp::arg("velocity_gradient_theta_theta"), bp::arg("velocity_gradient_theta_phi"), bp::arg("velocity_gradient_phi_theta"), bp::arg("velocity_gradient_phi_phi")),
				// Specific overload signature...
				"__init__(velocity_gradient_theta_theta,, velocity_gradient_theta_phi, velocity_gradient_phi_theta, velocity_gradient_phi_phi)\n"
				"  Create from the spatial gradients of velocity :math:`\\boldsymbol L` (in units of :math:`second^{-1}`) in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol L = \\begin{bmatrix} L_{\\theta\\theta} & L_{\\theta\\phi} \\\\ L_{\\phi\\theta} & L_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :param velocity_gradient_theta_theta: :math:`L_{\\theta\\theta}`\n"
				"  :type velocity_gradient_theta_theta: float\n"
				"  :param velocity_gradient_theta_phi: :math:`L_{\\theta\\phi}`\n"
				"  :type velocity_gradient_theta_phi: float\n"
				"  :param velocity_gradient_phi_theta: :math:`L_{\\phi\\theta}`\n"
				"  :type velocity_gradient_phi_theta: float\n"
				"  :param velocity_gradient_phi_phi: :math:`L_{\\phi\\phi}`\n"
				"  :type velocity_gradient_phi_phi: float\n"
				"\n"
				"  .. seealso:: :meth:`get_velocity_spatial_gradient` for the spatial gradients of velocity in spherical polar coordinates\n"))
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesAppLogic::DeformationStrainRate>>())
		// Static property 'pygplates.StrainRate.zero'...
		.def_readonly("zero", GPlatesApi::zero_strain_rate)
		.def("get_dilatation_rate",
				&GPlatesAppLogic::DeformationStrainRate::get_strain_rate_dilatation,
				"get_dilatation_rate()\n"
				"  Return the rate of change of crustal area per unit area (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The dilatation rate is the rate of increase (if positive) or decrease (if negative) of crustal area per unit area at the current location "
				"(at which this strain rate was calculated).\n"
				"\n"
				"  The dilatation rate is defined as the `trace` of the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` :math:`\\boldsymbol D` (sum of its diagonal elements). "
				"So if we define :math:`A` as the area of a parcel of crust at the current location, then the dilatation rate is the "
				"*rate of change of area per unit area*, and is given by:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\dot{A}}{A} = trace(\\boldsymbol D)\n"
				"\n"
				"  .. seealso::\n"
				"\n"
				"     Chapter 4.11 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ for a derivation of the material derivative of volume. "
				"We ignore the radial dimension, hence volume becomes area.\n"
				"\n"
				"  .. note:: The dilatation rate is *invariant* with respect to the local coordinate axes (South and East) since :math:`trace(\\boldsymbol D)` is "
				"the *first invariant* of :math:`\\boldsymbol D` (see Chapter 3.6 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_).\n"
				"\n"
				"  .. note:: As :class:`shown for the rate-of-deformation tensor<StrainRate>` :math:`\\boldsymbol D`, the *rate of stretching per unit stretch* "
				"along the local coordinate South and East axes are the *diagonal* elements :math:`D_{\\theta\\theta}` and :math:`D_{\\phi\\phi}` "
				"(which are included in :math:`trace(\\boldsymbol D)`). And the *off-diagonal* elements determine the instantaneous shear rate which does not affect "
				"expansion/contraction (and is excluded from :math:`trace(\\boldsymbol D))`.\n")
		.def("get_total_strain_rate",
				&GPlatesAppLogic::DeformationStrainRate::get_strain_rate_second_invariant,
				"get_total_strain_rate()\n"
				"  Return the total strain rate (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The total strain rate represents the magnitude, including both the normal (extension/compression) and shear components, of strain rate.\n"
				"\n"
				"  The total strain rate is defined in terms of the :meth:`rate-of-deformation symmetric tensor<get_rate_of_deformation>` :math:`\\boldsymbol D` as:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\sqrt{trace(\\boldsymbol{D}^2)} = \\sqrt{D_{\\theta\\theta}^2 + D_{\\phi\\phi}^2 + 2 D_{\\phi\\theta}^2}\n"
				"\n"
				"  ...where :math:`D_{\\phi\\theta} = D_{\\theta\\phi}` since :math:`\\boldsymbol D` is symmetric\n"
				"\n"
				"  .. note:: The total strain rate is *invariant* with respect to the local coordinate axes (South and East) since :math:`\\sqrt{trace(\\boldsymbol{D}^2)}` "
				"is invariant. This is because it is a function of the *second invariant*: :math:`\\frac{1}{2} \\left[trace(\\boldsymbol{D})^2 - trace(\\boldsymbol{D}^2)\\right]` "
				"and the *first invariant*: :math:`trace(\\boldsymbol D)` (see Chapter 3.6 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_).\n")
		.def("get_strain_rate_style",
				&GPlatesAppLogic::DeformationStrainRate::get_strain_rate_style,
				"get_strain_rate_style()\n"
				"  Return a measure categorising the type of deformation.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The strain rate *style* is a way to represent the type of deformation. The approach used here is from "
				"*Kreemer, C., G. Blewitt, and E. C. Klein (2014),* `A geodetic plate motion and Global Strain Rate Model <https://10.1002/2014GC005407>`_, "
				"which defines *strain rate style* as:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\dot{\\varepsilon}_{(1)} + \\dot{\\varepsilon}_{(2)}}{max(\\lvert \\dot{\\varepsilon}_{(1)} \\rvert, \\lvert \\dot{\\varepsilon}_{(2)} \\rvert)}\n"
				"\n"
				"  ...and is equivalent to:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{D_{(1)} + D_{(2)}}{max(\\lvert D_{(1)} \\rvert, \\lvert D_{(2)} \\rvert)}\n"
				"\n"
				"  ..where :math:`D_{(1)}` and :math:`D_{(2)}` are the principal values of the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` :math:`\\boldsymbol D`.\n"
				"\n"
				"  A value of `-1` represents contraction (eg, pure reverse faulting), `0` represents pure strike-slip faulting and `1` represents extension (eg, pure normal faulting).\n"
				"\n"
				"  .. warning:: | If the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` is zero (ie, no deformation) then `NaN` (zero divided by zero) will be returned.\n"
				"               | Also, the returned value is **not** clamped to the range `[-1, 1]` when *both* principal values are non-zero and have the same sign.\n"
				"\n"
				"  .. note:: The strain rate *style* is *invariant* with respect to the local coordinate axes (South and East) since it uses the principal values of :math:`\\boldsymbol D`.\n"
				"\n"
				"  .. note:: A similar measure for *style* is to divide :meth:`dilatation rate<get_dilatation_rate>` by :meth:`total strain rate<get_total_strain_rate>`. This is "
				"equivalent to :math:`\\frac{trace(\\boldsymbol D)}{\\sqrt{trace(\\boldsymbol{D}^2)}}` which is also *invariant* with respect to the local coordinate axes (South and East).\n")
		.def("get_rate_of_deformation",
				&GPlatesApi::strain_rate_get_rate_of_deformation,
				"get_rate_of_deformation()\n"
				"  Return the rate-of-deformation symmetric tensor :math:`\\boldsymbol D` (in units of :math:`second^{-1}`) in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol D = \\begin{bmatrix} D_{\\theta\\theta} & D_{\\theta\\phi} \\\\ D_{\\phi\\theta} & D_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :returns: the tuple of :math:`(D_{\\theta\\theta}, D_{\\theta\\phi}, D_{\\phi\\theta}, D_{\\phi\\phi})`\n"
				"  :rtype: tuple (float, float, float, float)\n"
				"\n"
				"  .. note:: :math:`\\theta` is **co**-latitude and hence increases from North to South (and :math:`\\phi` increases from West to East, as expected).\n")
		.def("get_velocity_spatial_gradient",
				&GPlatesApi::strain_rate_get_velocity_spatial_gradient,
				"get_velocity_spatial_gradient()\n"
				"  Return the spatial gradients of velocity tensor :math:`\\boldsymbol L` (in units of :math:`second^{-1}`) in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol L = \\begin{bmatrix} L_{\\theta\\theta} & L_{\\theta\\phi} \\\\ L_{\\phi\\theta} & L_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :returns: the tuple of :math:`(L_{\\theta\\theta}, L_{\\theta\\phi}, L_{\\phi\\theta}, L_{\\phi\\phi})`\n"
				"  :rtype: tuple (float, float, float, float)\n"
				"\n"
				"  .. note:: :math:`\\theta` is **co**-latitude and hence increases from North to South (and :math:`\\phi` increases from West to East, as expected).\n")
		// Comparisons...
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<StrainRate> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::DeformationStrainRate>();


	//
	// Strain - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::DeformationStrain>(
					"Strain",
					"\n"
					"The strain at a particular location (parcel of crust) is tracked over time and represents the accumulated deformation undergone by that parcel of crust.\n"
					"\n"
					"The strain is represented internally by the deformation *gradient* :math:`\\boldsymbol F`, which is a tensor that transforms a direction :math:`\\hat{\\boldsymbol N}` "
					"in the initial configuration (eg, at a time before deformation began) to a direction :math:`\\hat{\\boldsymbol n}` in the current configuration "
					"(eg, at a time during or after deformation), and stretches it by a factor of :math:`\\Lambda`:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\hat{\\boldsymbol n} \\Lambda = \\boldsymbol F \\cdot \\hat{\\boldsymbol N}\n"
					"\n"
					".. note:: :math:`\\hat{\\boldsymbol N}` is a 2D unit vector direction in the local South-East coordinate system at a location (:math:`\\Theta, \\Phi`) that a point occupies "
					"at the *initial* time. :math:`\\hat{\\boldsymbol n}` is a 2D unit vector direction in the local South-East coordinate system at a *new* location (:math:`\\theta, \\phi`) that "
					"the same point now occupies at the *current* time (eg, it has moved as its tectonic plate rotates/deforms). For example, if an initial South direction transforms "
					" to an Eastwards direction then :math:`\\hat{\\boldsymbol N} = (1, 0)` and :math:`\\hat{\\boldsymbol n} = (0, 1)`.\n"
					"\n"
					"The *Lagrangian* deformation tensor :math:`\\boldsymbol C` is defined in terms of the deformation gradient tensor :math:`\\boldsymbol F`:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\boldsymbol C = \\boldsymbol{F}^T \\cdot \\boldsymbol F\n"
					"\n"
					"...and determines the stretch factor :math:`\\Lambda_{(\\hat{\\boldsymbol N})}` given a direction :math:`\\hat{\\boldsymbol N}` in the *initial* configuration:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\Lambda_{(\\hat{\\boldsymbol N})} = \\sqrt{\\hat{\\boldsymbol N} \\cdot \\boldsymbol C \\cdot \\hat{\\boldsymbol N}}\n"
					"\n"
					"The *Eulerian* deformation tensor :math:`\\boldsymbol c` is also defined in terms of the deformation gradient tensor :math:`\\boldsymbol F`:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\boldsymbol c = (\\boldsymbol{F}^{-1})^T \\cdot \\boldsymbol{F}^{-1}\n"
					"\n"
					"...and determines the stretch factor :math:`\\Lambda_{(\\hat{\\boldsymbol n})}` given a direction :math:`\\hat{\\boldsymbol n}` in the *current* configuration:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\frac{1}{\\Lambda_{(\\hat{\\boldsymbol n})}} = \\sqrt{\\hat{\\boldsymbol n} \\cdot \\boldsymbol c \\cdot \\hat{\\boldsymbol n}}\n"
					"\n"
					".. note:: These stretch factors (:math:`\\Lambda_{(\\hat{\\boldsymbol N})}` and :math:`\\Lambda_{(\\hat{\\boldsymbol n})}`) are stretches along "
					"the surface direction, not the depth direction (as is the case with crustal thinning). Although surface stretch is used to determine crustal thinning "
					"(obtained by passing ``pygplates.ScalarType.gpml_crustal_stretching_factor`` to :meth:`ReconstructedGeometryTimeSpan.get_scalar_values` returned by "
					":meth:`TopologicalModel.reconstruct_geometry`).\n"
					"\n"
					"The *normal* strain is the change in length per unit *initial* length in a given direction, and is the stretch factor minus one. "
					"For a direction :math:`\\hat{\\boldsymbol N}` in the *initial* configuration, the normal strain is:\n"
					"\n"
					".. math::\n"
					"\n"
					"   e_{(\\hat{\\boldsymbol N})} &= \\Lambda_{(\\hat{\\boldsymbol N})} - 1\\\\\n"
					"                               &= \\sqrt{\\hat{\\boldsymbol N} \\cdot \\boldsymbol C \\cdot \\hat{\\boldsymbol N}} - 1\n"
					"\n"
					"And for a direction :math:`\\hat{\\boldsymbol n}` in the *current* configuration, the normal strain is:\n"
					"\n"
					".. math::\n"
					"\n"
					"   e_{(\\hat{\\boldsymbol n})} &= \\Lambda_{(\\hat{\\boldsymbol n})} - 1\\\\\n"
					"                               &= \\frac{1}{\\sqrt{\\hat{\\boldsymbol n} \\cdot \\boldsymbol c \\cdot \\hat{\\boldsymbol n}}} - 1\n"
					"\n"
					"The *Lagrangian* finite strain tensor :math:`\\boldsymbol E` is defined in terms of the *Lagrangian* deformation tensor :math:`\\boldsymbol C`:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\boldsymbol E = \\frac{1}{2} (\\boldsymbol C - \\boldsymbol I)\n"
					"\n"
					"...and it's called a *strain tensor* because for very small strains the normal strain :math:`e_{(\\hat{\\boldsymbol N})}` can be approximated as:\n"
					"\n"
					".. math::\n"
					"\n"
					"   e_{(\\hat{\\boldsymbol N})} \\simeq \\hat{\\boldsymbol N} \\cdot \\boldsymbol E \\cdot \\hat{\\boldsymbol N}\n"
					"\n"
					"The *Eulerian* finite strain tensor :math:`\\boldsymbol e` is defined in terms of the *Eulerian* deformation tensor :math:`\\boldsymbol c`:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\boldsymbol e = \\frac{1}{2} (\\boldsymbol I - \\boldsymbol c)\n"
					"\n"
					"...and for very small strains the normal strain :math:`e_{(\\hat{\\boldsymbol n})}` can be approximated as:\n"
					"\n"
					".. math::\n"
					"\n"
					"   e_{(\\hat{\\boldsymbol n})} \\simeq \\frac{\\hat{\\boldsymbol n} \\cdot \\boldsymbol e \\cdot \\hat{\\boldsymbol n}}"
					"{\\hat{\\boldsymbol n} \\cdot \\boldsymbol c \\cdot \\hat{\\boldsymbol n}}\n"
					"\n"
					"The *angle* :math:`\\alpha` between two directions in the *current* configuration that were originally in the directions "
					":math:`\\hat{\\boldsymbol N}_1` and :math:`\\hat{\\boldsymbol N}_2` in the *initial* configuration is given by:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\cos{\\alpha} = \\frac{\\hat{\\boldsymbol N}_1 \\cdot \\boldsymbol C \\cdot \\hat{\\boldsymbol N}_2}"
					"{\\Lambda_{(\\hat{\\boldsymbol N}_1)} \\Lambda_{(\\hat{\\boldsymbol N}_2)}}\n"
					"\n"
					"...and the *angle* :math:`A` between two directions in the *initial* configuration that are currently in the directions "
					":math:`\\hat{\\boldsymbol n}_1` and :math:`\\hat{\\boldsymbol n}_2` in the *current* configuration is given by:\n"
					"\n"
					".. math::\n"
					"\n"
					"   \\cos{A} = \\Lambda_{(\\hat{\\boldsymbol n}_1)} \\Lambda_{(\\hat{\\boldsymbol n}_2)} "
					"(\\hat{\\boldsymbol n}_1 \\cdot \\boldsymbol c \\cdot \\hat{\\boldsymbol n}_2)\n"
					"\n"
					"References:\n"
					"\n"
					"- Malvern, L. E. (1969). `Introduction to the mechanics of a continuous medium. <http://books.google.com/books?id=IIMpAQAAMAAJ>`_ Prentice-Hall.\n"
					"- Mase, G.T., Smelser, R.E., & Mase, G.E. (2010). `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ (3rd ed.). CRC Press.\n"
					"\n"
					"Strains are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
					"\n"
					"Convenience class static data is available for the identity strain:\n"
					"\n"
					"* ``pygplates.Strain.identity``\n"
					"\n"
					"A *Strain* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.46\n",
					bp::init<>(
							// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
							"__init__(...)\n"
							"A *Strain* object can be constructed in more than one way...\n"
							"\n"
							// Specific overload signature...
							"__init__()\n"
							"  Construct an identity strain (no deformation).\n"
							"\n"
							"  ::\n"
							"\n"
							"    identity_strain = pygplates.Strain()\n"
							"\n"
							"  .. note:: Alternatively you can use ``identity_strain = pygplates.Strain.identity``.\n"))
		.def(bp::init<double,double,double,double>(
				(bp::arg("deformation_gradient_theta_theta"), bp::arg("deformation_gradient_theta_phi"), bp::arg("deformation_gradient_phi_theta"), bp::arg("deformation_gradient_phi_phi")),
				// Specific overload signature...
				"__init__(deformation_gradient_theta_theta,, deformation_gradient_theta_phi, deformation_gradient_phi_theta, deformation_gradient_phi_phi)\n"
				"  Create from the deformation gradient :math:`\\boldsymbol F` in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol F = \\begin{bmatrix} F_{\\theta\\theta} & F_{\\theta\\phi} \\\\ F_{\\phi\\theta} & F_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :param deformation_gradient_theta_theta: :math:`F_{\\theta\\theta}`\n"
				"  :type deformation_gradient_theta_theta: float\n"
				"  :param deformation_gradient_theta_phi: :math:`F_{\\theta\\phi}`\n"
				"  :type deformation_gradient_theta_phi: float\n"
				"  :param deformation_gradient_phi_theta: :math:`F_{\\phi\\theta}`\n"
				"  :type deformation_gradient_phi_theta: float\n"
				"  :param deformation_gradient_phi_phi: :math:`F_{\\phi\\phi}`\n"
				"  :type deformation_gradient_phi_phi: float\n"
				"\n"
				"  .. seealso:: :meth:`get_deformation_gradient`\n"
				"\n"
				"  .. note:: Typically you wouldn't need this since you can start with no deformation (``pygplates.Strain.identity``) at the initial time "
				"and use :meth:`accumulate` to incrementally update strain over time using your own calculations of :class:`strain rate <StrainRate>`.\n"))
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesAppLogic::DeformationStrain>>())
		// Static property 'pygplates.Strain.identity'...
		.def_readonly("identity", GPlatesApi::identity_strain)
		.def("get_dilatation",
				&GPlatesAppLogic::DeformationStrain::get_strain_dilatation,
				"get_dilatation()\n"
				"  Return the change in crustal area with respect to the original area.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The dilatation is the increase (if positive) or decrease (if negative) of crustal area with respect to the original area in the initial configuration "
				"(eg, at a time before deformation began).\n"
				"\n"
				"  The dilatation is defined as the determinant of the :meth:`deformation gradient tensor <get_deformation_gradient>` :math:`\\boldsymbol F` minus one. "
				"So if we define :math:`A` as the original area of a parcel of crust in the initial configuration, then the dilatation is given by:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\Delta A}{A} = \\det \\boldsymbol F - 1\n"
				"\n"
				"  .. seealso::\n"
				"\n"
				"     Chapter 4.11 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ for a derivation of the change in volume. "
				"We ignore the radial dimension, hence volume becomes area.\n"
				"\n"
				"  .. note:: The dilatation is *invariant* with respect to the local coordinate axes (South and East) since :math:`\\det \\boldsymbol F` is "
				"the *third invariant* of :math:`\\boldsymbol F` (see Chapter 3.6 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_).\n")
		.def("get_principal_strain",
				&GPlatesApi::strain_get_principal_strain,
				(bp::arg("principal_angle_type") = GPlatesApi::PrincipalAngleType::MAJOR_SOUTH),
				"get_principal_strain([principal_angle_type=PrincipalAngleType.major_south])\n"
				"  Return the maximum and minimum strains (along principal axes), and the angle of the major principal axis.\n"
				"\n"
				"  :param principal_angle_type: how the angle of the major principal axis is defined relative to the local coordinate system "
				"(defaults to *PrincipalAngleType.major_south*)\n"
				"  :type principal_angle_type: PrincipalAngleType.major_south, PrincipalAngleType.major_east or PrincipalAngleType.major_azimuth\n"
				"  :returns: the tuple of maximum strain, minimum strain and major axis angle :math:`(e_{(1)}, e_{(2)}, \\alpha)`\n"
				"  :rtype: tuple (float, float, float)\n"
				"\n"
				"  *principal_angle_type* supports the following enumeration types:\n"
				"\n"
				"  ================================= ==============\n"
				"  Value                              Description\n"
				"  ================================= ==============\n"
				"  PrincipalAngleType.major_south    The major principal axis points South when the angle is zero. "
				"The angle ranges from :math:`-\\pi` to :math:`\\pi` radians **anti**-clockwise (observed from above the globe).\n"
				"  PrincipalAngleType.major_east     The major principal axis points East when the angle is zero. "
				"The angle ranges from :math:`-\\pi` to :math:`\\pi` radians **anti**-clockwise (observed from above the globe). "
				"This is equivalent to *MajorAngle* in the GPlates deformation export.\n"
				"  PrincipalAngleType.major_azimuth  The major principal axis points North when the angle is zero. "
				"The angle ranges from :math:`0` to :math:`2\\pi` radians **clockwise** (observed from above the globe). "
				"This is equivalent to *MajorAzimuth* in the GPlates deformation export.\n"
				"  ================================= ==============\n"
				"\n"
				"  .. note:: Regardless of the value of *principal_angle_type*, the direction of the *minimum* principal axis is always an **anti-clockwise** rotation "
				"of :math:`\\frac{\\pi}{2}` radians (90 degrees) of the *major* principal axis (observed from above the globe).\n"
				"\n"
				"  The principal strains are the maximum and minimum strains that occur along the principal axes (where shear strain is zero). "
				"The principal axes are the coordinate axes rotated anti-clockwise (when observed from above the globe) by an angle :math:`\\alpha` "
				"which is defined in terms of the *Eulerian* deformation tensor :math:`\\boldsymbol c` (see :class:`Strain`):\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\tan{2 \\alpha} = \\frac{2 c_{\\theta\\phi}}{c_{\\theta\\theta} - c_{\\phi\\phi}}\n"
				"\n"
				"  Then the two perpendicular principal strain directions are (in the local South-East coordinate system):\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\hat{\\boldsymbol n}_{(1)} &= (\\cos \\alpha, \\sin \\alpha)\\\\\n"
				"     \\hat{\\boldsymbol n}_{(2)} &= (-\\sin \\alpha, \\cos \\alpha)\n"
				"\n"
				"  ...and the two principal strains are determined by (see :class:`Strain`):\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     e_{(1)} &= \\Lambda_{(1)} - 1 &= \\frac{1}{\\sqrt{\\hat{\\boldsymbol n}_{(1)} \\cdot \\boldsymbol c \\cdot \\hat{\\boldsymbol n}_{(1)}}} - 1\\\\\n"
				"     e_{(2)} &= \\Lambda_{(2)} - 1 &= \\frac{1}{\\sqrt{\\hat{\\boldsymbol n}_{(2)} \\cdot \\boldsymbol c \\cdot \\hat{\\boldsymbol n}_{(2)}}} - 1\n"
				"\n"
				"  .. note:: This function returns principal *strains*. To get the principal *stretches* simply add one since :math:`\\Lambda_{(i)} = 1 + e_{(i)}` (see :class:`Strain`).\n"
				"\n"
				"  To get the principal *stretches* (ie, *strains* plus one) with the angle (in degrees) of the major principal axis clockwise relative to North (ie, azimuth):\n"
				"  ::\n"
				"\n"
				"    import math\n"
				"    ...\n"
				"    max_strain, min_strain, major_azimuth_radians = strain.get_principal_strain(\n"
				"        principal_angle_type=pygplates.PrincipalAngleType.major_azimuth)\n"
				"\n"
				"    max_stretch = 1 + max_strain\n"
				"    min_stretch = 1 + min_strain\n"
				"    major_azimuth_degrees = math.degrees(major_azimuth_radians)\n")
		.def("get_deformation_gradient",
				&GPlatesApi::strain_get_deformation_gradient,
				"get_deformation_gradient()\n"
				"  Return the deformation gradient tensor :math:`\\boldsymbol F` in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol F = \\begin{bmatrix} F_{\\theta\\theta} & F_{\\theta\\phi} \\\\ F_{\\phi\\theta} & F_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :returns: the tuple of :math:`(F_{\\theta\\theta}, F_{\\theta\\phi}, F_{\\phi\\theta}, F_{\\phi\\phi})`\n"
				"  :rtype: tuple (float, float, float, float)\n"
				"\n"
				"  .. note:: :math:`\\theta` is **co**-latitude and hence increases from North to South (and :math:`\\phi` increases from West to East, as expected).\n")
		.def("accumulate",
				&GPlatesAppLogic::accumulate_strain,
				(bp::arg("previous_strain"), bp::arg("previous_strain_rate"), bp::arg("current_strain_rate"), bp::arg("time_increment")),
				"accumulate(previous_strain, previous_strain_rate, current_strain_rate, time_increment)\n"
				"  Accumulate previous strain using both previous and current strain rates (in units of :math:`second^{-1}`) "
				"over a time increment (in units of :math:`second`).\n"
				"\n"
				"  :param previous_strain: the *previous* strain\n"
				"  :type previous_strain: Strain\n"
				"  :param previous_strain_rate: the *previous* strain rate\n"
				"  :type previous_strain_rate: StrainRate\n"
				"  :param current_strain_rate: the *current* strain rate\n"
				"  :type current_strain_rate: StrainRate\n"
				"  :param time_increment: the time increment to accumulate strain over (in units of :math:`second^{-1}`)\n"
				"  :type time_increment: float\n"
				"  :returns: the *current* strain (accumulated from *previous* strain)\n"
				"  :rtype: Strain\n"
				"\n"
				"  To accumulate strain from an initial undeformed state at 100Ma to its final deformed strain at present day:\n"
				"  ::\n"
				"\n"
				"    time_increment_1myr_in_seconds = 1e6 * 365 * 24 * 60 * 60\n"
				"    previous_strain = pygplates.Strain.identity\n"
				"    previous_strain_rate = pygplates.StrainRate.zero\n"
				"\n"
				"    for time in range(100, -1, -1):\n"
				"        current_strain_rate = pygplates.StrainRate(...)\n"
				"        current_strain = pygplates.Strain.accumulate(previous_strain,\n"
				"            previous_strain_rate, current_strain_rate, time_increment_1myr_in_seconds)\n"
				"\n"
				"        previous_strain = current_strain\n"
				"        previous_strain_rate = current_strain_rate\n"
				"\n"
				"  Strain is accumulated by integrating the ordinary differential equation defining the rate of change of the deformation gradient "
				":math:`\\boldsymbol F` in terms of the :meth:`spatial gradients of velocity <StrainRate.get_velocity_spatial_gradient>` :math:`\\boldsymbol L`:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\dot{\\boldsymbol F} = \\boldsymbol L \\cdot \\boldsymbol F\n"
				"\n"
				"  ...which is approximated using the central differencing scheme over a time increment :math:`\\Delta t`:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\boldsymbol{F}_{t + \\Delta t} - \\boldsymbol{F}}{\\Delta t} = "
				"\\frac{\\boldsymbol{L}_{t + \\Delta t} \\boldsymbol{F}_{t + \\Delta t} + \\boldsymbol{L}_{t} \\boldsymbol{F}_{t}}{2}\n"
				"\n"
				"  ...which rearranges to become (in matrix form, with identity matrix :math:`\\boldsymbol I`):\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol{F}_{t + \\Delta t} = (\\boldsymbol I - \\boldsymbol{L}_{t + \\Delta t} \\frac{\\Delta t}{2})^{-1} "
				"(\\boldsymbol I + \\boldsymbol{L}_{t} \\frac{\\Delta t}{2}) \\boldsymbol{F}_{t}\n"
				"\n"
				"  ...where :math:`\\Delta t` is *time_increment*, "
				":math:`\\boldsymbol{L}_{t}` is *previous_strain_rate*, :math:`\\boldsymbol{L}_{t + \\Delta t}` is *current_strain_rate*, "
				":math:`\\boldsymbol{F}_{t}` is *previous_strain* and :math:`\\boldsymbol{F}_{t + \\Delta t}` is returned by this function.\n")
		.staticmethod("accumulate")
		// Comparisons...
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<Strain> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::DeformationStrain>();
}
