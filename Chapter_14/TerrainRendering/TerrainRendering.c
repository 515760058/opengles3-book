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
// TerrainRendering.c
//
//    Demonstrates rendering a terrain with vertex texture fetch
// 14.9节 地形渲染
#include <stdlib.h>
#include <math.h>
#include "esUtil.h"

#define POSITION_LOC    0

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Uniform locations
   GLint  mvpLoc;
   GLint  lightDirectionLoc;

   // Sampler location
   GLint samplerLoc;

   // Texture handle
   GLuint textureId;

   // VBOs
   GLuint positionVBO;
   GLuint indicesIBO;

   // Number of indices
   int    numIndices;

   // dimension of grid
   int    gridSize;

   // MVP matrix
   ESMatrix  mvpMatrix;
} UserData;

///
// Load texture from disk
// 加载纹理图像
GLuint LoadTexture (  void *ioContext, char *fileName )
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
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, buffer );
   //过滤模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   //包装模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   free ( buffer );

   return texId;
}

///
// Initialize the MVP matrix
// 生成MVP矩阵
int InitMVP ( ESContext *esContext )
{
   ESMatrix perspective;
   ESMatrix modelview;
   float    aspect;
   UserData *userData = esContext->userData;

   // Compute the window aspect ratio
   aspect = ( GLfloat ) esContext->width / ( GLfloat ) esContext->height;

   // Generate a perspective matrix with a 60 degree FOV 投影矩阵
   esMatrixLoadIdentity ( &perspective );
   esPerspective ( &perspective, 60.0f, aspect, 0.1f, 20.0f );

   // Generate a model view matrix to rotate/translate the terrain
   esMatrixLoadIdentity ( &modelview );
   // Center the terrain
   esTranslate ( &modelview, -0.5f, -0.5f, -0.7f );
   // Rotate
   esRotate ( &modelview, 45.0f, 1.0, 0.0, 0.0 );

   // Compute the final MVP by multiplying the
   // modelview and perspective matrices together  MVP = MV * P
   esMatrixMultiply ( &userData->mvpMatrix, &modelview, &perspective );

   return TRUE;
}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   GLfloat *positions;
   GLuint *indices;

   UserData *userData = esContext->userData;
   const char vShaderStr[] =
      "#version 300 es                                      \n"
      "uniform mat4 u_mvpMatrix;                            \n"
      "uniform vec3 u_lightDirection;                       \n"//平行光的方向
      "layout(location = 0) in vec4 a_position;             \n"
      "uniform sampler2D s_texture;                         \n"//纹理的采样器
      "out vec4 v_color;                                    \n"
      "void main()                                          \n"
      "{                                                    \n"
      "   // compute vertex normal from height map          \n"//从高度图，计算法线
      "   float hxl = textureOffset( s_texture,             \n"
      "                  a_position.xy, ivec2(-1,  0) ).w;  \n"
      "   float hxr = textureOffset( s_texture,             \n"
      "                  a_position.xy, ivec2( 1,  0) ).w;  \n"
      "   float hyl = textureOffset( s_texture,             \n"
      "                  a_position.xy, ivec2( 0, -1) ).w;  \n"
      "   float hyr = textureOffset( s_texture,             \n"
      "                  a_position.xy, ivec2( 0,  1) ).w;  \n"
      "   vec3 u = normalize( vec3(0.05, 0.0, hxr-hxl) );   \n"
      "   vec3 v = normalize( vec3(0.0, 0.05, hyr-hyl) );   \n"
      "   vec3 normal = cross( u, v );                      \n"//顶点的法线
      "                                                     \n"
      "   // compute diffuse lighting                       \n"//计算漫射光
      "   float diffuse = dot( normal, u_lightDirection );  \n"
      "   v_color = vec4( vec3(diffuse), 1.0 );             \n"
      "                                                     \n"
      "   // get vertex position from height map            \n"
      "   float h = texture ( s_texture, a_position.xy ).w; \n"
      "   vec4 v_position = vec4 ( a_position.xy,           \n"//从高度图中，获取顶点的高度
      "                            h/2.5,                   \n"
      "                            a_position.w );          \n"
      "   gl_Position = u_mvpMatrix * v_position;           \n"//转换坐标
      "}                                                    \n";

   const char fShaderStr[] =
      "#version 300 es                                      \n"
      "precision mediump float;                             \n"
      "in vec4 v_color;                                     \n"
      "layout(location = 0) out vec4 outColor;              \n"
      "void main()                                          \n"
      "{                                                    \n"
      "  outColor = v_color;                                \n"
      "}                                                    \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the uniform locations 统一变量的location
   userData->mvpLoc = glGetUniformLocation ( userData->programObject, "u_mvpMatrix" );
   userData->lightDirectionLoc = glGetUniformLocation ( userData->programObject, "u_lightDirection" );
   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   // Load the heightmap 加载高度图（纹理）
   userData->textureId = LoadTexture ( esContext->platformData, "heightmap.tga" );

   if ( userData->textureId == 0 )
   {
      return FALSE;
   }

   // Generate the position and indices of a square grid for the base terrain
   userData->gridSize = 200;
   userData->numIndices = esGenSquareGrid ( userData->gridSize, &positions, &indices );//生成顶点的位置和索引数据

   // Index buffer for base terrain 顶点 索引数据的VBO
   glGenBuffers ( 1, &userData->indicesIBO );
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, userData->indicesIBO );
   //申请GPU内存，上传索引数据
   glBufferData ( GL_ELEMENT_ARRAY_BUFFER, userData->numIndices * sizeof ( GLuint ), indices, GL_STATIC_DRAW );
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );//恢复到默认的buffer
   free ( indices );

   // Position VBO for base terrain 顶点 位置数据的VB0
   glGenBuffers ( 1, &userData->positionVBO );
   glBindBuffer ( GL_ARRAY_BUFFER, userData->positionVBO );
   //申请GPU内存，上传索引数据
   glBufferData ( GL_ARRAY_BUFFER, userData->gridSize * userData->gridSize * sizeof ( GLfloat ) * 3, positions, GL_STATIC_DRAW );
   free ( positions );

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );//白色背景

   return TRUE;
}

///
// Draw a flat grid
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   InitMVP ( esContext );

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load the vertex position 加载顶点位置数据
   glBindBuffer ( GL_ARRAY_BUFFER, userData->positionVBO );
   //指明顶点位置数据在VB0中的格式，和地址偏移
   glVertexAttribPointer ( POSITION_LOC, 3, GL_FLOAT, GL_FALSE, 3 * sizeof ( GLfloat ), ( const void * ) NULL );
   glEnableVertexAttribArray ( POSITION_LOC );//使能顶点数组

   // Bind the index buffer 顶点索引数据
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, userData->indicesIBO );

   // Bind the height map 把纹理绑定到纹理单元0  从而采样器可以使用
   glActiveTexture ( GL_TEXTURE0 );//激活纹理单元0
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );//把纹理绑定到纹理单元0 
   // Set the height map sampler to texture unit to 0
   glUniform1i(userData->samplerLoc, 0); //采样器使用纹理单元0

   // Load the MVP matrix 设置MVP矩阵
   glUniformMatrix4fv ( userData->mvpLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvpMatrix.m[0][0] );
   // Load the light direction 设置光线方向
   glUniform3f ( userData->lightDirectionLoc, 0.86f, 0.14f, 0.49f );
   //printf("   %d \n", userData->numIndices);// 237606
   // Draw the grid 渲染
   glDrawElements ( GL_TRIANGLES, userData->numIndices, GL_UNSIGNED_INT, ( const void * ) NULL );
}

///
// Cleanup
//
void Shutdown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   glDeleteBuffers ( 1, &userData->positionVBO );
   glDeleteBuffers ( 1, &userData->indicesIBO );

   // Delete program object
   glDeleteProgram ( userData->programObject );
}


int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "TerrainRendering", 640, 480, ES_WINDOW_RGB | ES_WINDOW_DEPTH );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterShutdownFunc ( esContext, Shutdown );
   esRegisterDrawFunc ( esContext, Draw );

   return GL_TRUE;
}

