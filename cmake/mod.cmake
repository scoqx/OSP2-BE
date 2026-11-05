set(MOD_NAME OSP2BE)

# The version is written in one place: the last entry of changelog_be.txt, such as
# "06.11.2026 be v1.10c-beta". Everything else is built from it.
set(MOD_CHANGELOG ${CMAKE_SOURCE_DIR}/changelog_be.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${MOD_CHANGELOG})
file(STRINGS ${MOD_CHANGELOG} MOD_VERSION_LINES REGEX "^[0-9][0-9]\\.[0-9][0-9]\\.[0-9][0-9][0-9][0-9] be v[0-9]")
list(GET MOD_VERSION_LINES -1 MOD_VERSION_LINE)
string(REGEX REPLACE "^.* be v" "" MOD_VERSION_FULL "${MOD_VERSION_LINE}")
string(STRIP "${MOD_VERSION_FULL}" MOD_VERSION_FULL)
string(REGEX MATCH "^[0-9]+\\.[0-9]+" MOD_VERSION "${MOD_VERSION_FULL}")

set(BASEGAME osp)

set(OSP_VERSION "be-${MOD_VERSION_FULL}")
set(OSP_CLIENT "1008_OSP2_${OSP_VERSION}")
set(OSP_PAKNAME "zz-osp-pak8be.pk3")

set(CGAME_MODULE cgame)
set(UI_MODULE ui)

set(ASSETS_LOCATION ${CMAKE_SOURCE_DIR}/assets)
