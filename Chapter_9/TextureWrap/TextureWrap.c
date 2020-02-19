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
// TextureWrap.c
//
//    This is an example that demonstrates the three texture
//    wrap modes available on 2D textures
//  ����İ�װģʽ(GL_TEXTURE_WRAP_S   GL_TEXTURE_WRAP_T)����������Χ�Ĳ�������
#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Sampler location
   GLint samplerLoc;

   // Offset location
   GLint offsetLoc;

   // Texture handle
   GLuint textureId;

} UserData;

///
//  Generate an RGB8 checkerboard image
// ����һ��ͼ��
GLubyte *GenCheckImage ( int width, int height, int checkSize )
{
   int x, y;
   GLubyte *pixels = malloc ( width * height * 3 );

   if ( pixels == NULL )
   {
      return NULL;
   }

   for ( y = 0; y < height; y++ )
      for ( x = 0; x < width; x++ )
      {
         GLubyte rColor = 0;
         GLubyte bColor = 0;

         if ( ( x / checkSize ) % 2 == 0 )
         {
            rColor = 255 * ( ( y / checkSize ) % 2 );
            bColor = 255 * ( 1 - ( ( y / checkSize ) % 2 ) );
         }
         else
         {
            bColor = 255 * ( ( y / checkSize ) % 2 );
            rColor = 255 * ( 1 - ( ( y / checkSize ) % 2 ) );
         }

         pixels[ ( y * width + x ) * 3] = rColor;
         pixels[ ( y * width + x ) * 3 + 1] = 0;
         pixels[ ( y * width + x ) * 3 + 2] = bColor;
      }

   return pixels;
}

///
// Create a mipmapped 2D texture image
//  ����һ��ͼ�񣬲��ϴ��������ù���ģʽ
GLuint CreateTexture2D( )
{
   // Texture object handle
   GLuint textureId;
   int    width = 512;// 256;
   int    height = 512;// 256;
   GLubyte *pixels;

   pixels = GenCheckImage ( width, height, 64 );

   if ( pixels == NULL )
   {
      return 0;
   }

   // Generate a texture object
   glGenTextures ( 1, &textureId );

   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );

   // Load mipmap level 0
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height,
                  0, GL_RGB, GL_UNSIGNED_BYTE, pixels );

   // Set the filtering mode ����ģʽ
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

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
      "uniform float u_offset;                    \n"//ͳһ������ƫ��
      "layout(location = 0) in vec4 a_position;   \n"
      "layout(location = 1) in vec2 a_texCoord;   \n"
      "out vec2 v_texCoord;                       \n"
      "void main()                                \n"
      "{                                          \n"
      "   gl_Position = a_position;               \n"
      "   gl_Position.x += u_offset;              \n"//x����ƫ��
      "   v_texCoord = a_texCoord;                \n"//��������
      "}                                          \n";

   char fShaderStr[] =
      "#version 300 es                                     \n"
      "precision mediump float;                            \n"
      "in vec2 v_texCoord;                                 \n"
      "layout(location = 0) out vec4 outColor;             \n"
      "uniform sampler2D s_texture;                        \n"//ͳһ���� ������
      "void main()                                         \n"
      "{                                                   \n"
      "   outColor = texture( s_texture, v_texCoord );     \n"//�����������꣬�Ӳ�������Ӧ������Ԫ����Ӧ��������ȡ��ɫ����
      "}                                                   \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   // Get the offset location
   userData->offsetLoc = glGetUniformLocation ( userData->programObject, "u_offset" );

   // Load the texture
   userData->textureId = CreateTexture2D ();

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
   return TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   //�����λ�����ԣ���ά,���һά��z��������й�ϵ����1.0�ĳ���0.8��ʵ��Ч����������������������
   GLfloat vVertices[] = { -0.3f,  0.3f, 0.0f, 1.0f,  // Position 0
                           -1.0f,  -1.0f,              // TexCoord 0
                           -0.3f, -0.3f, 0.0f, 1.0f, // Position 1
                           -1.0f,  2.0f,              // TexCoord 1
                           0.3f, -0.3f, 0.0f, 0.8f, // Position 2      0.8 �Ӿ��ϸ������Լ�
                           2.0f,  2.0f,              // TexCoord 2
                           0.3f,  0.3f, 0.0f, 1.0f,  // Position 3
                           2.0f,  -1.0f               // TexCoord 3
                         };
   GLushort indices[] = { 0, 1, 2, 0, 2, 3 };//��������

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load the vertex position ָ������λ�õ����ݸ�ʽ���������ݵ�ַ
   glVertexAttribPointer ( 0, 4, GL_FLOAT,
                           GL_FALSE, 6 * sizeof ( GLfloat ), vVertices );
   // Load the texture coordinate ָ�������������������ݸ�ʽ���������ݵ�ַ
   glVertexAttribPointer ( 1, 2, GL_FLOAT,
                           GL_FALSE, 6 * sizeof ( GLfloat ), &vVertices[4] );
   //ʹ�ܶ�������
   glEnableVertexAttribArray ( 0 );
   glEnableVertexAttribArray ( 1 );

   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );//��������Ԫ0
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );//�� ���� �󶨵� ����Ԫ0

   // Set the sampler texture unit to 0 ���ò�����ʹ������Ԫ0
   glUniform1i ( userData->samplerLoc, 0 );

   // Draw quad with repeat wrap mode ���ð�װģʽ
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glUniform1f ( userData->offsetLoc, -0.7f );//����
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

   // Draw quad with clamp to edge wrap mode ���ð�װģʽ
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glUniform1f ( userData->offsetLoc, 0.0f );//�м�
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

   // Draw quad with mirrored repeat ���ð�װģʽ
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
   glUniform1f ( userData->offsetLoc, 0.7f );//����
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
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

   esCreateWindow ( esContext, "TextureWrap", 640, 480, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterDrawFunc ( esContext, Draw );
   esRegisterShutdownFunc ( esContext, ShutDown );

   return GL_TRUE;
}
