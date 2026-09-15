#####################################
# GPlates/pyGPlates release targets #
#####################################

#
# The release each develop line is heading towards.
#
# THIS IS THE ONLY HAND-EDITED PART OF THE VERSION. The development number that turns it into a
# full version (eg, '2.6.0' -> '2.6.0-47', '1.1.0' -> '1.1.0.dev46') is counted from git by
# 'VersionFromGit.cmake' - see the comments there, and 'Version.cmake' for the version grammars.
#
# There are exactly two times to edit this file:
#
# 1. On the development branch, the moment a release series branch cut from it gets its first
#    tag - a candidate ('PyGPlates-1.1.0rc1') or the release itself. The pyGPlates target then
#    becomes '1.2.0'. The 1.1.0 line now lives on the series branch, and the development
#    branch's count restarts from the branch point, so left on '1.1.0' it would re-issue
#    versions it has already used - and, once 1.1.0 is released, ones sorting *below* it. The
#    resolver refuses both, rather than letting either reach a package.
#
#    Note: What that next target should be is a decision about the next *release*, not about
#          any one commit. A release that adds API (a new function or class) should be a new
#          minor version ('1.2.0') rather than a patch ('1.1.1'), because that is what lets a
#          user - or an internal build - test for the new functionality. Individual commits
#          need nothing: adding a function to a develop branch simply advances the development
#          number under whatever target is already set.
#
#          So set the next minor version straight after a release, and reduce it to a patch
#          version only if that release turns out to contain no new API. Waiting until the API
#          actually changes only risks nobody remembering to do it.
#
# 2. On a release series branch ('release/pygplates-<major>.<minor>' etc), to name the release being
#    prepared - eg, set the target to '1.1.0rc1' on cutting the branch, to '1.1.0rc2' if a
#    second candidate is needed, and to '1.1.0' for the release itself. Development commits on
#    the release branch then carry '1.1.0rc1.dev3' and so on.
#
#    Nothing detects the branch by name: a target is a decision ("there will be another release
#    candidate"), and branch-name detection is unreliable anyway under detached HEAD, pull
#    request merge refs, worktrees and forks.
#
# A target must NOT carry a development suffix ('2.6.0-8', '1.1.0.dev10') - that part is what
# gets counted. The resolver rejects one that does.
#
# The resolver also checks the target against the nearest release, and aborts the configure if it
# does not sort above it or if it skips a version - so a target left behind, set backwards, or
# mistyped one release too far ahead is a loud failure rather than a package nobody can install
# over. 'gplates_check_release_target' in 'VersionFromGit.cmake' has the rules and the reasoning;
# 'VersionFromGitTest.cmake' has them as tests.
#

# The GPlates release target - a restricted Semantic Version 'X.Y.Z' or 'X.Y.Z-{alpha|beta|rc}.N'.
set(GPLATES_RELEASE_VERSION 2.6.0)

# The pyGPlates release target - a restricted PEP 440 version 'X.Y.Z[{a|b|rc}N][.postN]'.
set(PYGPLATES_RELEASE_VERSION 1.1.0)
