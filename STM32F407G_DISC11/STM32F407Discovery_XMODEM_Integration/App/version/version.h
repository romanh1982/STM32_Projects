/**************************************************************************//**
  * @file   version.h
  * @brief  Current system version information.
 *****************************************************************************/

#ifndef VERSION_H
#define VERSION_H

#define MAJOR_VERSION 0
#define MINOR_VERSION 3
#define MINOR_PATCH_VERSION 1
#define BUILD_DATE              __DATE__
#define BUILD_TIME              __TIME__

#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)

#ifdef RELEASE_BUILD
#define SYSTEM_VERSION_STR QUOTE(MAJOR_VERSION.MINOR_VERSION.MINOR_PATCH_VERSION)
#else
#define SYSTEM_VERSION_STR QUOTE(MAJOR_VERSION.MINOR_VERSION.MINOR_PATCH_VERSION (Build: BUILD_DATE - BUILD_TIME))
#endif

#endif
