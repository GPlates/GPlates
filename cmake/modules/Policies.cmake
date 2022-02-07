#
# CMake policies give the choice of OLD (backwards compatible) or NEW (non-backwards compatible) behaviour
# when a version of CMake newer than specified in 'cmake_minimum_required()' is used.
# CMake states that ultimately all policies should specify NEW with OLD being a temporary solution.
#
# Each 'cmake_policy' command is surrounded by if/endif since the runtime version of CMake may not support them.
# For example, when specifying CMake 3.x policies but the runtime CMake is 2.8.8.
#
