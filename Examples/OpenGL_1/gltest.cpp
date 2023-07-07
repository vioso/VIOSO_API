#include <stdio.h>
#include <iostream>
#ifdef WIN32
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#else
#include <X11/X.h>
#include <X11/Xlib.h>
#endif // def WIN32
#include <GL/gl.h>
#include <GL/glu.h>
#define GL_EXT_DEFINE_AND_IMPLEMENT
#include "../../VIOSOWarpBlend/GL/GLext.h"

#include "../../Include/VIOSOWarpBlend.hpp"

int logStr( int level, char const* str )
{
    std::cout << str;
    return 0;
}

void DrawAQuad() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1., 1., -1., 1., 1., 20.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

    glBegin(GL_QUADS);
    glColor3f(1., 0., 0.); glVertex3f(-.75, -.75, 0.);
    glColor3f(0., 1., 0.); glVertex3f( .75, -.75, 0.);
    glColor3f(0., 0., 1.); glVertex3f( .75,  .75, 0.);
    glColor3f(1., 1., 0.); glVertex3f(-.75,  .75, 0.);
    glEnd();
}

#ifdef WIN32
HMODULE g_hModDll = 0;
int main( int argc, char* argv[] )
{
    g_hModDll = ::GetModuleHandle( nullptr );
}
#else
int main(int argc, char *argv[])
{
    GLint att[]{GLX_RGBA,GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    auto disp = XOpenDisplay(nullptr);
    if( disp)
    {
        auto root = XRootWindow(disp,0);
        auto vi = glXChooseVisual(disp,0,att);
        auto cmap = XCreateColormap(disp,root,vi->visual,AllocNone);
        XSetWindowAttributes swa{};
        swa.colormap = cmap;
        swa.event_mask = ExposureMask | KeyPressMask;

        auto win = XCreateWindow(disp, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

        XMapWindow(disp, win);
        XStoreName(disp, win, "VERY SIMPLE APPLICATION");

        auto glc = glXCreateContext(disp, vi, NULL, GL_TRUE);
        glXMakeCurrent(disp, win, glc);

        #define GL_EXT_INITIALIZE
        #include "../../VIOSOWarpBlend/GL/GLext.h"

        if(
        #define GL_EXT_TEST_VERBOSE
        #include "../../VIOSOWarpBlend/GL/GLext.h"
            )
            logStr(0,"OK");
        else
        {
            logStr( 0, "Failed");
            return -1;
        }

        VWB warper( "", nullptr, "VIOSOWarpBlendGL.ini", "Display1");
        warper.get().bUseGL110 = 1;
        if( VWB_ERROR_NONE != warper.Init() )
            return 3;

        glEnable(GL_DEPTH_TEST);
        GLint prog = glCreateProgram();
        GLint ps = glCreateShader(GL_FRAGMENT_SHADER);

        XWindowAttributes       gwa{};
        XEvent xev{};
        while( true )
        {
            XNextEvent(disp, &xev);

            if(xev.type == Expose) {
                XGetWindowAttributes(disp, win, &gwa);
                glViewport(0, 0, gwa.width, gwa.height);
                DrawAQuad();
                warper.Render(VWB_UNDEFINED_GL_TEXTURE);
                glXSwapBuffers(disp, win);
            }

            else if(xev.type == KeyPress) {
                glXMakeCurrent(disp, None, NULL);
                glXDestroyContext(disp, glc);
                XDestroyWindow(disp, win);
                XCloseDisplay(disp);
                break;
            }
        }
    }
    return 0;
}
#endif // def WIN32
