// Code from the GLSL cookbook http://www.packtpub.com/article/opengl-glsl-4-building-c%20%20-shader-program-class

#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include "GL/glew.h"
#ifdef _WIN32
#include "GL/wglew.h"
#elif (!defined(__APPLE__))
#include "GL/glxew.h"
#endif

#include <string>
#include <vector>
using std::string;
#include <map>
#include <iostream>

#include <boost/format.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <MVRCore/StringUtils.H>
#include <glm/glm.hpp>
using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;
using glm::dmat3;


#include <stdexcept>

class GLSLProgramException : public std::runtime_error {
public:
        GLSLProgramException( const string & msg ) :
                std::runtime_error(msg) { }
};

namespace GLSLShader {
    enum GLSLShaderType {
        VERTEX, FRAGMENT, GEOMETRY,
        TESS_CONTROL, TESS_EVALUATION, COMPUTE
    };
};

class GLSLProgram
{
private:
    int  handle;
    bool linked;
    std::map<string, int> uniformLocations;

    int  getUniformLocation(const char * name );
    bool fileExists( const string & fileName );
    string getExtension( const char * fileName );

	static bool expandForPragmas(std::string& processedSource, const std::string& originalSource, std::map<std::string, std::string> &args);
	void addDefines(std::string& processedSource, const std::string&originalSource, std::map<std::string, std::string> &args);

    // Make these private in order to make the object non-copyable
    GLSLProgram( const GLSLProgram & other ) { }
    GLSLProgram & operator=( const GLSLProgram &other ) { return *this; }

public:
    GLSLProgram();
    ~GLSLProgram();

    void   compileShader( const char *fileName, std::map<std::string, std::string> &args );
    void   compileShader( const char * fileName, GLSLShader::GLSLShaderType type, std::map<std::string, std::string> &args );
    void   compileShader( const string & source, GLSLShader::GLSLShaderType type, std::map<std::string, std::string> &args, const char *fileName = NULL );
                          
    void   link();
    void   validate();
    void   use();

    int    getHandle();
    bool   isLinked();

    void   bindAttribLocation( GLuint location, const char * name);
    void   bindFragDataLocation( GLuint location, const char * name );


	// Note: doubles will be converted to floats for uploading.
    void   setUniform( const char *name, float x, float y, float z);
	void   setUniform( const char *name, double x, double y, double z);
    void   setUniform( const char *name, const dvec2 & v);
    void   setUniform( const char *name, const dvec3 & v);
    void   setUniform( const char *name, const dvec4 & v);
    void   setUniform( const char *name, const dmat4 & m);
    void   setUniform( const char *name, const dmat3 & m);
    void   setUniform( const char *name, float val );
	void   setUniform( const char *name, double val );
    void   setUniform( const char *name, int val );
    void   setUniform( const char *name, bool val );
    void   setUniform( const char *name, GLuint val );

    void   printActiveUniforms();
    void   printActiveUniformBlocks();
    void   printActiveAttribs();
    
    const char * getTypeString( GLenum type );
};

#endif // GLSLPROGRAM_H