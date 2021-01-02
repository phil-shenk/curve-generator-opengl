#ifndef _INIT_SHADER_H_
#define _INIT_SHADER_H_

#ifdef __APPLE__  // include Mac OS X verions of headers
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else // non-Mac OS X operating systems
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#endif  // __APPLE__


struct Shader
{
    const char*  filename;
    GLenum       type;
    GLchar*      source;
};

GLuint initShader(const char* vertexShaderFile, const char* fragmentShaderFile);

#endif
