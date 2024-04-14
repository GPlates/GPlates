#
# Version variables.
#

#
# Whether to build (and install, package, etc) GPlates or pyGPlates.
#
# We no longer support building *both* since, for example, CPack's Debian packages don't support different versions for different components
# (eg, we used to have a 'gplates' component and a 'pygplates' component using the COMPONENT variable of the 'install()' command).
# Now we build *either* GPlates or pyGPlates. If you need to build both then create two separate out-of-place CMake builds
# (out-of-place means the binary artifacts are created in a directory separate from the source code).
#
# NOTE: THIS IS CURRENTLY THE MAIN BRANCH (SO 'GPLATES_BUILD_GPLATES' DEFAULTS TO 'TRUE').
#       YOU SHOULD ONLY BUILD 'gplates'. YOU SHOULDN'T BUILD 'pygplates' UNTIL THE PYGPLATES BRANCH HAS BEEN FULLY MERGED TO MAIN
#       (WHICH CAN ONLY HAPPEN ONCE WE'VE COMPLETELY UPDATED THE INTERNAL MODEL IN THE PYGPLATES BRANCH).
#
option(GPLATES_BUILD_GPLATES "True to build GPlates (false to build pyGPlates)." true)


#
# A note about pre-release version suffixes (such as GPLATES_VERSION_PRERELEASE_SUFFIX and PYGPLATES_VERSION_PRERELEASE_SUFFIX)...
#
# This should be:
# - empty if not a pre-release,
# - a number for development pre-releases (eg, 1, 2, etc),
# - 'alpha' followed by '.' followed by a number for alpha pre-releases (eg, alpha.1, alpha.2, etc),
# - 'beta' followed by '.' followed by a number for beta pre-releases (eg, beta.1, beta.2, etc),
# - 'rc' followed by '.' followed by a number for pre-release candidates (eg, rc.1, rc.2, etc).
#
# The reason for the above rules is they support the correct version ordering precedence for both Semantic Versioning and Debian versioning
# (even though Semantic and Debian versioning have slightly different precedence rules).
#
# Semantic version precedence separates identifiers between dots and compares each identifier.
# According to https://semver.org/spec/v2.0.0.html ...
# - digit-only identifiers are compared numerically,
# - identifiers with letters are compared lexically in ASCII order,
# - numeric identifiers have lower precedence than non-numeric identifiers.
# ...and so '1' < 'beta.1' because '1' < 'beta', and 'beta.1' < 'beta.2' because 'beta' == 'beta' but '1' < '2'.
#
# Debian version precedence separates identifiers into alternating non-digit and digit identifiers.
# According to https://www.debian.org/doc/debian-policy/ch-controlfields.html#version ...
# - find initial part consisting only of non-digits and compare lexically in ASCII order (modified so letters sort earlier than non-letters, and '~' earliest of all),
# - find next part consisting only of digits and compare numerically,
# - repeat the above two steps until a difference is found.
# ...and so '1' < 'beta.1' because '' < 'beta.', and 'beta.1' < 'beta.2' because 'beta.' == 'beta.' but '1' < '2'.
#
# For example, for a major.minor.patch verson of 2.3.0:
# For Semantic Versioning: 2.3.0-1 < 2.3.0-alpha.1 < 2.3.0-beta.1 < 2.3.0-rc.1 < 2.3.0.
# For Debian versioning:   2.3.0~1 < 2.3.0~alpha.1 < 2.3.0~beta.1 < 2.3.0~rc.1 < 2.3.0.
#

#
# Ensure pre-release satisfies the above conditions.
#
function(check_prerelease_suffix prerelease_suffix)
	if (prerelease_suffix)
		if (NOT ${prerelease_suffix} MATCHES [[^((alpha|beta|rc)\.)?[0-9]+$]])
			message(FATAL_ERROR "${prerelease_suffix} should be <N>, alpha.<N>, beta.<N> or rc.<N>")
		endif()
	endif()
endfunction()

#
# Return the pre-release version (and human-readable user pre-release version), as well as a human-readable pre-release version *suffix*.
#
function(set_prerelease_version version version_prerelease_suffix version_prerelease version_prerelease_user version_prerelease_suffix_user)
	# For a pre-release append the pre-release version (using a hyphen for pre-releases as dictated by Semantic Versioning).
	if (version_prerelease_suffix)
		set(_version_prerelease ${version}-${version_prerelease_suffix})
		# A human-readable pre-release version (unset/empty if not a pre-release).
		#
		# If a development release (ie, if pre-release version is just a number) then insert 'dev' into the version *name* to make it more obvious to users.
		# Note: We don't insert 'dev' into the version itself because that would give it a higher version ordering precedence than 'alpha' and 'beta' (since a < b < d).
		#       Keeping only the development number in the actual version works because digits have lower precedence than non-digits (according to Semantic and Debian versioning).
		if (version_prerelease_suffix MATCHES "^[0-9]+$")
			set(_version_prerelease_suffix_user dev${version_prerelease_suffix})
		else()
			set(_version_prerelease_suffix_user ${version_prerelease_suffix})
		endif()
		set(_version_prerelease_user ${version}-${_version_prerelease_suffix_user})
	else()
		set(_version_prerelease ${version})
		set(_version_prerelease_user ${version})
		set(_version_prerelease_suffix_user "")
	endif()

	# Set caller's variables.
	set(${version_prerelease} ${_version_prerelease} PARENT_SCOPE)
	set(${version_prerelease_user} ${_version_prerelease_user} PARENT_SCOPE)
	set(${version_prerelease_suffix_user} ${_version_prerelease_suffix_user} PARENT_SCOPE)
endfunction()


###########
# GPlates #
###########

#
# The GPlates version.
#
set(GPLATES_VERSION_MAJOR 2)
set(GPLATES_VERSION_MINOR 5)
set(GPLATES_VERSION_PATCH 0)

# The pyGPlates version without the pre-release suffix
# (matches the version generated by 'project()' which does not support pre-release suffixes).
set(GPLATES_VERSION ${GPLATES_VERSION_MAJOR}.${GPLATES_VERSION_MINOR}.${GPLATES_VERSION_PATCH})

#
# GPlates pre-release version suffix (in Semantic Versioning format).
#
# See note about pre-release version suffixes above.
#
set(GPLATES_VERSION_PRERELEASE_SUFFIX "")
# Ensure pre-release contains only dot-separated alphanumeric identifiers.
check_prerelease_suffix("${GPLATES_VERSION_PRERELEASE_SUFFIX}")

#
# Full GPlates version including pre-release suffix:
#
# GPLATES_VERSION_PRERELEASE             - Version dictated by Semantic Versioning (used when need correct version precendence, eg, '1' < 'alpha.1').
# GPLATES_VERSION_PRERELEASE_USER        - Human-readable version that inserts 'dev' for development pre-releases (useful for any string the user might see).
#                                          Does not maintain correct version precedence (eg, 'dev1' > 'alpha.1' whereas '1' < 'alpha.1').
#
# Also a human-readable version of the pre-release version suffix:
#
# GPLATES_VERSION_PRERELEASE_SUFFIX_USER - Human-readable pre-release suffix that inserts 'dev' for development pre-releases.
#
set_prerelease_version(
		"${GPLATES_VERSION}" "${GPLATES_VERSION_PRERELEASE_SUFFIX}"
		GPLATES_VERSION_PRERELEASE GPLATES_VERSION_PRERELEASE_USER GPLATES_VERSION_PRERELEASE_SUFFIX_USER)


#############
# PyGPlates #
#############
#
# The pyGPlates version should typically be updated when the API changes (eg, a new function or class)
# so users can then test for new functionality (even for internal releases).
#

#
# The pyGPlates version (MAJOR.MINOR.PATCH).
#
set(PYGPLATES_VERSION_MAJOR 0)
set(PYGPLATES_VERSION_MINOR 39)
set(PYGPLATES_VERSION_PATCH 0)

# The pyGPlates version without the pre-release suffix
# (matches the version generated by 'project()' which does not support pre-release suffixes).
set(PYGPLATES_VERSION ${PYGPLATES_VERSION_MAJOR}.${PYGPLATES_VERSION_MINOR}.${PYGPLATES_VERSION_PATCH})

#
# PyGPlates pre-release version suffix (in Semantic Versioning format).
#
# See note about pre-release version suffixes above.
#
set(PYGPLATES_VERSION_PRERELEASE_SUFFIX "")
# Ensure pre-release contains only dot-separated alphanumeric identifiers.
check_prerelease_suffix("${PYGPLATES_VERSION_PRERELEASE_SUFFIX}")

#
# Full pyGPlates version including pre-release suffix:
#
# PYGPLATES_VERSION_PRERELEASE             - Version dictated by Semantic Versioning (used when need correct version precendence, eg, '1' < 'alpha.1').
# PYGPLATES_VERSION_PRERELEASE_USER        - Human-readable version that inserts 'dev' for development pre-releases (useful for any string the user might see).
#                                            Does not maintain correct version precedence (eg, 'dev1' > 'alpha.1' whereas '1' < 'alpha.1').
#
# Also a human-readable version of the pre-release version suffix:
#
# PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER - Human-readable pre-release suffix that inserts 'dev' for development pre-releases.
#
set_prerelease_version(
		"${PYGPLATES_VERSION}" "${PYGPLATES_VERSION_PRERELEASE_SUFFIX}"
		PYGPLATES_VERSION_PRERELEASE PYGPLATES_VERSION_PRERELEASE_USER PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER)


#
# Get the Python PEP 440 version from the pyGPlates version and optional pre-release suffix.
#
function(set_prerelease_PEP440_version version version_prerelease_suffix version_prerelease_PEP440)
	# For a pre-release append the pre-release version in PEP 440 format.
	if (version_prerelease_suffix)
		# Note that the 'check_prerelease_suffix()' function has already guaranteed a
		# prerelease suffix match with the regular expression "^((alpha|beta|rc)\.)?[0-9]+$".
		if (version_prerelease_suffix MATCHES [[^([0-9]+)$]])
			set(_version_prerelease_PEP440_suffix ".dev${CMAKE_MATCH_1}")
		elseif (version_prerelease_suffix MATCHES [[^alpha\.([0-9]+)$]])
			set(_version_prerelease_PEP440_suffix "a${CMAKE_MATCH_1}")
		elseif (version_prerelease_suffix MATCHES [[^beta\.([0-9]+)$]])
			set(_version_prerelease_PEP440_suffix "b${CMAKE_MATCH_1}")
		elseif (version_prerelease_suffix MATCHES [[^rc\.([0-9]+)$]])
			set(_version_prerelease_PEP440_suffix "rc${CMAKE_MATCH_1}")
		else()
			message(FATAL_ERROR "${version_prerelease_suffix} should be <N>, alpha.<N>, beta.<N> or rc.<N>")
		endif()
		set(_version_prerelease_PEP440 ${version}${_version_prerelease_PEP440_suffix})
	else()
		set(_version_prerelease_PEP440 ${version})
	endif()

	# Set caller's variables.
	set(${version_prerelease_PEP440} ${_version_prerelease_PEP440} PARENT_SCOPE)
endfunction()

#
# Full Python PEP 440 pyGPlates version including PEP 440 pre-release suffix:
#
# PYGPLATES_VERSION_PRERELEASE_PEP440
#
set_prerelease_PEP440_version(
		"${PYGPLATES_VERSION}" "${PYGPLATES_VERSION_PRERELEASE_SUFFIX}"
		PYGPLATES_VERSION_PRERELEASE_PEP440)
