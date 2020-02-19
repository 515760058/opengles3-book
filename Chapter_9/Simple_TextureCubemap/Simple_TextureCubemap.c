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
// Simple_TextureCubemap.c
//
//    This is a simple example that draws a sphere with a cubemap image applied.
//  �޸ģ��Ѿ�̬������Ч�����ĳɶ�̬��ת��������Ч��
#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Sampler location
   GLint samplerLoc;

   // Texture handle
   GLuint textureId;

   // Vertex data
   int      numIndices;//����������
   GLfloat *vertices;//����λ������ָ��
   GLfloat *normals;//���㷨������ָ��
   GLuint  *indices;//������������ָ��
   //���ӵ�
   GLint  mvpLoc;// Uniform locations
   GLfloat   angle;// Rotation angle
   ESMatrix  mvpMatrix;// MVP matrix
} UserData;

///
// Create a simple cubemap with a 1x1 face with a different color for each face
// ��������ͼ������Ϊ����ͼÿ�����ϴ���������
GLuint CreateSimpleTextureCubemap( )
{
   GLuint textureId;
   // Six 1x1 RGB faces
   GLubyte cubePixels[6][3] =
   {
      // Face 0 - Red
      255, 0, 0,
      // Face 1 - Green,
      0, 255, 0,
      // Face 2 - Blue
      0, 0, 255,
      // Face 3 - Yellow
      255, 255, 0,
      // Face 4 - Purple
      255, 0, 255,
      // Face 5 - White
      255, 255, 255
   };

   // Generate a texture object
   glGenTextures ( 1, &textureId );

   // Bind the texture object ������ͼ����
   glBindTexture ( GL_TEXTURE_CUBE_MAP, textureId );
   // �����ڴ棬�ϴ�����ͼ6�������������
   // Load the cube face - Positive X 
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[0] );

   // Load the cube face - Negative X
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[1] );

   // Load the cube face - Positive Y
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[2] );

   // Load the cube face - Negative Y
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[3] );

   // Load the cube face - Positive Z
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[4] );

   // Load the cube face - Negative Z
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[5] );

   // Set the filtering mode ����ģʽ
   glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return textureId;

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
      "layout(location = 1) in vec3 a_normal;     \n"
      "uniform mat4 u_mvpMatrix;                  \n"
      "out vec3 v_normal;                         \n"
      "void main()                                \n"
      "{                                          \n"
      "   gl_Position = u_mvpMatrix * a_position; \n"
      "   v_normal = a_normal;                    \n"
      "}                                          \n";

   char fShaderStr[] =
      "#version 300 es                                     \n"
      "precision mediump float;                            \n"
      "in vec3 v_normal;                                   \n"
      "layout(location = 0) out vec4 outColor;             \n"
      "uniform samplerCube s_texture;                      \n"
      "void main()                                         \n"
      "{                                                   \n"
      "   outColor = texture( s_texture, v_normal );       \n"
      "}                                                   \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the sampler locations
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   userData->mvpLoc = glGetUniformLocation(userData->programObject, "u_mvpMatrix");
   userData->angle = 45.0f;   // Starting rotation angle for the cube
   // Load the texture
   userData->textureId = CreateSimpleTextureCubemap ();

   // Generate the vertex data ����Ķ�����Ϣ
   //userData->numIndices = esGenSphere ( 20, 0.75f, &userData->vertices, &userData->normals, NULL, &userData->indices );
   // �Լ��޸ĵģ�����������
   userData->numIndices = esGenCube(1.0, &userData->vertices, &userData->normals, NULL, &userData->indices);

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
   return TRUE;
}

///
// Update MVP matrix based on time
// ����ʱ�䣬������ת->model_view����-->MVP����
void Update(ESContext* esContext, float deltaTime)
{
    UserData* userData = esContext->userData;
    ESMatrix perspective;
    ESMatrix modelview;
    float    aspect;

    // Compute a rotation angle based on time to rotate the cube ����ʱ�������ת�Ƕ�
    userData->angle += (deltaTime * 40.0f);
    if (userData->angle >= 360.0f)
    {
        userData->angle -= 360.0f;
    }
    // ����project����
    // Compute the window aspect ratio
    aspect = (GLfloat)esContext->width / (GLfloat)esContext->height;
    // Generate a perspective matrix with a 60 degree FOV
    esMatrixLoadIdentity(&perspective);
    esPerspective(&perspective, 60.0f, aspect, 1.0f, 20.0f);

    // ����medel view����
    // Generate a model view matrix to rotate/translate the cube
    esMatrixLoadIdentity(&modelview);//��λ����
    // Translate away from the viewer  ƽ��
    esTranslate(&modelview, 0.0, 0.0, -2.0);
    // Rotate the cube ��ת
    esRotate(&modelview, userData->angle, 1.0, 0.0, 1.0);

    // Compute the final MVP by multiplying the modevleiw and perspective matrices together
    // ����mvp = model_view * perspective
    esMatrixMultiply(&userData->mvpMatrix, &modelview, &perspective);
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
   //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //���޳�
   glCullFace ( GL_BACK );
   glEnable ( GL_CULL_FACE );

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load the vertex position  ָ������λ�õ����ݸ�ʽ�������ݵ�ַ
   glVertexAttribPointer ( 0, 3, GL_FLOAT,
                           GL_FALSE, 0, userData->vertices );
   // Load the normal ָ�����㷨�ߵ����ݸ�ʽ�������ݵ�ַ
   glVertexAttribPointer ( 1, 3, GL_FLOAT,
                           GL_FALSE, 0, userData->normals );
   //ʹ�ܶ�������
   glEnableVertexAttribArray ( 0 );
   glEnableVertexAttribArray ( 1 );

   // Load the MVP matrix  ��������ɫ���е�ͳһ����mvp����ֵ
   glUniformMatrix4fv(userData->mvpLoc, 1, GL_FALSE, (GLfloat*)&userData->mvpMatrix.m[0][0]);

   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );//��������Ԫ0
   glBindTexture ( GL_TEXTURE_CUBE_MAP, userData->textureId );//������󶨵�����Ԫ0

   // Set the sampler texture unit to 0
   glUniform1i ( userData->samplerLoc, 0 );//���ò�����ʹ������Ԫ0

   glDrawElements ( GL_TRIANGLES, userData->numIndices,
                    GL_UNSIGNED_INT, userData->indices );
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

   free ( userData->vertices );
   free ( userData->normals );
}


int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "Simple Texture Cubemap", 320, 240, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }
   esRegisterUpdateFunc(esContext, Update);
   esRegisterDrawFunc ( esContext, Draw );
   esRegisterShutdownFunc ( esContext, ShutDown );

   return GL_TRUE;
}
