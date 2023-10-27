#if 0
// Note (Aaron): Draw window using X11/xlib
// source: https://rosettacode.org/wiki/Window_creation/X11#Xlib
// gcc -g -o test test.c -lX11

#include <X11/Xlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char const *argv[])
{
    Display *d;
    Window w;
    XEvent e;
    const char *msg = "Hello, world!";
    int s;

    d = XOpenDisplay(NULL);
    if (d == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, 100, 100, 1, BlackPixel(d, s), WhitePixel(d, s));

    XSelectInput(d, w, ExposureMask | KeyPressMask);
    XMapWindow(d, w);

    while (1)
    {
        XNextEvent(d, &e);
        if (e.type == Expose)
        {
            XFillRectangle(d, w, DefaultGC(d, s), 20, 20, 10, 10);
            XDrawString(d, w, DefaultGC(d, s), 10, 50, msg, strlen(msg));
        }
        if (e.type == KeyPress)
        {
            break;
        }
    }

    XCloseDisplay(d);
    return 0;
}
#endif


#if 0
// Note (Aaron): Draw window using X11/xcb
// source: https://rosettacode.org/wiki/Window_creation/X11#XCB
// gcc -g -o test test.c -lcbx

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xcb/xcb.h>

int main ()
{
  xcb_connection_t    *c;
  xcb_screen_t        *screen;
  xcb_drawable_t       win;
  xcb_gcontext_t       foreground;
  xcb_gcontext_t       background;
  xcb_generic_event_t *e;
  uint32_t             mask = 0;
  uint32_t             values[2];

  char string[] = "Hello, XCB!";
  uint8_t string_len = strlen(string);

  xcb_rectangle_t rectangles[] = {
    {40, 40, 20, 20},
  };

  c = xcb_connect (NULL, NULL);

  /* get the first screen */
  screen = xcb_setup_roots_iterator (xcb_get_setup (c)).data;

  /* root window */
  win = screen->root;

  /* create black (foreground) graphic context */
  foreground = xcb_generate_id (c);
  mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  values[0] = screen->black_pixel;
  values[1] = 0;
  xcb_create_gc (c, foreground, win, mask, values);

  /* create white (background) graphic context */
  background = xcb_generate_id (c);
  mask = XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  values[0] = screen->white_pixel;
  values[1] = 0;
  xcb_create_gc (c, background, win, mask, values);

  /* create the window */
  win = xcb_generate_id(c);
  mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  values[0] = screen->white_pixel;
  values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
  xcb_create_window (c,                             /* connection    */
                     XCB_COPY_FROM_PARENT,          /* depth         */
                     win,                           /* window Id     */
                     screen->root,                  /* parent window */
                     0, 0,                          /* x, y          */
                     150, 150,                      /* width, height */
                     10,                            /* border_width  */
                     XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class         */
                     screen->root_visual,           /* visual        */
                     mask, values);                 /* masks         */

  /* map the window on the screen */
  xcb_map_window (c, win);

  xcb_flush (c);

  while ((e = xcb_wait_for_event (c))) {
    switch (e->response_type & ~0x80) {
    case XCB_EXPOSE:
      xcb_poly_rectangle (c, win, foreground, 1, rectangles);
      xcb_image_text_8 (c, string_len, win, background, 20, 20, string);
      xcb_flush (c);
      break;
    case XCB_KEY_PRESS:
      goto endloop;
    }
    free (e);
  }
  endloop:

  return 0;
}
#endif

#if 1
// Note (Aaron):
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
#endif
