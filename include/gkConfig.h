#ifndef _GK_CONFIG_H_
#define _GK_CONFIG_H_

#ifdef _WIN32
#define GK_PLATFORM_WIN
#elif defined(LINUX)
#define GK_PLATFORM_LINUX
#elif defined(TIZEN)
#define GK_PLATFORM_TIZEN
#elif defined(ANDROID)
#define GK_PLATFORM_ANDROID
#endif


#ifdef GK_PLATFORM_WIN
/*
	Choose XAudio2 or OpenAL
*/
#if !defined(GK_USE_XAUDIO2) && !defined(GK_USE_OPENAL)
#define GK_USE_XAUDIO2
#endif

/*
	Choose audio decoders
*/
#define GK_USE_MPG123
#define GK_USE_OGGVORBIS

/*
	Choose image decoders
*/
#define GK_USE_LIBJPEG
#define GK_USE_LIBPNG

#define GK_USE_FONTS
#define GK_USE_JOYSTICK

#elif defined(GK_PLATFORM_LINUX)

#define GK_USE_OPENAL
#define GK_USE_MPG123
#define GK_USE_OGGVORBIS

#define GK_USE_LIBJPEG
#define GK_USE_LIBPNG

#define GK_USE_FONTS
#define GK_USE_JOYSTICK

#elif defined(GK_PLATFORM_TIZEN)

#define GK_USE_NOAUDIO
#define GK_SHOW_PLATFORM_ERRORS
#define GK_PLATFORM_TEST

#elif defined(GK_PLATFORM_ANDROID)

#define GK_USE_NOAUDIO
#define GK_SHOW_PLATFORM_ERRORS
#define GK_PLATFORM_TEST

#define GK_USE_LIBJPEG
#define GK_USE_LIBPNG

#else
/* Unknown platform */

#define GK_USE_NOAUDIO
#define GK_SHOW_PLATFORM_ERRORS
#define GK_PLATFORM_TEST

#endif

#ifndef stricmp
#ifdef GK_PLATFORM_WIN
#define stricmp _stricmp
#else
#define stricmp strcasecmp
#endif
#endif

#endif