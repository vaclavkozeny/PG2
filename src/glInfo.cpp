#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glInfo.hpp"

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