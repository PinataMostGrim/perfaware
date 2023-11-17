// Note (Aaron):
// DearImGui does not currently have a platform implementation that uses x11 directly. As such,
// this script remains an example for how to open an X11 window and acquire an OpenGL context
// but does not implement a platform layer for sim8086 (yet).

// source: https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib
// gcc -g -o test-opengl test-opengl.c -lX11 -lGL -lGLU

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>


void DrawAQuad()
{
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


int main(int argc, char const *argv[])
{
    Window rootWindow;
    Display *display;
    GLint attribList[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo *visualInfo;
    Colormap colorMap;
    XSetWindowAttributes swa;
    Window window;
    GLXContext glContext;
    XWindowAttributes windowAttributes;
    XEvent xEvent;

    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        printf("Error opening X display\n");
        return 1;
    }

    rootWindow = DefaultRootWindow(display);
    visualInfo = glXChooseVisual(display, 0, attribList);
    if (visualInfo == NULL)
    {
        printf("No appropriate visual found\n");
        return 1;
    }

    printf("\tvisual %p selected\n", (void *)visualInfo->visualid); // %p creates hexadecimal output like in glxinfo

    colorMap = XCreateColormap(display, rootWindow, visualInfo->visual, AllocNone);
    swa.colormap = colorMap;
    swa.event_mask = ExposureMask | KeyPressMask;

    window = XCreateWindow(display, rootWindow, 0, 0, 600, 600, 0, visualInfo->depth, InputOutput, visualInfo->visual, CWColormap | CWEventMask, &swa);
    XMapWindow(display, window);
    XStoreName(display, window, "VERY SIMPLE APPLICATION");

    glContext = glXCreateContext(display, visualInfo, NULL, GL_TRUE);
    glXMakeCurrent(display, window, glContext);

    while (1)
    {
        XNextEvent(display, &xEvent);
        if (xEvent.type == Expose)
        {
            XGetWindowAttributes(display, window, &windowAttributes);
            glViewport(0, 0, windowAttributes.width, windowAttributes.height);
            DrawAQuad();
            glXSwapBuffers(display, window);
        }
        else if (xEvent.type == KeyPress)
        {
            glXMakeCurrent(display, None, NULL);
            glXDestroyContext(display, glContext);
            XDestroyWindow(display, window);
            XCloseDisplay(display);
            return 0;
        }
    }

    return 0;
}
