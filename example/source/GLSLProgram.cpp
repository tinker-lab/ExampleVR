#include "glslprogram.h"

#include <fstream>
using std::ifstream;
using std::ios;

#include <sstream>
#include <sys/stat.h>

namespace GLSLShaderInfo {
        struct shader_file_extension {
            const char *ext;
            GLSLShader::GLSLShaderType type;
    };
    
    struct shader_file_extension extensions[] = 
    {
            {".vs", GLSLShader::VERTEX},
             {".vert", GLSLShader::VERTEX},
            {".gs", GLSLShader::GEOMETRY},
            {".geom", GLSLShader::GEOMETRY},
            {".tcs", GLSLShader::TESS_CONTROL},
            {".tes", GLSLShader::TESS_EVALUATION},
            {".fs", GLSLShader::FRAGMENT},
            {".frag", GLSLShader::FRAGMENT},
            {".cs", GLSLShader::COMPUTE}   
    };
}

GLSLProgram::GLSLProgram() : handle(0), linked(false) { }

GLSLProgram::~GLSLProgram() {
        if(handle == 0) return;

        // Query the number of attached shaders
        GLint numShaders = 0;
        glGetProgramiv(handle, GL_ATTACHED_SHADERS, &numShaders);

        // Get the shader names
        GLuint * shaderNames = new GLuint[numShaders];
        glGetAttachedShaders(handle, numShaders, NULL, shaderNames);

        // Delete the shaders
        for (int i = 0; i < numShaders; i++)
                glDeleteShader(shaderNames[i]);

        // Delete the program
        glDeleteProgram (handle);
        
        delete[] shaderNames;
}

void GLSLProgram::compileShader( const char * fileName, std::map<std::string, std::string> &args )
{
    int numExts = sizeof(GLSLShaderInfo::extensions) / sizeof(GLSLShaderInfo::shader_file_extension);
        
    // Check the file name's extension to determine the shader type
    string ext = getExtension( fileName );
    GLSLShader::GLSLShaderType type = GLSLShader::VERTEX;
    bool matchFound = false;
    for( int i = 0; i < numExts; i++ ) {
            if( ext == GLSLShaderInfo::extensions[i].ext ) {
                    matchFound = true;
                    type = GLSLShaderInfo::extensions[i].type;
                    break;
            }
    }
        
    // If we didn't find a match, throw an exception
    if( !matchFound ) {
            string msg = "Unrecognized extension: " + ext;
            throw GLSLProgramException(msg);
    }
        
    // Pass the discovered shader type along
    compileShader( fileName, type, args );
}

string GLSLProgram::getExtension( const char * name ) {
        string nameStr(name);
        
        size_t loc = nameStr.find_last_of('.');
        if( loc != string::npos ) {
                return nameStr.substr(loc, string::npos);
        }
        return "";
}

void GLSLProgram::compileShader( const char * fileName,  GLSLShader::GLSLShaderType type, std::map<std::string, std::string> &args )
{
    if( ! fileExists(fileName) )
    {
        string message = string("Shader: ") + fileName + " not found.";
        throw GLSLProgramException(message);
    }

    if( handle <= 0 ) {
        handle = glCreateProgram();
        if( handle == 0) {
            throw GLSLProgramException("Unable to create shader program.");
        }
    }

    ifstream inFile( fileName, ios::in );
    if( !inFile ) {
            string message = string("Unable to open: ") + fileName;
                throw GLSLProgramException(message);
    }

        // Get file contents
        std::stringstream code;
        code << inFile.rdbuf();
        inFile.close();

    compileShader(code.str(), type, args, fileName);
}

void GLSLProgram::compileShader( const string & source, GLSLShader::GLSLShaderType type, std::map<std::string, std::string> &args, const char * fileName )
{
	std::string processed, finalSource;
	expandForPragmas(processed, source, args);
	addDefines(finalSource, processed, args);

    if( handle <= 0 ) {
        handle = glCreateProgram();
        if( handle == 0) {
            throw GLSLProgramException("Unable to create shader program.");
        }
    }

    GLuint shaderHandle = 0;

    switch( type ) {
    case GLSLShader::VERTEX:
        shaderHandle = glCreateShader(GL_VERTEX_SHADER);
        break;
    case GLSLShader::FRAGMENT:
        shaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
        break;
    case GLSLShader::GEOMETRY:
        shaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
        break;
    case GLSLShader::TESS_CONTROL:
        shaderHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
        break;
    case GLSLShader::TESS_EVALUATION:
        shaderHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
        break;
    case GLSLShader::COMPUTE:
        shaderHandle = glCreateShader(GL_COMPUTE_SHADER);
        break;
    default:
        throw GLSLProgramException("Invalid shader type");
    }

    const char * c_code = finalSource.c_str();
    glShaderSource( shaderHandle, 1, &c_code, NULL );

    // Compile the shader
    glCompileShader(shaderHandle);

    // Check for errors
    int result;
    glGetShaderiv( shaderHandle, GL_COMPILE_STATUS, &result );
    if( GL_FALSE == result ) {
        // Compile failed, get log
        int length = 0;
        string logString;
        glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length );
        if( length > 0 ) {
            char * c_log = new char[length];
            int written = 0;
            glGetShaderInfoLog(shaderHandle, length, &written, c_log);
            logString = c_log;
            delete [] c_log;
        }
        string msg;
        if( fileName ) {
                msg = string(fileName) + ": shader compliation failed\n";
        } else {
                msg = "Shader compilation failed.\n";
        }
        msg += logString;

        throw GLSLProgramException(msg);
        
    } else {
        // Compile succeeded, attach shader
        glAttachShader(handle, shaderHandle);
    }
}

void GLSLProgram::link() throw(GLSLProgramException)
{
    if( linked ) return;
    if( handle <= 0 ) 
            throw GLSLProgramException("Program has not been compiled.");

    glLinkProgram(handle);

    int status = 0;
    glGetProgramiv( handle, GL_LINK_STATUS, &status);
    if( GL_FALSE == status ) {
        // Store log and return false
        int length = 0;
        string logString;

        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length );

        if( length > 0 ) {
            char * c_log = new char[length];
            int written = 0;
            glGetProgramInfoLog(handle, length, &written, c_log);
            logString = c_log;
            delete [] c_log;
        }

        throw GLSLProgramException(string("Program link failed:\n") + logString);
    } else {
            uniformLocations.clear();
        linked = true;
    }    
}

void GLSLProgram::use() throw(GLSLProgramException)
{
    if( handle <= 0 || (! linked) ) 
            throw GLSLProgramException("Shader has not been linked");
    glUseProgram( handle );
}

int GLSLProgram::getHandle()
{
    return handle;
}

bool GLSLProgram::isLinked()
{
    return linked;
}

void GLSLProgram::bindAttribLocation( GLuint location, const char * name)
{
    glBindAttribLocation(handle, location, name);
}

void GLSLProgram::bindFragDataLocation( GLuint location, const char * name )
{
    glBindFragDataLocation(handle, location, name);
}

void GLSLProgram::setUniform( const char *name, float x, float y, float z)
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform3f(loc,x,y,z);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, double x, double y, double z)
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform3f(loc,(float)x,(float)y,(float)z);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, const dvec3 & v)
{
    this->setUniform(name,v.x,v.y,v.z);
}

void GLSLProgram::setUniform( const char *name, const dvec4 & v)
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform4f(loc,(float)v.x, (float)v.y, (float)v.z, (float)v.w);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, const dvec2 & v)
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform2f(loc, (float)v.x, (float)v.y);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, const dmat4 & m)
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
		GLfloat matrix[16];
		for (int c = 0; c < 4; ++c) {
			for(int r = 0; r < 4; ++r) {
				matrix[c*4+r] = (float)m[c][r];
			}
		}
        glUniformMatrix4fv(loc, 1, GL_FALSE, &matrix[0]);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, const dmat3 & m)
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
		GLfloat matrix[9];
		for (int c = 0; c < 3; ++c) {
			for(int r = 0; r < 3; ++r) {
				matrix[c*3+r] = (float)m[c][r];
			}
		}
        glUniformMatrix3fv(loc, 1, GL_FALSE, &matrix[0]);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, float val )
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform1f(loc, val);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, double val )
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform1f(loc, (float)val);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, int val )
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform1i(loc, val);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, GLuint val )
{
  int loc = getUniformLocation(name);
  if( loc >=0 ) {
    glUniform1ui(loc, val);
  }
  else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::setUniform( const char *name, bool val )
{
    int loc = getUniformLocation(name);
    if( loc >= 0 ) {
        glUniform1i(loc, val);
    }
	else {
		std::cout << "Unable to find uniform location: " << name << std::endl;
	}
}

void GLSLProgram::printActiveUniforms() {
        GLint numUniforms = 0;
        glGetProgramInterfaceiv( handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);
        
        GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};
        
        printf("Active uniforms:\n");
        for( int i = 0; i < numUniforms; ++i ) {
                GLint results[4];
                glGetProgramResourceiv(handle, GL_UNIFORM, i, 4, properties, 4, NULL, results);
                
                if( results[3] != -1 ) continue;  // Skip uniforms in blocks 
                GLint nameBufSize = results[0] + 1;
                char * name = new char[nameBufSize];
                glGetProgramResourceName(handle, GL_UNIFORM, i, nameBufSize, NULL, name);
                printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
                delete [] name;
        }
}

void GLSLProgram::printActiveUniformBlocks() {
        GLint numBlocks = 0;
        
        glGetProgramInterfaceiv(handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
        GLenum blockProps[] = {GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH};
        GLenum blockIndex[] = {GL_ACTIVE_VARIABLES};
        GLenum props[] = {GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX};
        
        for(int block = 0; block < numBlocks; ++block) {
                GLint blockInfo[2];
                glGetProgramResourceiv(handle, GL_UNIFORM_BLOCK, block, 2, blockProps, 2, NULL, blockInfo);
                GLint numUnis = blockInfo[0];
                
                char * blockName = new char[blockInfo[1]+1];
                glGetProgramResourceName(handle, GL_UNIFORM_BLOCK, block, blockInfo[1]+1, NULL, blockName);
                printf("Uniform block \"%s\":\n", blockName);
                delete [] blockName;
                
                GLint * unifIndexes = new GLint[numUnis];
                glGetProgramResourceiv(handle, GL_UNIFORM_BLOCK, block, 1, blockIndex, numUnis, NULL, unifIndexes);
                
                for( int unif = 0; unif < numUnis; ++unif ) {
                        GLint uniIndex = unifIndexes[unif];
                        GLint results[3];
                        glGetProgramResourceiv(handle, GL_UNIFORM, uniIndex, 3, props, 3, NULL, results);
                        
                        GLint nameBufSize = results[0] + 1;
                        char * name = new char[nameBufSize];
                        glGetProgramResourceName(handle, GL_UNIFORM, uniIndex, nameBufSize, NULL, name);
                        printf("    %s (%s)\n", name, getTypeString(results[1]));
                        delete [] name;
                }
                
                delete [] unifIndexes;
        }
}

void GLSLProgram::printActiveAttribs() {
        GLint numAttribs;
        glGetProgramInterfaceiv( handle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);
        
        GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};
        
        printf("Active attributes:\n");
        for( int i = 0; i < numAttribs; ++i ) {
                GLint results[3];
                glGetProgramResourceiv(handle, GL_PROGRAM_INPUT, i, 3, properties, 3, NULL, results);
                
                GLint nameBufSize = results[0] + 1;
                char * name = new char[nameBufSize];
                glGetProgramResourceName(handle, GL_PROGRAM_INPUT, i, nameBufSize, NULL, name);
                printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
                delete [] name;
        }
}

const char * GLSLProgram::getTypeString( GLenum type ) {
        // There are many more types than are covered here, but
        // these are the most common in these examples.
        switch(type) {
                case GL_FLOAT:
                        return "float";
                case GL_FLOAT_VEC2:
                        return "vec2";
                case GL_FLOAT_VEC3:
                        return "vec3";
                case GL_FLOAT_VEC4:
                        return "vec4";
                case GL_DOUBLE:
                        return "double";
                case GL_INT:
                        return "int";
                case GL_UNSIGNED_INT:
                        return "unsigned int";
                case GL_BOOL:
                        return "bool";
                case GL_FLOAT_MAT2:
                        return "mat2";
                case GL_FLOAT_MAT3:
                        return "mat3";
                case GL_FLOAT_MAT4:
                        return "mat4";
                default:
                        return "?";
        }
}

void GLSLProgram::validate() throw(GLSLProgramException)
{
    if( ! isLinked() ) 
            throw GLSLProgramException("Program is not linked");

    GLint status;
    glValidateProgram( handle );
    glGetProgramiv( handle, GL_VALIDATE_STATUS, &status );

    if( GL_FALSE == status ) {
        // Store log and return false
        int length = 0;
        string logString;

        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length );

        if( length > 0 ) {
            char * c_log = new char[length];
            int written = 0;
            glGetProgramInfoLog(handle, length, &written, c_log);
            logString = c_log;
            delete [] c_log;
        }

        throw GLSLProgramException(string("Program failed to validate\n") + logString);
        
    }
}

int GLSLProgram::getUniformLocation(const char * name )
{
        std::map<string, int>::iterator pos;
        pos = uniformLocations.find(name);
        
        if( pos == uniformLocations.end() ) {
                uniformLocations[name] = glGetUniformLocation(handle, name);
        }
        
    return uniformLocations[name];
}

bool GLSLProgram::fileExists( const string & fileName )
{
    struct stat info;
    int ret = -1;

    ret = stat(fileName.c_str(), &info);
    return 0 == ret;
}

static bool isNextToken(const std::string& macro, const std::string& code, size_t offset = 0) {
    size_t macroOffset = code.find(macro, offset);
    if(offset == std::string::npos || macroOffset == std::string::npos) return false;
    for(size_t i = offset; i < macroOffset; ++i){
        const char c = code[i];
        if(c != ' ' && c != '\t') return false;
    }
    return true;
}

static bool precedingCharactersAreWhitespace(size_t offset, const std::string& code) {
    if( offset == 0 ) { 
        return true;
    }
    char c;
    for (size_t i = offset - 1; i > 0; --i) {
        c = code[i];
        if ( c == '\n') {
            return true;
        }
        if ( (c != ' ') && (c != '\t') ) {
            return false;
        }
    }
    return true;
}

/** Returns the position of the last pragma of the form "#macro" (with any whitespace between the # and macro) in code, starting from the \param offset */
static size_t findLastPragmaWithSpaces(const std::string& macro, const std::string& code, size_t offset) {
    
    do {
        offset = code.rfind("#", offset);
        if(offset != std::string::npos) {
            if(precedingCharactersAreWhitespace(offset, code) && isNextToken(macro, code, (int)offset + 1)){
                return offset;
            }
            offset -= 1;
        } 

    } while(offset != std::string::npos);

	if (boost::starts_with(code, "#")) {
        if(isNextToken(macro, code, 1)) {
            return 0;
        }
    }
    
    return offset;
}

static size_t findPragmaWithSpaces(const std::string& macro, const std::string& code, size_t offset = 0) {
	if (boost::starts_with(code, "#")){
        if (isNextToken(macro, code, 1)) {
            return 0;
        }
    }

    do {
        offset = code.find("#", offset);
        if (offset != std::string::npos) {
            if (precedingCharactersAreWhitespace(offset, code) && isNextToken(macro, code, (int)offset + 1)) {
                // Look backwards to make sure that we aren't in a comment
                const int previousBlockCommentEnd(int(code.rfind("*/", offset)));
                const int previousBlockCommentBegin(int(code.rfind("/*", offset)));

                // Take advantage of std::npos == -1
                if (previousBlockCommentEnd >= previousBlockCommentBegin) {
                    // We are not in a comment
                    return offset;
                }

            }
            ++offset;
        } 

    } while (offset != std::string::npos);

    return offset;
}

static void findPragmaBlockStartAndEnd(const std::string& beginMacro, const std::string& endMacro, const std::string& source, size_t& beginLocation, size_t& endLocation) {
    beginLocation = findPragmaWithSpaces(beginMacro, source);
    if (beginLocation == std::string::npos) {
        endLocation = std::string::npos;
        return;
    }
    size_t currentLocation = beginLocation + 1;
    int openMacroCount = 1;
    while (openMacroCount > 0) {
        size_t nextBeginLocation    = findPragmaWithSpaces(beginMacro,  source, currentLocation);
        size_t nextEndLocation      = findPragmaWithSpaces(endMacro,    source, currentLocation);
        if ( nextEndLocation == std::string::npos ) {
            endLocation = std::string::npos;
            return;
        }
        if ( nextBeginLocation == std::string::npos || nextBeginLocation > nextEndLocation ) {
            endLocation     = nextEndLocation;
            currentLocation = nextEndLocation + 1;
            --openMacroCount;
        } else {
            currentLocation     = nextBeginLocation + 1;
            ++openMacroCount;
        }
    }
    return;

}

static void expandForBlock(std::string& expandedBlock, const std::string& innerBlock, const std::string& counterToken, int initValue, int endValue) {
    const std::string& searchString = "$(" + counterToken + ")";
    for(int i = initValue; i < endValue; ++i) {
		expandedBlock += boost::replace_all_copy(innerBlock, searchString, MinVR::intToString(i));
    }
}

/** Includes everything after "#endfor[each]" (including the newline) in afterForBlockString */
static void fragmentSourceAroundForPragma(const std::string& source, size_t forLocation, size_t endForLocation, std::string& beforeForBlockString, 
                                        std::string& forLine, std::string& innerBlock, std::string& afterForBlockString){
    
    size_t afterEndOfForLine    = source.find('\n', forLocation) + 1;
    
                                            
    beforeForBlockString        = source.substr(0, forLocation);
    forLine                     = source.substr(forLocation, afterEndOfForLine - forLocation);
    innerBlock                  = source.substr(afterEndOfForLine, endForLocation - afterEndOfForLine);

    size_t indexOfNewlineAferEndFor = source.find('\n', endForLocation);
    if(indexOfNewlineAferEndFor == std::string::npos) {
        afterForBlockString = "";
    } else {
        size_t afterForBlockStart   = indexOfNewlineAferEndFor + 1;
        afterForBlockString         = source.substr(afterForBlockStart, source.length() - afterForBlockStart);
    }
}

/** Don't emit an error message if the #for loops macro arg bounds don't exist, but rather insert an #error macro into the source, so if its in a dead branch of an #ifdef it still works */
static void parseForLine(std::string& forLine, std::string& counterToken, int& initValue, int& endValue, std::map<std::string, std::string> &args)
{
	MinVR::readSymbol(forLine, "#");
	MinVR::readSymbol(forLine, "for");
	MinVR::readSymbol(forLine, "(");
	MinVR::readSymbol(forLine, "int");

	MinVR::popNextToken(forLine, counterToken); 
    MinVR::readSymbol(forLine, "=");

	std::string initTokenString;
	MinVR::popNextToken(forLine, initTokenString);
	if (args.find(initTokenString) != args.end()) {
		initValue = MinVR::stringToInt(args[initTokenString]);
	}
	else {
		initValue = MinVR::stringToInt(initTokenString);
	}

    MinVR::readSymbol(forLine, ";");
    MinVR::readSymbol(forLine, counterToken);
    MinVR::readSymbol(forLine, "<");

	std::string endTokenString;
	MinVR::popNextToken(forLine, endTokenString);
	if (args.find(endTokenString) != args.end()) {
		endValue = MinVR::stringToInt(args[endTokenString]);
	}
	else {
		endValue = MinVR::stringToInt(endTokenString);
	}

    MinVR::readSymbol(forLine, ";");
    MinVR::readSymbol(forLine, "++");
    MinVR::readSymbol(forLine, counterToken);
    MinVR::readSymbol(forLine, ")");
}

bool GLSLProgram::expandForPragmas(std::string& processedSource, const std::string& originalSource, std::map<std::string, std::string> &args)
{
    processedSource = originalSource;

    size_t forLocation, endForLocation;
    findPragmaBlockStartAndEnd("for", "endfor", processedSource, forLocation, endForLocation);
    while (forLocation != std::string::npos) {

		if (endForLocation == std::string::npos) {
            BOOST_ASSERT_MSG(false, "No matching #endfor found.\n");
            return false;
        }

        std::string beforeForBlockString, forLine, innerBlock, afterForBlockString;
        fragmentSourceAroundForPragma(processedSource, forLocation, endForLocation, beforeForBlockString, forLine, innerBlock, afterForBlockString);

        int initValue, endValue;
        std::string counterToken; 
        parseForLine(forLine, counterToken, initValue, endValue, args);

        std::string expandedBlock;
        expandForBlock(expandedBlock, innerBlock, counterToken, initValue, endValue);
        // Newlines take the place of the old #for and #endfor lines
        processedSource = beforeForBlockString + "\n" + expandedBlock + "\n" + afterForBlockString;
        
        findPragmaBlockStartAndEnd("for", "endfor", processedSource, forLocation, endForLocation);
    }

    return true;
}

void GLSLProgram::addDefines(std::string& processedSource, const std::string&originalSource, std::map<std::string, std::string> &args)
{
	std::string versionLine;
	std::string defines;
	std::string afterDefines;

	size_t endOfVersionLine    = originalSource.find('\n')+1; // Assumes first line declares a glsl version number
    
    versionLine = originalSource.substr(0, endOfVersionLine);
	for(auto it = args.begin(); it != args.end(); ++it) {
		defines += "#define " + it->first + " " + it->second + "\n";
	}

    afterDefines = originalSource.substr(endOfVersionLine);

	processedSource = versionLine + defines + afterDefines;
}