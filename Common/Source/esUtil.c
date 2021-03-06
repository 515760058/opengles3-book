// The MIT License (MIT)
//
// Copyright (c) 2013 Dan Ginsburg, Budirijanto Purnomo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//
// Book:      OpenGL(R) ES 3.0 Programming Guide, 2nd Edition
// Authors:   Dan Ginsburg, Budirijanto Purnomo, Dave Shreiner, Aaftab Munshi
// ISBN-10:   0-321-93388-5
// ISBN-13:   978-0-321-93388-1
// Publisher: Addison-Wesley Professional
// URLs:      http://www.opengles-book.com
//            http://my.safaribooksonline.com/book/animation-and-3d/9780133440133
//
// ESUtil.c
//
//    A utility library for OpenGL ES.  This library provides a
//    basic common framework for the example applications in the
//    OpenGL ES 3.0 Programming Guide.
//

///
//  Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "esUtil.h"
#include "esUtil_win.h"

#ifdef ANDROID
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
typedef AAsset esFile;
#else
typedef FILE esFile;
#endif

#ifdef __APPLE__
#include "FileWrapper.h"
#endif

///
//  Macros
//
#define INVERTED_BIT            (1 << 5)

///
//  Types
//
#ifndef __APPLE__
#pragma pack(push,x1)                            // Byte alignment (8-bit)
#pragma pack(1)
#endif

typedef struct
#ifdef __APPLE__
__attribute__ ( ( packed ) )
#endif
{
   unsigned char  IdSize,
            MapType,
            ImageType;
   unsigned short PaletteStart,
            PaletteSize;
   unsigned char  PaletteEntryDepth;
   unsigned short X,
            Y,
            Width,
            Height;
   unsigned char  ColorDepth,
            Descriptor;

} TGA_HEADER;

#ifndef __APPLE__
#pragma pack(pop,x1)
#endif

#ifndef __APPLE__

///
// GetContextRenderableType()
//
//    Check whether EGL_KHR_create_context extension is supported.  If so,
//    return EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT
//
EGLint GetContextRenderableType ( EGLDisplay eglDisplay )
{
#ifdef EGL_KHR_create_context
   const char *extensions = eglQueryString ( eglDisplay, EGL_EXTENSIONS );

   // check whether EGL_KHR_create_context is in the extension string
   if ( extensions != NULL && strstr( extensions, "EGL_KHR_create_context" ) )
   {
      // extension is supported
      return EGL_OPENGL_ES3_BIT_KHR;
   }
#endif
   // extension is not supported
   return EGL_OPENGL_ES2_BIT;
}
#endif

//////////////////////////////////////////////////////////////////
//
//  Public Functions
//
//

///
//  esCreateWindow()
//
//      title - name for title bar of window
//      width - width of window to create
//      height - height of window to create
//      flags  - bitwise or of window creation flags
//          ES_WINDOW_ALPHA       - specifies that the framebuffer should have alpha
//          ES_WINDOW_DEPTH       - specifies that a depth buffer should be created
//          ES_WINDOW_STENCIL     - specifies that a stencil buffer should be created
//          ES_WINDOW_MULTISAMPLE - specifies that a multi-sample buffer should be created
//   通过EGL获取并初始化display, 指定属性列表获取EGLConfig配置，利用创建surface（窗口）和context（上下文）
GLboolean ESUTIL_API esCreateWindow ( ESContext *esContext, const char *title, GLint width, GLint height, GLuint flags )
{
#ifndef __APPLE__
   EGLConfig config;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

   if ( esContext == NULL )
   {
      return GL_FALSE;
   }

#ifdef ANDROID
   // For Android, get the width/height from the window rather than what the
   // application requested.
   esContext->width = ANativeWindow_getWidth ( esContext->eglNativeWindow );
   esContext->height = ANativeWindow_getHeight ( esContext->eglNativeWindow );
#else
   esContext->width = width;
   esContext->height = height;
#endif

   if ( !WinCreate ( esContext, title ) )
   {
      return GL_FALSE;
   }
   //创建和初始化与本地EGL显示的连接，通过两个函数完成：
   esContext->eglDisplay = eglGetDisplay( esContext->eglNativeDisplay );
   if ( esContext->eglDisplay == EGL_NO_DISPLAY )//EGL不可用，不能创建连接
   {
      return GL_FALSE;
   }

   // Initialize EGL
   // 初始化EGL内部数据结构，返回主次版本号。初始化失败会返回egl_false。
   if ( !eglInitialize ( esContext->eglDisplay, &majorVersion, &minorVersion ) )
   {
      return GL_FALSE;
   }

   {
      EGLint numConfigs = 0;
      EGLint attribList[] = // 指定的属性列表，eglChooseConfig函数要返回满足此属性列表的EGLConfig配置
      {
         EGL_RED_SIZE,       5,//颜色缓冲区需要支持5位红色分量
         EGL_GREEN_SIZE,     6,//颜色缓冲区需要支持6位绿色分量
         EGL_BLUE_SIZE,      5,//颜色缓冲区需要支持5位蓝色分量
         EGL_ALPHA_SIZE,     ( flags & ES_WINDOW_ALPHA ) ? 8 : EGL_DONT_CARE, //颜色缓冲区需要支持x位阿尔法分量
         EGL_DEPTH_SIZE,     ( flags & ES_WINDOW_DEPTH ) ? 8 : EGL_DONT_CARE,//深度缓冲区位数
         EGL_STENCIL_SIZE,   ( flags & ES_WINDOW_STENCIL ) ? 8 : EGL_DONT_CARE,//模板缓冲区位数
         EGL_SAMPLE_BUFFERS, ( flags & ES_WINDOW_MULTISAMPLE ) ? 1 : 0,//可用多重采样缓冲区数量
         // if EGL_KHR_create_context extension is supported, then we will use
         // EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT in the attribute list
         // 渲染类型 通过eglQueryString ( eglDisplay, EGL_EXTENSIONS )确定是EGL_OPENGL_ES3_BIT_KHR或者EGL_OPENGL_ES2_BIT
         EGL_RENDERABLE_TYPE, GetContextRenderableType ( esContext->eglDisplay ),
         EGL_NONE
      };

      // Choose config 让EGL选择匹配的EGLconfig配置
      // attribList：指定属性列表，eglChooseConfig函数要返回满足此属性列表的EGLConfig配置
      // config：返回的配置
      // 1 : 最多只接受1个配置，存放在config中。
      // numConfigs：返回的配置数量
      if ( !eglChooseConfig ( esContext->eglDisplay, attribList, &config, 1, &numConfigs ) )
      {
         return GL_FALSE;
      }

      if ( numConfigs < 1 )
      {
         return GL_FALSE;
      }
   }


#ifdef ANDROID
   // For Android, need to get the EGL_NATIVE_VISUAL_ID and set it using ANativeWindow_setBuffersGeometry
   {
      EGLint format = 0;
      eglGetConfigAttrib ( esContext->eglDisplay, config, EGL_NATIVE_VISUAL_ID, &format );
      ANativeWindow_setBuffersGeometry ( esContext->eglNativeWindow, 0, 0, format );
   }
#endif // ANDROID

   // Create a surface
   // 查询到满足渲染要求的config之后，就可以创建窗口。
   // config 指定的配置
   // eglNativeWindow 指定原生窗口
   // null 窗口属性列表，可能为NULL
   esContext->eglSurface = eglCreateWindowSurface ( esContext->eglDisplay, config, 
                                                    esContext->eglNativeWindow, NULL );

   if ( esContext->eglSurface == EGL_NO_SURFACE )
   {
      return GL_FALSE;
   }

   // Create a GL context
   // 创建一个渲染上下文
   // EGL_NO_CONTEXT ： 不允许多个EGL上下文共享特定类型的数据
   // contextAttribs ： 创建上下文使用的属性列表，只能指定EGL_CONTEXT_CLIENT_VERSION的版本
   esContext->eglContext = eglCreateContext ( esContext->eglDisplay, config, 
                                              EGL_NO_CONTEXT, contextAttribs );

   if ( esContext->eglContext == EGL_NO_CONTEXT )
   {
      return GL_FALSE;
   }

   // Make the context current
   // 指定一个context为当前上下文（因为一个应用程序可能创建多个context用于不同的用途，所以需要关联特定的Context和surface）
   if ( !eglMakeCurrent ( esContext->eglDisplay, esContext->eglSurface, //这个surface用于表示EGL的绘图(draw)表面
                          esContext->eglSurface, esContext->eglContext ) )//这个surface用于表示EGL的读取(read)表面
   {
      return GL_FALSE;
   }

#endif // #ifndef __APPLE__

   return GL_TRUE;
}

///
//  esRegisterDrawFunc()
//
void ESUTIL_API esRegisterDrawFunc ( ESContext *esContext, void ( ESCALLBACK *drawFunc ) ( ESContext * ) )
{
   esContext->drawFunc = drawFunc;
}

///
//  esRegisterShutdownFunc()
//
void ESUTIL_API esRegisterShutdownFunc ( ESContext *esContext, void ( ESCALLBACK *shutdownFunc ) ( ESContext * ) )
{
   esContext->shutdownFunc = shutdownFunc;
}

///
//  esRegisterUpdateFunc()
//
void ESUTIL_API esRegisterUpdateFunc ( ESContext *esContext, void ( ESCALLBACK *updateFunc ) ( ESContext *, float ) )
{
   esContext->updateFunc = updateFunc;
}


///
//  esRegisterKeyFunc()
//
void ESUTIL_API esRegisterKeyFunc ( ESContext *esContext,
                                    void ( ESCALLBACK *keyFunc ) ( ESContext *, unsigned char, int, int ) )
{
   esContext->keyFunc = keyFunc;
}


///
// esLogMessage()
//
//    Log an error message to the debug output for the platform
//
void ESUTIL_API esLogMessage ( const char *formatStr, ... )
{
   va_list params;
   char buf[BUFSIZ];

   va_start ( params, formatStr );
   vsprintf ( buf, formatStr, params );

#ifdef ANDROID
   __android_log_print ( ANDROID_LOG_INFO, "esUtil" , "%s", buf );
#else
   printf ( "%s", buf );
#endif

   va_end ( params );
}

///
// esFileRead()
//
//    Wrapper for platform specific File open
//
static esFile *esFileOpen ( void *ioContext, const char *fileName )
{
   esFile *pFile = NULL;

#ifdef ANDROID

   if ( ioContext != NULL )
   {
      AAssetManager *assetManager = ( AAssetManager * ) ioContext;
      pFile = AAssetManager_open ( assetManager, fileName, AASSET_MODE_BUFFER );
   }

#else
#ifdef __APPLE__
   // iOS: Remap the filename to a path that can be opened from the bundle.
   fileName = GetBundleFileName ( fileName );
#endif

   pFile = fopen ( fileName, "rb" );
#endif

   return pFile;
}

///
// esFileRead()
//
//    Wrapper for platform specific File close
//
static void esFileClose ( esFile *pFile )
{
   if ( pFile != NULL )
   {
#ifdef ANDROID
      AAsset_close ( pFile );
#else
      fclose ( pFile );
      pFile = NULL;
#endif
   }
}

///
// esFileRead()
//
//    Wrapper for platform specific File read
//
static int esFileRead ( esFile *pFile, int bytesToRead, void *buffer )
{
   int bytesRead = 0;

   if ( pFile == NULL )
   {
      return bytesRead;
   }

#ifdef ANDROID
   bytesRead = AAsset_read ( pFile, buffer, bytesToRead );
#else
   bytesRead = fread ( buffer, bytesToRead, 1, pFile );
#endif

   return bytesRead;
}

///
// esLoadTGA()
//
//    Loads a 8-bit, 24-bit or 32-bit TGA image from a file
// 加载TGA图像文件
char *ESUTIL_API esLoadTGA ( void *ioContext, const char *fileName, int *width, int *height )
{
   char        *buffer;
   esFile      *fp;
   TGA_HEADER   Header;
   int          bytesRead;

   // Open the file for reading
   fp = esFileOpen ( ioContext, fileName );

   if ( fp == NULL )
   {
      // Log error as 'error in opening the input file from apk'
      esLogMessage ( "esLoadTGA FAILED to load : { %s }\n", fileName );
      return NULL;
   }

   bytesRead = esFileRead ( fp, sizeof ( TGA_HEADER ), &Header );

   *width = Header.Width;
   *height = Header.Height;

   if ( Header.ColorDepth == 8 ||
         Header.ColorDepth == 24 || Header.ColorDepth == 32 )
   {
      int bytesToRead = sizeof ( char ) * ( *width ) * ( *height ) * Header.ColorDepth / 8;

      // Allocate the image data buffer
      buffer = ( char * ) malloc ( bytesToRead );

      if ( buffer )
      {
         bytesRead = esFileRead ( fp, bytesToRead, buffer );
         esFileClose ( fp );

         return ( buffer );
      }
   }

   return ( NULL );
}
