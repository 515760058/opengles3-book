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
// Shadows.c
//
//    Demonstrates shadow rendering with depth texture and 6x6 PCF
// 使用深度纹理和6*6PCF来渲染阴影
#include <stdlib.h>
#include <math.h>
#include "esUtil.h"

#define POSITION_LOC    0
#define COLOR_LOC       1

typedef struct
{
   // Handle to a program object
   GLuint sceneProgramObject;
   GLuint shadowMapProgramObject;

   // Uniform locations
   GLint  sceneMvpLoc;
   GLint  shadowMapMvpLoc;
   GLint  sceneMvpLightLoc;
   GLint  shadowMapMvpLightLoc;

   // Sampler location
   GLint shadowMapSamplerLoc;

   // shadow map Texture handle
   GLuint shadowMapTextureId;
   GLuint shadowMapBufferId;
   GLuint shadowMapTextureWidth;
   GLuint shadowMapTextureHeight;

   // VBOs of the model
   GLuint groundPositionVBO;
   GLuint groundIndicesIBO;
   GLuint cubePositionVBO;
   GLuint cubeIndicesIBO;
   
   // Number of indices
   int    groundNumIndices;
   int    cubeNumIndices;

   // dimension of grid
   int    groundGridSize;

   // MVP matrices
   ESMatrix  groundMvpMatrix;
   ESMatrix  groundMvpLightMatrix;
   ESMatrix  cubeMvpMatrix;
   ESMatrix  cubeMvpLightMatrix;

   float eyePosition[3];//视角位置
   float lightPosition[3];//光源的位置
} UserData;

///
// Initialize the MVP matrix
// 初始化MVP矩阵
int InitMVP ( ESContext *esContext )
{
   ESMatrix perspective;
   ESMatrix ortho;
   ESMatrix modelview;
   ESMatrix model;
   ESMatrix view;
   float    aspect;
   UserData *userData = esContext->userData;
   
   // Compute the window aspect ratio
   aspect = (GLfloat) esContext->width / (GLfloat) esContext->height;
   
   // Generate a perspective matrix with a 45 degree FOV for the scene rendering 投影矩阵
   esMatrixLoadIdentity ( &perspective );
   esPerspective ( &perspective, 45.0f, aspect, 0.1f, 100.0f );

   // Generate an orthographic projection matrix for the shadow map rendering 正交投影矩阵
   esMatrixLoadIdentity ( &ortho );
   esOrtho ( &ortho, -10, 10, -10, 10, -30, 30 );

   // GROUND
   // Generate a model view matrix to rotate/translate the ground
   esMatrixLoadIdentity ( &model );//单位矩阵
   // Center the ground
   esTranslate ( &model, -2.0f, -2.0f, 0.0f );//平移
   esScale ( &model, 10.0f, 10.0f, 10.0f );//缩放
   esRotate ( &model, 90.0f, 1.0f, 0.0f, 0.0f );//旋转

   // create view matrix transformation from the eye position
   esMatrixLookAt ( &view, 
                    userData->eyePosition[0], userData->eyePosition[1], userData->eyePosition[2],
                    0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f );
   // MV = M * V
   esMatrixMultiply ( &modelview, &model, &view );

   // Compute the final ground MVP for the scene rendering by multiplying the 
   // modelview and perspective matrices together  MVP = MV * P
   esMatrixMultiply ( &userData->groundMvpMatrix, &modelview, &perspective );

   // create view matrix transformation from the light position
   esMatrixLookAt ( &view, 
                    userData->lightPosition[0], userData->lightPosition[1], userData->lightPosition[2],
                    0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f );

   esMatrixMultiply ( &modelview, &model, &view );

   // Compute the final ground MVP for the shadow map rendering by multiplying the 
   // modelview and ortho matrices together
   esMatrixMultiply ( &userData->groundMvpLightMatrix, &modelview, &ortho );

   // CUBE
   // position the cube
   esMatrixLoadIdentity ( &model );
   esTranslate ( &model, 5.0f, -0.4f, -3.0f );
   esScale ( &model, 1.0f, 2.5f, 1.0f );
   esRotate ( &model, -15.0f, 0.0f, 1.0f, 0.0f );

   // create view matrix transformation from the eye position
   esMatrixLookAt ( &view, 
                    userData->eyePosition[0], userData->eyePosition[1], userData->eyePosition[2],
                    0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f );

   esMatrixMultiply ( &modelview, &model, &view );
   
   // Compute the final cube MVP for scene rendering by multiplying the 
   // modelview and perspective matrices together
   esMatrixMultiply ( &userData->cubeMvpMatrix, &modelview, &perspective );

   // create view matrix transformation from the light position
   esMatrixLookAt ( &view, 
                    userData->lightPosition[0], userData->lightPosition[1], userData->lightPosition[2],
                    0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f );

   esMatrixMultiply ( &modelview, &model, &view );
   
   // Compute the final cube MVP for shadow map rendering by multiplying the 
   // modelview and ortho matrices together
   esMatrixMultiply ( &userData->cubeMvpLightMatrix, &modelview, &ortho );

   return TRUE;
}

//初始化深度纹理，并设为fbo的attachment
int InitShadowMap ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLenum none = GL_NONE;
   GLint defaultFramebuffer = 0;

   // use 1K by 1K texture for shadow map
   userData->shadowMapTextureWidth = userData->shadowMapTextureHeight = 1024;

   glGenTextures ( 1, &userData->shadowMapTextureId );
   glBindTexture ( GL_TEXTURE_2D, userData->shadowMapTextureId );
   //申请GPU内存
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, userData->shadowMapTextureWidth, userData->shadowMapTextureHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
   //过滤模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   //包装模式
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );
        
   // Setup hardware comparison
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
   //绑定到默认纹理点
   glBindTexture ( GL_TEXTURE_2D, 0 );

   glGetIntegerv ( GL_FRAMEBUFFER_BINDING, &defaultFramebuffer );

   // setup fbo
   glGenFramebuffers ( 1, &userData->shadowMapBufferId );
   glBindFramebuffer ( GL_FRAMEBUFFER, userData->shadowMapBufferId );
   //片段颜色要写入的缓冲区，（没有使用颜色渲染）
   glDrawBuffers ( 1, &none );
   //纹理作为fbo的color attachment
   glFramebufferTexture2D ( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, userData->shadowMapTextureId, 0 );

   glActiveTexture ( GL_TEXTURE0 );//激活纹理单元0
   glBindTexture ( GL_TEXTURE_2D, userData->shadowMapTextureId );//把纹理绑定到纹理单元0
 
   if ( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus ( GL_FRAMEBUFFER ) )
   {
      return FALSE;
   }
   //还原状态，设置为默认的fbo
   glBindFramebuffer ( GL_FRAMEBUFFER, defaultFramebuffer );

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
   //第一遍渲染：从光源的角度渲染场景，将片段的深度值记录在一个深度纹理中
   const char vShadowMapShaderStr[] =  
      "#version 300 es                                  \n"
      "uniform mat4 u_mvpLightMatrix;                   \n"
      "layout(location = 0) in vec4 a_position;         \n"
      "out vec4 v_color;                                \n"
      "void main()                                      \n"
      "{                                                \n"
      "   gl_Position = u_mvpLightMatrix * a_position;  \n"
      "}                                                \n";
   
   const char fShadowMapShaderStr[] =  
      "#version 300 es                                  \n"
      "precision lowp float;                            \n"
      "void main()                                      \n"
      "{                                                \n"//没有输出片段颜色，只要在深度纹理中记录片段的深度值
      "}                                                \n";
   //第二遍渲染：从眼睛位置的角度渲染场景，到fbo0中
    const char vSceneShaderStr[] =  
      "#version 300 es                                   \n"
      "uniform mat4 u_mvpMatrix;                         \n"//眼睛位置MVP
      "uniform mat4 u_mvpLightMatrix;                    \n"//光源位置MVP
      "layout(location = 0) in vec4 a_position;          \n"
      "layout(location = 1) in vec4 a_color;             \n"
      "out vec4 v_color;                                 \n"
      "out vec4 v_shadowCoord;                           \n"
      "void main()                                       \n"
      "{                                                 \n"
      "   v_color = a_color;                             \n"
      "   gl_Position = u_mvpMatrix * a_position;        \n"
      "   v_shadowCoord = u_mvpLightMatrix * a_position; \n"//经过光源MVP的转换，获取片段在深度纹理中的坐标
      "                                                  \n"
      "   // transform from [-1,1] to [0,1];             \n"
      "   v_shadowCoord = v_shadowCoord * 0.5 + 0.5;     \n"//转化为纹理坐标
      "}                                                 \n";
   
   const char fSceneShaderStr[] =  
      "#version 300 es                                                \n"
      "precision lowp float;                                          \n"
      "uniform lowp sampler2DShadow s_shadowMap;                      \n"
      "in vec4 v_color;                                               \n"
      "in vec4 v_shadowCoord;                                         \n"
      "layout(location = 0) out vec4 outColor;                        \n"
      "                                                               \n"
      "float lookup ( float x, float y )                              \n"//根据纹理坐标，判断该片段是否在阴影中
      "{                                                              \n"
      "   float pixelSize = 0.002; // 1/500                           \n"
      "   vec4 offset = vec4 ( x * pixelSize * v_shadowCoord.w,       \n"
      "                        y * pixelSize * v_shadowCoord.w,       \n"
      "                        -0.005 * v_shadowCoord.w, 0.0 );       \n"
      "   return textureProj ( s_shadowMap, v_shadowCoord + offset ); \n"//片段在阴影中，采样结果为0。不在阴影中，采样结果为1.
      "}                                                              \n"
      "                                                               \n"
      "void main()                                                    \n"
      "{                                                              \n"
      "   // 3x3 kernel with 4 taps per sample, effectively 6x6 PCF   \n"
      "   float sum = 0.0;                                            \n"
      "   float x, y;                                                 \n"
      "   for ( x = -2.0; x <= 2.0; x += 2.0 )                        \n"
      "      for ( y = -2.0; y <= 2.0; y += 2.0 )                     \n"
      "         sum += lookup ( x, y );                               \n"//9个点的采样
      "                                                               \n"
      "   // divide sum by 9.0                                        \n"
      "   sum = sum * 0.11;                                           \n"//9个点的平均
      "   outColor = v_color * sum;                                   \n"//获得最终的颜色
      "}                                                              \n";

   // Load the shaders and get a linked program object
   userData->shadowMapProgramObject = esLoadProgram ( vShadowMapShaderStr, fShadowMapShaderStr );//渲染fbo深度纹理的程序
   userData->sceneProgramObject = esLoadProgram ( vSceneShaderStr, fSceneShaderStr );//渲染最终场景的程序

   // Get the uniform locations 获得统一变量的location
   userData->sceneMvpLoc = glGetUniformLocation ( userData->sceneProgramObject, "u_mvpMatrix" );
   userData->shadowMapMvpLoc = glGetUniformLocation ( userData->shadowMapProgramObject, "u_mvpMatrix" );//着色器中没有这个mvp矩阵啊？
   userData->sceneMvpLightLoc = glGetUniformLocation ( userData->sceneProgramObject, "u_mvpLightMatrix" );
   userData->shadowMapMvpLightLoc = glGetUniformLocation ( userData->shadowMapProgramObject, "u_mvpLightMatrix" );

   // Get the sampler location 获得采样器的location
   userData->shadowMapSamplerLoc = glGetUniformLocation ( userData->sceneProgramObject, "s_shadowMap" );

   // Generate the vertex and index data for the ground
   userData->groundGridSize = 3;
   userData->groundNumIndices = esGenSquareGrid( userData->groundGridSize, &positions, &indices );//地面的顶点位置和索引

   // Index buffer object for the ground model
   glGenBuffers ( 1, &userData->groundIndicesIBO );
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, userData->groundIndicesIBO );
   //申请GPU内存  上传地面顶点的索引数据
   glBufferData ( GL_ELEMENT_ARRAY_BUFFER, userData->groundNumIndices * sizeof( GLuint ), indices, GL_STATIC_DRAW );
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
   free( indices );

   // Position VBO for ground model
   glGenBuffers ( 1, &userData->groundPositionVBO );
   glBindBuffer ( GL_ARRAY_BUFFER, userData->groundPositionVBO );
   //申请GPU内存  上传地面顶点的位置数据
   glBufferData ( GL_ARRAY_BUFFER, userData->groundGridSize * userData->groundGridSize * sizeof( GLfloat ) * 3,  positions, GL_STATIC_DRAW );
   free( positions );

   // Generate the vertex and index date for the cube model  生成立方体模型的顶点位置和索引数据
   userData->cubeNumIndices = esGenCube ( 1.0f, &positions, NULL, NULL, &indices );

   // Index buffer object for cube model
   glGenBuffers ( 1, &userData->cubeIndicesIBO );
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, userData->cubeIndicesIBO );
   //申请GPU内存 上传立方体模型的顶点索引数据
   glBufferData ( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * userData->cubeNumIndices, indices, GL_STATIC_DRAW );
   glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
   free( indices );

   // Position VBO for cube model
   glGenBuffers ( 1, &userData->cubePositionVBO );
   glBindBuffer ( GL_ARRAY_BUFFER, userData->cubePositionVBO );
   //申请GPU内存 上传立方体模型的顶点位置数据
   glBufferData ( GL_ARRAY_BUFFER, 24 * sizeof( GLfloat ) * 3, positions, GL_STATIC_DRAW );
   free( positions );

   // setup transformation matrices
   userData->eyePosition[0] = -5.0f;//眼睛位置
   userData->eyePosition[1] = 3.0f;
   userData->eyePosition[2] = 5.0f;
   userData->lightPosition[0] = 10.0f;//光源位置
   userData->lightPosition[1] = 5.0f;
   userData->lightPosition[2] = 2.0f;
   
   // create depth texture  初始化深度纹理，并设为fbo的attachment
   if ( !InitShadowMap( esContext ) )
   {
      return FALSE;
   }

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );//白色背景

   // disable culling 面剔除
   glDisable ( GL_CULL_FACE );

   // enable depth test 使能深度测试
   glEnable ( GL_DEPTH_TEST );

   return TRUE;
}

///
// Draw the model
//
void DrawScene ( ESContext *esContext, 
                 GLint mvpLoc, 
                 GLint mvpLightLoc )
{
   UserData *userData = esContext->userData;
 
   // Draw the ground 渲染地面
   // Load the vertex position
   glBindBuffer ( GL_ARRAY_BUFFER, userData->groundPositionVBO );
   //指定缓冲区中地面顶点位置数据的格式，和数据偏移
   glVertexAttribPointer ( POSITION_LOC, 3, GL_FLOAT,  GL_FALSE, 3 * sizeof(GLfloat), (const void*)NULL );
   glEnableVertexAttribArray ( POSITION_LOC );   //使能顶点数组

   // Bind the index buffer  地面顶点索引数据
   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, userData->groundIndicesIBO );

   // Load the MVP matrix for the ground model 加载地面的MVP矩阵
   glUniformMatrix4fv ( mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->groundMvpMatrix.m[0][0] );
   glUniformMatrix4fv ( mvpLightLoc, 1, GL_FALSE, (GLfloat*) &userData->groundMvpLightMatrix.m[0][0] );

   // Set the ground color to light gray 设置地面的颜色常量属性（灰色）
   glVertexAttrib4f ( COLOR_LOC, 0.9f, 0.9f, 0.9f, 1.0f );
   //渲染地面
   glDrawElements ( GL_TRIANGLES, userData->groundNumIndices, GL_UNSIGNED_INT, (const void*)NULL );

   // Draw the cube 渲染立方体
   // Load the vertex position
   glBindBuffer( GL_ARRAY_BUFFER, userData->cubePositionVBO );
   //指定缓冲区中立方体顶点位置数据的格式，和数据偏移
   glVertexAttribPointer ( POSITION_LOC, 3, GL_FLOAT,  GL_FALSE, 3 * sizeof(GLfloat), (const void*)NULL );
   glEnableVertexAttribArray ( POSITION_LOC );  //使能顶点数组 

   // Bind the index buffer  立方体顶点索引数据
   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, userData->cubeIndicesIBO );

   // Load the MVP matrix for the cube model  加载立方体的MVP矩阵
   glUniformMatrix4fv ( mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->cubeMvpMatrix.m[0][0] );
   glUniformMatrix4fv ( mvpLightLoc, 1, GL_FALSE, (GLfloat*) &userData->cubeMvpLightMatrix.m[0][0] );

   // Set the cube color to red  设置立方体的颜色常量属性（红色）
   glVertexAttrib4f ( COLOR_LOC, 1.0f, 0.0f, 0.0f, 1.0f );
   //渲染立方体
   glDrawElements ( GL_TRIANGLES, userData->cubeNumIndices, GL_UNSIGNED_INT, (const void*)NULL );
}

void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLint defaultFramebuffer = 0;

   // Initialize matrices
   InitMVP ( esContext );
   //查询默认的fbo
   glGetIntegerv ( GL_FRAMEBUFFER_BINDING, &defaultFramebuffer );

   // FIRST PASS: Render the scene from light position to generate the shadow map texture
   glBindFramebuffer ( GL_FRAMEBUFFER, userData->shadowMapBufferId );

   // Set the viewport 视口（深度纹理的大小）
   glViewport ( 0, 0, userData->shadowMapTextureWidth, userData->shadowMapTextureHeight );

   // clear depth buffer 清楚深度缓冲区
   glClear( GL_DEPTH_BUFFER_BIT );

   // disable color rendering, only write to depth buffer 禁用颜色渲染
   glColorMask ( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

   // reduce shadow rendering artifact 多边形偏移（增大写入纹理的深度值，以免因为精度问题而造成阴影的渲染伪像）
   glEnable ( GL_POLYGON_OFFSET_FILL );
   glPolygonOffset( 5.0f, 100.0f );
   //第一遍渲染：从光源的角度渲染场景，将片段的深度值记录在一个纹理中
   glUseProgram ( userData->shadowMapProgramObject );
   DrawScene ( esContext, userData->shadowMapMvpLoc, userData->shadowMapMvpLightLoc );

   glDisable( GL_POLYGON_OFFSET_FILL );//禁用多边形偏移

   // SECOND PASS: Render the scene from eye location using the shadow map texture created in the first pass
   glBindFramebuffer ( GL_FRAMEBUFFER, defaultFramebuffer );//默认的fbo
   glColorMask ( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );//使能颜色渲染

   // Set the viewport 视口（画面的大小）
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   // Clear the color and depth buffers
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );//清除fbo的颜色缓冲区和深度缓冲区
   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );//背景白色

   // Use the scene program object
   glUseProgram ( userData->sceneProgramObject );

   // Bind the shadow map texture
   glActiveTexture ( GL_TEXTURE0 );//激活纹理单元0
   glBindTexture ( GL_TEXTURE_2D, userData->shadowMapTextureId );//把深度纹理绑定到纹理单元0
   // Set the sampler texture unit to 0 设置采样器使用纹理单元0（深度纹理）
   glUniform1i ( userData->shadowMapSamplerLoc, 0 );
   //第二遍渲染：从眼睛位置的角度渲染场景，到fbo0中
   DrawScene ( esContext, userData->sceneMvpLoc, userData->sceneMvpLightLoc );
}

///
// Cleanup
//
void Shutdown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   glDeleteBuffers( 1, &userData->groundPositionVBO );
   glDeleteBuffers( 1, &userData->groundIndicesIBO );

   glDeleteBuffers( 1, &userData->cubePositionVBO );
   glDeleteBuffers( 1, &userData->cubeIndicesIBO );
   
   // Delete shadow map
   glBindFramebuffer ( GL_FRAMEBUFFER, userData->shadowMapBufferId );
   glFramebufferTexture2D ( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0 );
   glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
   glDeleteFramebuffers ( 1, &userData->shadowMapBufferId );
   glDeleteTextures ( 1, &userData->shadowMapTextureId );

   // Delete program object
   glDeleteProgram ( userData->sceneProgramObject );
   glDeleteProgram ( userData->shadowMapProgramObject );
}

int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof( UserData ) );

   esCreateWindow ( esContext, "Shadow Rendering", 500, 500, ES_WINDOW_RGB | ES_WINDOW_DEPTH );
   
   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }
   //注册回调函数
   esRegisterShutdownFunc ( esContext, Shutdown );
   esRegisterDrawFunc ( esContext, Draw );
   
   return GL_TRUE;
}

