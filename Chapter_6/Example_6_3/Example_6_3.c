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
// Example_6_3.c
//
//    This example demonstrates using client-side vertex arrays
//    and a constant vertex attribute
//  ��ʹ��glVertexAttrib4fv���ó����������ԣ�
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;


///
// Initialize the program object
//
int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   const char vShaderStr[] =
      "#version 300 es                            \n"
      "layout(location = 0) in vec4 a_position;   \n"//�����λ������ location=0
      "layout(location = 1) in vec4 a_color;      \n"//�������ɫ���� location=1
      "out vec4 v_color;                          \n"
      "void main()                                \n"
      "{                                          \n"
      "    v_color = a_color;                     \n"
      "    gl_Position = a_position;              \n"
      "}";


   const char fShaderStr[] =
      "#version 300 es            \n"
      "precision mediump float;   \n"
      "in vec4 v_color;           \n"//�����Ƭ����ɫ v_colorҪ�Ͷ�����ɫ�����v_color����һ��
      "out vec4 o_fragColor;      \n"//�����Ƭ����ɫ
      "void main()                \n"
      "{                          \n"
      "    o_fragColor = v_color; \n"
      "}" ;

   GLuint programObject;

   // Create the program object
   programObject = esLoadProgram ( vShaderStr, fShaderStr );

   if ( programObject == 0 )
   {
      return GL_FALSE;
   }

   // Store the program object
   userData->programObject = programObject;

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
   return GL_TRUE;
}

///
// Draw a triangle using the program object created in Init()
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLfloat color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };//��ɫ
   // 3 vertices, with (x,y,z) per-vertex
   GLfloat vertexPos[3 * 3] =
   {
      0.0f,  0.5f, 0.0f, // v0
      -0.5f, -0.5f, 0.0f, // v1
      0.5f, -0.5f, 0.0f  // v2
   };

   glViewport ( 0, 0, esContext->width, esContext->height );

   glClear ( GL_COLOR_BUFFER_BIT );

   glUseProgram ( userData->programObject );
   //�������飨��ʹ��VBO��
   glVertexAttribPointer ( 0, 3, GL_FLOAT, GL_FALSE, 0, vertexPos );
   glEnableVertexAttribArray ( 0 );//���ö�����������
   //���� �����������ԣ�����һ��ͼԪ�����ж��㶼��ͬ
   glVertexAttrib4fv ( 1, color );//������ɫ������ɫ����a_color �� location=1


   glDrawArrays ( GL_TRIANGLES, 0, 3 );

   glDisableVertexAttribArray ( 0 );//���ö�����������
}

void Shutdown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   glDeleteProgram ( userData->programObject );
}

int esMain ( ESContext *esContext )
{
   // ���Զ���Ľṹ�����ڴ�
   esContext->userData = malloc ( sizeof ( UserData ) );
   //ͨ��EGL��������
   esCreateWindow ( esContext, "Example 6-3", 320, 240, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterShutdownFunc ( esContext, Shutdown );
   //ע���ͼ�ص��������˳�esMain֮�󣬿�ܽ�ѭ������ע���Draw��Update��ֱ�����ڹر�
   esRegisterDrawFunc ( esContext, Draw );

   return GL_TRUE;
}
