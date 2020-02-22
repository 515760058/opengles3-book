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
// ParticleSystemTransformFeedback.c
//
//    This is an example that demonstrates a particle system
//    using transform feedback.
// 14.4节 使用变换反馈的粒子系统
#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include "esUtil.h"
#include "Noise3D.h"

#define NUM_PARTICLES   200
#define EMISSION_RATE   0.3f
#define ACCELERATION   -1.0f

#define ATTRIBUTE_POSITION      0
#define ATTRIBUTE_VELOCITY      1
#define ATTRIBUTE_SIZE          2
#define ATTRIBUTE_CURTIME       3
#define ATTRIBUTE_LIFETIME      4

typedef struct
{
   float position[2];//粒子的位置xy
   float velocity[2];//粒子的速度，x方向和y方向
   float size;//粒子的size大小
   float curtime;//粒子的生成时间
   float lifetime;//粒子的寿命
} Particle;

typedef struct
{
   // Handle to a program object
   GLuint emitProgramObject;
   GLuint drawProgramObject;

   // Emit shader uniform locations
   GLint emitTimeLoc;
   GLint emitEmissionRateLoc;
   GLint emitNoiseSamplerLoc;

   // Draw shader uniform location
   GLint drawTimeLoc;
   GLint drawColorLoc;
   GLint drawAccelerationLoc;
   GLint samplerLoc;

   // Texture handles
   GLuint textureId;
   GLuint noiseTextureId;

   // Particle vertex data
   Particle particleData[ NUM_PARTICLES ];//200个粒子的数组

   // Particle VBOs  两个VBO，交替
   GLuint particleVBOs[2];

   // Index into particleVBOs (0 or 1) as to which is the source. Ping-pong between the two VBOs
   GLuint curSrcIndex;//源VBO的下标

   // Current time
   float time;//当前时间（帧）

   // synch object to synchronize the transform feedback results and the draw
   GLsync emitSync;//同步对象：保证先变换反馈，然后才能渲染

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
   //申请GPU内存，上传纹理数据
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
   //过滤模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   //包装模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   free ( buffer );

   return texId;
}

void InitEmitParticles ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
    //“粒子发射”顶点着色器，结果输出到变换反馈缓冲区中
   char vShaderStr[] =
      "#version 300 es                                                     \n"
      "#define NUM_PARTICLES           200                                 \n"
      "#define ATTRIBUTE_POSITION      0                                   \n"
      "#define ATTRIBUTE_VELOCITY      1                                   \n"
      "#define ATTRIBUTE_SIZE          2                                   \n"
      "#define ATTRIBUTE_CURTIME       3                                   \n"
      "#define ATTRIBUTE_LIFETIME      4                                   \n"
      "uniform float u_time;                                               \n"//当前时间
      "uniform float u_emissionRate;                                       \n"//发射速度
      "uniform mediump sampler3D s_noiseTex;                               \n"//噪音纹理
      "                                                                    \n"
      "layout(location = ATTRIBUTE_POSITION) in vec2 a_position;           \n"//粒子（顶点）的属性值
      "layout(location = ATTRIBUTE_VELOCITY) in vec2 a_velocity;           \n"//速度
      "layout(location = ATTRIBUTE_SIZE) in float a_size;                  \n"
      "layout(location = ATTRIBUTE_CURTIME) in float a_curtime;            \n"//生成时间
      "layout(location = ATTRIBUTE_LIFETIME) in float a_lifetime;          \n"
      "                                                                    \n"
      "out vec2 v_position;                                                \n"
      "out vec2 v_velocity;                                                \n"
      "out float v_size;                                                   \n"
      "out float v_curtime;                                                \n"
      "out float v_lifetime;                                               \n"
      "                                                                    \n"
      "float randomValue( inout float seed )                               \n"//获得随机值的函数
      "{                                                                   \n"
      "   float vertexId = float( gl_VertexID ) / float( NUM_PARTICLES );  \n"
      "   vec3 texCoord = vec3( u_time, vertexId, seed );                  \n"
      "   seed += 0.1;                                                     \n"
      "   return texture( s_noiseTex, texCoord ).r;                        \n"//计算坐标，从噪音纹理中采样，作为随机值
      "}                                                                   \n"
      "void main()                                                         \n"
      "{                                                                   \n"
      "  float seed = u_time;                                              \n"//当前时间作为随机种子
      "  float lifetime = a_curtime - u_time;                              \n"
      "  if( lifetime <= 0.0 && randomValue(seed) < u_emissionRate )       \n"//根据seed获取随机数，随机认为当前粒子死亡，需要重新初始化状态。（lifetime <= 0.0一直是成立的吧）
      "  {                                                                 \n"
      "     v_position = vec2( 0.0, -1.0 );                                \n"//初始位置
      "     v_velocity = vec2( randomValue(seed) * 2.0 - 1.00,             \n"//初始x和y方向的速度
      "                        randomValue(seed) * 1.4 + 1.0 );            \n"
      "     v_size = randomValue(seed) * 20.0 + 60.0;                      \n"//初始大小
      "     v_curtime = u_time;                                            \n"//生成时间
      "     v_lifetime = 2.0;                                              \n"//寿命
      "  }                                                                 \n"
      "  else                                                              \n"//当前粒子还活着，状态不需要不变
      "  {                                                                 \n"
      "     v_position = a_position;                                       \n"
      "     v_velocity = a_velocity;                                       \n"
      "     v_size = a_size;                                               \n"
      "     v_curtime = a_curtime;                                         \n"
      "     v_lifetime = a_lifetime;                                       \n"
      "  }                                                                 \n"
      "  gl_Position = vec4( v_position, 0.0, 1.0 );                       \n"
      "}                                                                   \n";
   //这个片段着色器应该用不到，只是因为ProgramObject需要而凑数的？
   char fShaderStr[] =
      "#version 300 es                                      \n"
      "precision mediump float;                             \n"
      "layout(location = 0) out vec4 fragColor;             \n"
      "void main()                                          \n"
      "{                                                    \n"
      "  fragColor = vec4(1.0);                             \n"
      "}                                                    \n";
   //编译“粒子发射”顶点着色器
   userData->emitProgramObject = esLoadProgram ( vShaderStr, fShaderStr );

   {
      const char *feedbackVaryings[5] =
      {
         "v_position",
         "v_velocity",
         "v_size",
         "v_curtime",
         "v_lifetime"
      };

      // Set the vertex shader outputs as transform feedback varyings 设置顶点着色器的输出变量用于变换反馈（需要在glLinkProgram之前调用）
      glTransformFeedbackVaryings ( userData->emitProgramObject, 5, feedbackVaryings, GL_INTERLEAVED_ATTRIBS );

      // Link program must occur after calling glTransformFeedbackVaryings  疑问：esLoadProgram函数中也调用过glLinkProgram...
      glLinkProgram ( userData->emitProgramObject );

      // Get the uniform locations - this also needs to happen after glLinkProgram is called again so
      // that the uniforms that output to varyings are active  发射program中的统一变量location
      userData->emitTimeLoc = glGetUniformLocation ( userData->emitProgramObject, "u_time" );
      userData->emitEmissionRateLoc = glGetUniformLocation ( userData->emitProgramObject, "u_emissionRate" );
      userData->emitNoiseSamplerLoc = glGetUniformLocation ( userData->emitProgramObject, "s_noiseTex" );
   }
}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   Particle particleData[ NUM_PARTICLES ];//200个粒子数组
   UserData *userData = ( UserData * ) esContext->userData;
   int i;
   // “粒子渲染”顶点着色器，使用变换反馈的内容作为输入变量
   char vShaderStr[] =
      "#version 300 es                                                     \n"
      "#define ATTRIBUTE_POSITION      0                                   \n"
      "#define ATTRIBUTE_VELOCITY      1                                   \n"
      "#define ATTRIBUTE_SIZE          2                                   \n"
      "#define ATTRIBUTE_CURTIME       3                                   \n"
      "#define ATTRIBUTE_LIFETIME      4                                   \n"
      "                                                                    \n"
      "layout(location = ATTRIBUTE_POSITION) in vec2 a_position;           \n"//位置
      "layout(location = ATTRIBUTE_VELOCITY) in vec2 a_velocity;           \n"//速度
      "layout(location = ATTRIBUTE_SIZE) in float a_size;                  \n"
      "layout(location = ATTRIBUTE_CURTIME) in float a_curtime;            \n"//生成粒子的时间戳
      "layout(location = ATTRIBUTE_LIFETIME) in float a_lifetime;          \n"//生命周期
      "                                                                    \n"
      "uniform float u_time;                                               \n"//当前时间
      "uniform vec2 u_acceleration;                                        \n"//加速度
      "                                                                    \n"
      "void main()                                                         \n"
      "{                                                                   \n"
      "  float deltaTime = u_time - a_curtime;                             \n"
      "  if ( deltaTime <= a_lifetime )                                    \n"//粒子还存活，则计算属性数据
      "  {                                                                 \n"
      "     vec2 velocity = a_velocity + deltaTime * u_acceleration;       \n"//根据加速度和时间，计算粒子的速度
      "     vec2 position = a_position + deltaTime * velocity;             \n"//根据速度和原位置，计算粒子的新位置
      "     gl_Position = vec4( position, 0.0, 1.0 );                      \n"
      "     gl_PointSize = a_size * ( 1.0 - deltaTime / a_lifetime );      \n"//根据时间，计算粒子的size大小
      "  }                                                                 \n"
      "  else                                                              \n"//粒子应该消失
      "  {                                                                 \n"
      "     gl_Position = vec4( -1000, -1000, 0, 0 );                      \n"//坐标在屏幕之外
      "     gl_PointSize = 0.0;                                            \n"
      "  }                                                                 \n"
      "}";
   //“粒子渲染”片段着色器
   char fShaderStr[] =
      "#version 300 es                                      \n"
      "precision mediump float;                             \n"
      "layout(location = 0) out vec4 fragColor;             \n"
      "uniform vec4 u_color;                                \n"//颜色
      "uniform sampler2D s_texture;                         \n"//采样器
      "void main()                                          \n"
      "{                                                    \n"
      "  vec4 texColor;                                     \n"
      "  texColor = texture( s_texture, gl_PointCoord );    \n"//采样纹理的颜色
      "  fragColor = texColor * u_color;                    \n"//对纹理颜色做衰减，得到输出颜色
      "}                                                    \n";
   //初始化“粒子发射”顶点着色器，并设置输出到变换反馈缓冲区
   InitEmitParticles ( esContext );

   // Load the shaders and get a linked program object 初始化“粒子渲染”顶点着色器和“粒子渲染”片段着色器
   userData->drawProgramObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the uniform locations 渲染program中的统一变量location
   userData->drawTimeLoc = glGetUniformLocation ( userData->drawProgramObject, "u_time" );
   userData->drawColorLoc = glGetUniformLocation ( userData->drawProgramObject, "u_color" );
   userData->drawAccelerationLoc = glGetUniformLocation ( userData->drawProgramObject, "u_acceleration" );
   userData->samplerLoc = glGetUniformLocation ( userData->drawProgramObject, "s_texture" );

   userData->time = 0.0f;
   userData->curSrcIndex = 0;
   //黑色背景
   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   //加载图像纹理
   userData->textureId = LoadTexture ( esContext->platformData, "smoke.tga" );

   if ( userData->textureId <= 0 )
   {
      return FALSE;
   }

   // Create a 3D noise texture for random values
   userData->noiseTextureId = Create3DNoiseTexture ( 128, 50.0 );

   // Initialize particle data 初始化两百个粒子数据
   for ( i = 0; i < NUM_PARTICLES; i++ )
   {
      Particle *particle = &particleData[i];
      particle->position[0] = 0.0f;
      particle->position[1] = 0.0f;
      particle->velocity[0] = 0.0f;
      particle->velocity[1] = 0.0f;
      particle->size = 0.0f;
      particle->curtime = 0.0f;
      particle->lifetime = 0.0f;
   }


   // Create the particle VBOs
   glGenBuffers ( 2, &userData->particleVBOs[0] );

   for ( i = 0; i < 2; i++ )//给两个VBO申请内存，并且都上传粒子数据，GL_DYNAMIC_COPY方向
   {
      glBindBuffer ( GL_ARRAY_BUFFER, userData->particleVBOs[i] );
      glBufferData ( GL_ARRAY_BUFFER, sizeof ( Particle ) * NUM_PARTICLES, particleData, GL_DYNAMIC_COPY );
   }

   return TRUE;
}

//使用VBO，设置顶点属性的数据地址（偏移）
void SetupVertexAttributes ( ESContext *esContext, GLuint vboID )
{
   glBindBuffer ( GL_ARRAY_BUFFER, vboID );
   //顶点位置属性的格式，及其在VBO缓冲区中的偏移
   glVertexAttribPointer ( ATTRIBUTE_POSITION, 2, GL_FLOAT,
                           GL_FALSE, sizeof ( Particle ),
                           ( const void * ) NULL );
   //顶点速度属性的格式，及其在VBO缓冲区中的偏移
   glVertexAttribPointer ( ATTRIBUTE_VELOCITY, 2, GL_FLOAT,
                           GL_FALSE, sizeof ( Particle ),
                           ( const void * ) offsetof ( Particle, velocity[0] ) );
   //顶点size大小属性的格式，及其在VBO缓冲区中的偏移
   glVertexAttribPointer ( ATTRIBUTE_SIZE, 1, GL_FLOAT,
                           GL_FALSE, sizeof ( Particle ),
                           ( const void * ) offsetof ( Particle, size ) );
   //顶点生成时间属性的格式，及其在VBO缓冲区中的偏移
   glVertexAttribPointer ( ATTRIBUTE_CURTIME, 1, GL_FLOAT,
                           GL_FALSE, sizeof ( Particle ),
                           ( const void * ) offsetof ( Particle, curtime ) );
   //顶点寿命属性的格式，及其在VBO缓冲区中的偏移
   glVertexAttribPointer ( ATTRIBUTE_LIFETIME, 1, GL_FLOAT,
                           GL_FALSE, sizeof ( Particle ),
                           ( const void * ) offsetof ( Particle, lifetime ) );
   //使能顶点数组
   glEnableVertexAttribArray ( ATTRIBUTE_POSITION );
   glEnableVertexAttribArray ( ATTRIBUTE_VELOCITY );
   glEnableVertexAttribArray ( ATTRIBUTE_SIZE );
   glEnableVertexAttribArray ( ATTRIBUTE_CURTIME );
   glEnableVertexAttribArray ( ATTRIBUTE_LIFETIME );
}

void EmitParticles ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;
   // 获取 源VBO 和 目的VBO
   GLuint srcVBO = userData->particleVBOs[ userData->curSrcIndex ];//源VBO
   GLuint dstVBO = userData->particleVBOs[ ( userData->curSrcIndex + 1 ) % 2 ];//目的VBO（作为变换反馈结果的缓冲区）

   glUseProgram ( userData->emitProgramObject );
   //使用源VBO，设置顶点属性的数据地址（偏移）
   SetupVertexAttributes ( esContext, srcVBO );

   // Set transform feedback buffer 把VBO目的缓冲区，设置为变换反馈，用以保存结果
   glBindBuffer ( GL_TRANSFORM_FEEDBACK_BUFFER, dstVBO );
   glBindBufferBase ( GL_TRANSFORM_FEEDBACK_BUFFER, 0, dstVBO );// ???

   // Turn off rasterization - we are not drawing
   glEnable ( GL_RASTERIZER_DISCARD );//禁用光栅化

   // Set uniforms 设置统一变量
   glUniform1f ( userData->emitTimeLoc, userData->time );
   glUniform1f ( userData->emitEmissionRateLoc, EMISSION_RATE );

   // Bind the 3D noise texture
   glActiveTexture ( GL_TEXTURE0 );//激活纹理单元0
   glBindTexture ( GL_TEXTURE_3D, userData->noiseTextureId );//把“噪音纹理”绑定到纹理单元0
   glUniform1i ( userData->emitNoiseSamplerLoc, 0 );//设置采样器使用纹理单元0

   // Emit particles using transform feedback 使用变换反馈 计算发射粒子
   glBeginTransformFeedback ( GL_POINTS );//开始变换反馈
   glDrawArrays ( GL_POINTS, 0, NUM_PARTICLES );//计算粒子的新状态
   glEndTransformFeedback();//结束变换反馈

   // Create a sync object to ensure transform feedback results are completed before the draw that uses them.
   //创建一个Fence同步对象（GPU_COMMANDS_COMPLETE的状态），确保在调用Draw函数之前，已经完成变换反馈。
   userData->emitSync = glFenceSync ( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );// Draw函数中会尝试获取这个同步对象

   // Restore state
   glDisable ( GL_RASTERIZER_DISCARD );//使能光栅化
   //把之前设置的状态，都清除成默认的状态
   glUseProgram ( 0 );
   glBindBufferBase ( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );
   glBindBuffer ( GL_ARRAY_BUFFER, 0 );
   glBindTexture ( GL_TEXTURE_3D, 0 );

   // Ping pong the buffers  交替使用目的VBO作为下一帧的源VBO
   userData->curSrcIndex = ( userData->curSrcIndex + 1 ) % 2;
}


///
//  Update time-based variables
//
void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = ( UserData * ) esContext->userData;

   userData->time += deltaTime;//时间一直往下走
   //完成“发射粒子”的变换反馈
   EmitParticles ( esContext, deltaTime );
}

///
// Draw a triangle using the shader pair created in Init()
// 变换反馈反馈之后，根据新的粒子状态，渲染图像
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Block the GL server until transform feedback results are completed
   //等待update函数中EmitParticles函数中的fence对象（表示“发射粒子”已经完成了）
   glWaitSync ( userData->emitSync, 0, GL_TIMEOUT_IGNORED );
   glDeleteSync ( userData->emitSync );

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->drawProgramObject );

   // Load the VBO and vertex attributes
   // 根据变换反馈的结果（“发射粒子”计算完成的状态），设置顶点属性数据
   SetupVertexAttributes ( esContext, userData->particleVBOs[ userData->curSrcIndex ] );

   // Set uniforms 设置统一变量
   glUniform1f ( userData->drawTimeLoc, userData->time );
   glUniform4f ( userData->drawColorLoc, 1.0f, 1.0f, 1.0f, 1.0f );//粒子的颜色，着色器中还要衰减操作，以及混合
   glUniform2f ( userData->drawAccelerationLoc, 0.0f, ACCELERATION );

   // Blend particles 混合
   glEnable ( GL_BLEND );
   glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );//激活纹理单元0
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );//把纹理绑定到纹理单元0

   // Set the sampler texture unit to 0 设置采样器使用纹理单元0
   glUniform1i ( userData->samplerLoc, 0 );

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
   glDeleteProgram ( userData->drawProgramObject );
   glDeleteProgram ( userData->emitProgramObject );

   glDeleteBuffers ( 2, &userData->particleVBOs[0] );
}


int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "ParticleSystemTransformFeedback", 640, 480, ES_WINDOW_RGB );

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

