#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "getinfo.hpp"

static std::string safeStr(GLenum name){
    const GLubyte* s = glGetString(name);
    return s ? reinterpret_cast<const char*>(s) : "N/A";
}

hwInfo getInfo(){
    hwInfo h;
    h.vendor   = safeStr(GL_VENDOR);
    h.version  = safeStr(GL_VERSION);
    h.renderer = safeStr(GL_RENDERER);
    h.glsl     = safeStr(GL_SHADING_LANGUAGE_VERSION);
    return h;
}

std::string getProfile(){
    GLint myint;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &myint);

    if ( myint & GL_CONTEXT_CORE_PROFILE_BIT ) {
        return "We are using CORE profile\n";
    } else {
        if ( myint & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {    
            return "We are using COMPATIBILITY profile\n";
        } else {
                return "What??";
        }
    }
}

// HOWTO get string safely ( glGetString() may return nullptr ) 
/*const char* mystring = (const char*)glGetString(GL_SOME_PARAMETER_NAME);

if (mystring == nullptr)
    std::cout << "<Unknown>\n";
else 
    std::cout << "Value is:" << mystring << '\n';
*/

//
// use following symbolic constants to get and print GL numeric version
// verify, that created window initialized at least requested version 4.6
//
/*
GL_MAJOR_VERSION
GL_MINOR_VERSION

// HOWTO get integer
GLint myint;
glGetIntegerv(GL_SOME_PARAMETER_NAME, &myint);

std::cout << "Value of myint is:" << myint << '\n';

//
// use following symbolic constants to get and print GL context info
// verify, that initialized context is in CORE profile
//
GL_CONTEXT_PROFILE_MASK
    GL_CONTEXT_CORE_PROFILE_BIT, GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
    
// getting profile info is little more complicated, but useful 
GLint myint;
glGetIntegerv(GL_CONTEXT_PROFILE_MASK, myint);

if ( myint & GL_CONTEXT_CORE_PROFILE_BIT ) {
    std::cout << "We are using CORE profile\n";
} else {
    if ( myint & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {    
        std::cout << "We are using COMPATIBILITY profile\n";
    } else {
            throw std::runtime_error("What??");
    }
}


//
// use following symbolic constants to get and print additional GL context info
// Multiple bits can be used simultaneously.
// 
glGetIntegerv(GL_CONTEXT_FLAGS,...

GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
GL_CONTEXT_FLAG_DEBUG_BIT
GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT
GL_CONTEXT_FLAG_NO_ERROR_BIT
*/
