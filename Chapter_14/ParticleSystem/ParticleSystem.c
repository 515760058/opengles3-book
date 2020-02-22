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
// ParticleSystem.c
//
//    This is an example that demonstrates rendering a particle system
//    using a vertex shader and point sprites.
//  使用顶点着色器和点精灵图元的粒子系统
#include <stdlib.h>
#include <math.h>
#include "esUtil.h"

#define NUM_PARTICLES   1000 // 粒子的数量
#define PARTICLE_SIZE   7  // 每个粒子的：寿命时长float + 起始位置vec3 + 结束位置vec3

#define ATTRIBUTE_LIFETIME_LOCATION       0
#define ATTRIBUTE_STARTPOSITION_LOCATION  1
#define ATTRIBUTE_ENDPOSITION_LOCATION    2

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Uniform location
   GLint timeLoc;
   GLint colorLoc;
   GLint centerPositionLoc;
   GLint samplerLoc;

   // Texture handle
   GLuint textureId;

   // Particle vertex data
   float particleData[ NUM_PARTICLES * PARTICLE_SIZE ]; //粒子数据

   // Current time
   float time;

} UserData;

///
// Load texture from disk
// 加载纹理图像
GLuint LoadTexture ( void *ioContext, char *fileName )
{
   int width, height;
   char *buffer = esLoadTGA ( ioContext, fileName, &width, &height );
   GLuint texId;

   if ( buffer == NULL )
   {
      esLogMessage ( "Error loading (%s) image.\n", fileName );
      return 0;
   }

   glGenTextures ( 1, &texId );
   glBindTexture ( GL_TEXTURE_2D, texId );
   // 申请GPU内存，上传纹理数据
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
   //设置纹理的过滤模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   // 设置纹理的包装模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   free ( buffer );

   return texId;
}


///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   int i;

   char vShaderStr[] =
      "#version 300 es                                      \n"
      "uniform float u_time;                                \n"//当前时间
      "uniform vec3 u_centerPosition;                       \n"//当前一次粒子动画的中心位置
      "layout(location = 0) in float a_lifetime;            \n"//当前粒子的寿命时长
      "layout(location = 1) in vec3 a_startPosition;        \n"//当前粒子的起始位置
      "layout(location = 2) in vec3 a_endPosition;          \n"//当前粒子的结束位置
      "out float v_lifetime;                                \n"//out：当前粒子的剩余寿命
      "void main()                                          \n"
      "{                                                    \n"
      "  if ( u_time <= a_lifetime )                        \n"
      "  {                                                  \n"
      "    gl_Position.xyz = a_startPosition +              \n"//根据时间，线性插值
      "                      (u_time * a_endPosition);      \n"
      "    gl_Position.xyz += u_centerPosition;             \n"//粒子的位置变化
      "    gl_Position.w = 1.0;                             \n"
      "  }                                                  \n"
      "  else                                               \n"
      "  {                                                  \n"
      "     gl_Position = vec4( -1000, -1000, 0, 0 );       \n"//屏幕外的裁剪
      "  }                                                  \n"
      "  v_lifetime = 1.0 - ( u_time / a_lifetime );        \n"//剩余寿命
      "  v_lifetime = clamp ( v_lifetime, 0.0, 1.0 );       \n"
      "  gl_PointSize = ( v_lifetime * v_lifetime ) * 40.0; \n"//根据剩余寿命，调整粒子的size
      "}";

   char fShaderStr[] =
      "#version 300 es                                      \n"
      "precision mediump float;                             \n"
      "uniform vec4 u_color;                                \n"//当前一次粒子动画的颜色
      "in float v_lifetime;                                 \n"//剩余寿命
      "layout(location = 0) out vec4 fragColor;             \n"//输出的颜色
      "uniform sampler2D s_texture;                         \n"//采样器
      "void main()                                          \n"
      "{                                                    \n"
      "  vec4 texColor;                                     \n"
      "  texColor = texture( s_texture, gl_PointCoord );    \n"//使用采样器，根据粒子坐标从纹理采样
      "  fragColor = vec4( u_color ) * texColor;            \n"//根据u_color，衰减粒子的颜色
      "  fragColor.a *= v_lifetime;                         \n"//根据剩余寿命，衰减粒子的透明度阿尔法值
      "}                                                    \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the uniform locations
   userData->timeLoc = glGetUniformLocation ( userData->programObject, "u_time" );
   userData->centerPositionLoc = glGetUniformLocation ( userData->programObject, "u_centerPosition" );
   userData->colorLoc = glGetUniformLocation ( userData->programObject, "u_color" );
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
   //黑色背景
   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

   // Fill in particle data array 生成1000个粒子的数据（寿命、起始坐标和结束坐标）
   srand ( 0 );//随机种子
   //随机生成每个粒子的寿命、起始坐标和结束坐标
   for ( i = 0; i < NUM_PARTICLES; i++ )
   {
      float *particleData = &userData->particleData[i * PARTICLE_SIZE];

      // Lifetime of particle 寿命
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 10000.0f );

      // End position of particle 结束坐标
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 5000.0f ) - 1.0f;
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 5000.0f ) - 1.0f;
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 5000.0f ) - 1.0f;

      // Start position of particle 起始坐标
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 40000.0f ) - 0.125f;
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 40000.0f ) - 0.125f;
      ( *particleData++ ) = ( ( float ) ( rand() % 10000 ) / 40000.0f ) - 0.125f;
   }

   // Initialize time to cause reset on first update 初始时间1.0，可以直接在update中更新中心位置和颜色
   userData->time = 1.0f;
   //加载纹理图像
   userData->textureId = LoadTexture ( esContext->platformData, "smoke.tga" );

   if ( userData->textureId <= 0 )
   {
      return FALSE;
   }

   return TRUE;
}

///
//  Update time-based variables
//
void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;

   userData->time += deltaTime;

   glUseProgram ( userData->programObject );

   if ( userData->time >= 1.0f )//每隔1秒，设置一次粒子动画的 中心位置和颜色
   {
      float centerPos[3];
      float color[4];

      userData->time = 0.0f;

      // Pick a new start location and color 中心位置
      centerPos[0] = ( ( float ) ( rand() % 10000 ) / 10000.0f ) - 0.5f;
      centerPos[1] = ( ( float ) ( rand() % 10000 ) / 10000.0f ) - 0.5f;
      centerPos[2] = ( ( float ) ( rand() % 10000 ) / 10000.0f ) - 0.5f;
      // 设置统一变量u_centerPosition
      glUniform3fv ( userData->centerPositionLoc, 1, &centerPos[0] );

      // Random color 颜色
      color[0] = ( ( float ) ( rand() % 10000 ) / 20000.0f ) + 0.5f;
      color[1] = ( ( float ) ( rand() % 10000 ) / 20000.0f ) + 0.5f;
      color[2] = ( ( float ) ( rand() % 10000 ) / 20000.0f ) + 0.5f;
      color[3] = 0.5;
      // 设置统一变量u_color
      glUniform4fv ( userData->colorLoc, 1, &color[0] );
   }

   // Load uniform time variable 每一帧，就设置一次当前时间统一变量u_time
   glUniform1f ( userData->timeLoc, userData->time );
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load the vertex attributes 指定粒子顶点的寿命时间(float)的格式，及其数据地址
   glVertexAttribPointer ( ATTRIBUTE_LIFETIME_LOCATION, 1, GL_FLOAT,
                           GL_FALSE, PARTICLE_SIZE * sizeof ( GLfloat ),//步长 7个float字节
                           userData->particleData );
   // 指定粒子顶点的结束位置(vec3)的格式，及其数据地址    步长 7个float字节
   glVertexAttribPointer ( ATTRIBUTE_ENDPOSITION_LOCATION, 3, GL_FLOAT,
                           GL_FALSE, PARTICLE_SIZE * sizeof ( GLfloat ),
                           &userData->particleData[1] );
   // 指定粒子顶点的起始位置(vec3)的格式，及其数据地址   步长 7个float字节
   glVertexAttribPointer ( ATTRIBUTE_STARTPOSITION_LOCATION, 3, GL_FLOAT,
                           GL_FALSE, PARTICLE_SIZE * sizeof ( GLfloat ),
                           &userData->particleData[4] );
   // 使能顶点数组
   glEnableVertexAttribArray ( ATTRIBUTE_LIFETIME_LOCATION );
   glEnableVertexAttribArray ( ATTRIBUTE_ENDPOSITION_LOCATION );
   glEnableVertexAttribArray ( ATTRIBUTE_STARTPOSITION_LOCATION );

   // Blend particles 使能混合，设置混合函数
   glEnable ( GL_BLEND );
   glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

   // Bind the texture 使用纹理
   glActiveTexture ( GL_TEXTURE0 );//激活纹理单元0
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );//把纹理绑定到纹理单元0

   // Set the sampler texture unit to 0 设置采样器s_texture使用纹理单元0
   glUniform1i ( userData->samplerLoc, 0 );
   //没有使用glDrawElements，参见书P268
   glDrawArrays ( GL_POINTS, 0, NUM_PARTICLES );
}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Delete texture object
   glDeleteTextures ( 1, &userData->textureId );

   // Delete program object
   glDeleteProgram ( userData->programObject );
}


int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "ParticleSystem", 640, 480, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }
   //注册回调函数，退出esMain之后，框架将循环调用注册的Draw和Update，直到窗口关闭
   esRegisterDrawFunc ( esContext, Draw );
   esRegisterUpdateFunc ( esContext, Update );

   esRegisterShutdownFunc ( esContext, ShutDown );

   return GL_TRUE;
}
