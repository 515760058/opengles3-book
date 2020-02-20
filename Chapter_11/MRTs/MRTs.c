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
// MRTs.c
//
//    This is an example to demonstrate Multiple Render Targets and framebuffer blits.
//    First, we will render a quad that outputs four colors (red, green, blue, gray)
//    per fragment using MRTs.
//    Then, we will copy the four color buffers into four screen quadrants
//    using framebuffer blits.
//  11.7节  多重渲染目标
#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Handle to a framebuffer object
   GLuint fbo;

   // Texture handle
   GLuint colorTexId[4];

   // Texture size
   GLsizei textureWidth;
   GLsizei textureHeight;

} UserData;

///
// Initialize the framebuffer object and MRTs
//
int InitFBO ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   int i;
   GLint defaultFramebuffer = 0;
   const GLenum attachments[4] = 
   { 
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2,
      GL_COLOR_ATTACHMENT3 
   };

   glGetIntegerv ( GL_FRAMEBUFFER_BINDING, &defaultFramebuffer );//defaultFramebuffer的值是0

   // Setup fbo 生成并绑定fbo
   glGenFramebuffers ( 1, &userData->fbo );
   glBindFramebuffer ( GL_FRAMEBUFFER, userData->fbo );

   // Setup four output buffers and attach to fbo 生成四个纹理，作为fbo的attachment
   userData->textureHeight = userData->textureWidth = 400;
   glGenTextures ( 4, &userData->colorTexId[0] );
   for (i = 0; i < 4; ++i)
   {
      glBindTexture ( GL_TEXTURE_2D, userData->colorTexId[i] );
      // 为每个纹理申请GPU内存
      glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, 
                     userData->textureWidth, userData->textureHeight, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

      // Set the filtering mode 设置过滤模式
      glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      // 纹理附着到fbo作为attachment   注意参数attachments[i]
      glFramebufferTexture2D ( GL_DRAW_FRAMEBUFFER, attachments[i], 
                               GL_TEXTURE_2D, userData->colorTexId[i], 0 );
   }
   //指定片段颜色要写入的缓冲区（颜色attachment）
   glDrawBuffers ( 4, attachments );

   if ( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus ( GL_FRAMEBUFFER ) )
   {
      return FALSE;
   }
   //printf("=== %d\n", defaultFramebuffer);
   // Restore the original framebuffer 重新绑定为原来的fbo
   glBindFramebuffer ( GL_FRAMEBUFFER, defaultFramebuffer );

   return TRUE;
}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   char vShaderStr[] =
      "#version 300 es                            \n"
      "layout(location = 0) in vec4 a_position;   \n"
      "void main()                                \n"
      "{                                          \n"
      "   gl_Position = a_position;               \n"
      "}                                          \n";

   char fShaderStr[] =
       "#version 300 es                                     \n"
       "precision mediump float;                            \n"
       "layout(location = 0) out vec4 fragData0;            \n"//四个输出颜色
       "layout(location = 1) out vec4 fragData1;            \n"
       "layout(location = 2) out vec4 fragData2;            \n"
       "layout(location = 3) out vec4 fragData3;            \n"
       "void main()                                         \n"
       "{                                                   \n"
       "  // first buffer will contain red color            \n"
       "  fragData0 = vec4 ( 1, 0, 0, 1 );                  \n"
       "                                                    \n"
       "  // second buffer will contain green color         \n"
       "  fragData1 = vec4 ( 0, 1, 0, 1 );                  \n"
       "                                                    \n"
       "  // third buffer will contain blue color           \n"
       "  fragData2 = vec4 ( 0, 0, 1, 1 );                  \n"
       "                                                    \n"
       "  // fourth buffer will contain gray color          \n"            
       "  fragData3 = vec4 ( 0.5, 0.5, 0.5, 1 );            \n"
       "}                                                   \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   InitFBO ( esContext );

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
   return TRUE;
}

///
// Draw a quad and output four colors per pixel
//
void DrawGeometry ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,
                           -1.0f, -1.0f, 0.0f,
                            1.0f, -1.0f, 0.0f,
                            1.0f,  1.0f, 0.0f,
                         };
   GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load the vertex position  指明顶点位置的数据格式，和数据地址（其实没有用到顶点数据）
   glVertexAttribPointer ( 0, 3, GL_FLOAT,
                           GL_FALSE, 3 * sizeof ( GLfloat ), vVertices );
   //使能顶点数组
   glEnableVertexAttribArray ( 0 );

   // Draw a quad
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
}

///
// Copy MRT output buffers to screen
//
void BlitTextures ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // set the fbo for reading
   glBindFramebuffer ( GL_READ_FRAMEBUFFER, userData->fbo );//设置源fbo，从中读取。
 
   // Copy the output red buffer to lower left quadrant
   glReadBuffer ( GL_COLOR_ATTACHMENT0 );//选择读取的fbo的attachment
   glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,//源fbo的color attachment的宽高：400*400
                       0, 0, esContext->width/2, esContext->height/2, //目的fbo中，目的区域的宽高，左下角，红色
                       GL_COLOR_BUFFER_BIT, GL_LINEAR );//指定要读取的缓冲区，GL_COLOR_BUFFER_BIT表示颜色缓冲区, GL_DEPTH_BUFFER_BIT表示深度缓冲区

   // Copy the output green buffer to lower right quadrant
   glReadBuffer ( GL_COLOR_ATTACHMENT1 );
   glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                       esContext->width/2, 0, esContext->width, esContext->height/2,// 右下角，绿色
                       GL_COLOR_BUFFER_BIT, GL_LINEAR );

   // Copy the output blue buffer to upper left quadrant
   glReadBuffer ( GL_COLOR_ATTACHMENT2 );
   glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                       0, esContext->height/2, esContext->width/2, esContext->height, //左上角 蓝色
                       GL_COLOR_BUFFER_BIT, GL_LINEAR );

   // Copy the output gray buffer to upper right quadrant
   glReadBuffer ( GL_COLOR_ATTACHMENT3 );
   glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                       esContext->width/2, esContext->height/2, esContext->width, esContext->height,//右上角 灰色 
                       GL_COLOR_BUFFER_BIT, GL_LINEAR );
}

///
// Render to MRTs and screen
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLint defaultFramebuffer = 0;
   const GLenum attachments[4] = 
   { 
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2,
      GL_COLOR_ATTACHMENT3 
   };
   
   glGetIntegerv ( GL_FRAMEBUFFER_BINDING, &defaultFramebuffer );//原来的fbo，是0

   // FIRST: use MRTs to output four colors to four buffers
   glBindFramebuffer ( GL_FRAMEBUFFER, userData->fbo );//输出到这个fbo（的4个纹理attachment）
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   glDrawBuffers ( 4, attachments );//为渲染指定要写入的缓冲区（例子中是颜色attachment）
   //
   DrawGeometry ( esContext );

   // SECOND: copy the four output buffers into four window quadrants with framebuffer blits
   // 第二步：把fbo中的四个attachment中的数据，拷贝出来

   // Restore the default framebuffer
   glBindFramebuffer ( GL_DRAW_FRAMEBUFFER, defaultFramebuffer );//绑定（要写入的）目的fbo，下一个函数中还绑定了源fbo。
   BlitTextures ( esContext );
}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Delete texture objects
   glDeleteTextures ( 4, userData->colorTexId );

   // Delete fbo
   glDeleteFramebuffers ( 1, &userData->fbo );

   // Delete program object
   glDeleteProgram ( userData->programObject );
}

int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );
   //注意窗口大小400*400，initFBO中每个纹理的宽高也都是400（使用对应的过滤模式）
   esCreateWindow ( esContext, "Multiple Render Targets", 400, 400, ES_WINDOW_RGB | ES_WINDOW_ALPHA );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterDrawFunc ( esContext, Draw );
   esRegisterShutdownFunc ( esContext, ShutDown );

   return GL_TRUE;
}
