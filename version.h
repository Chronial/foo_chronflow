#pragma once
#define COMPONENT_NAME_LABEL "Chronflow Mod"
#define COMPONENT_NAME "foo_chronflow_mod"
#define COMPONENT_YEAR "2023"

#define COMPONENT_VERSION_MAJOR 0

#define COMPONENT_VERSION_MINOR 5
#define COMPONENT_VERSION_PATCH 2
#define COMPONENT_VERSION_SUB_PATCH 12

#define MAKE_STRING(text) #text
#define MAKE_COMPONENT_VERSION(major, minor, patch, subpatch) \
  MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch) ".mod." MAKE_STRING(subpatch)
#define MAKE_DLL_VERSION(major,minor,patch,subpatch) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch) "." MAKE_STRING(subpatch)

//"0.1.2"
#ifdef BETA_VER
#define FOO_CHRONFLOW_VERSION \
  MAKE_COMPONENT_VERSION(COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, \
                         COMPONENT_VERSION_PATCH, COMPONENT_VERSION_SUB_PATCH) \
  ".beta"
#else
#define FOO_CHRONFLOW_VERSION \
  MAKE_COMPONENT_VERSION(COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, \
                         COMPONENT_VERSION_PATCH, COMPONENT_VERSION_SUB_PATCH)
#endif

//0.1.2.3 & "0.1.2.3"
#define DLL_VERSION_NUMERIC COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, COMPONENT_VERSION_SUB_PATCH
#define DLL_VERSION_STRING MAKE_DLL_VERSION(COMPONENT_VERSION_MAJOR,COMPONENT_VERSION_MINOR,COMPONENT_VERSION_PATCH,COMPONENT_VERSION_SUB_PATCH)

