// Standard Header Files
#include <stdio.h> // for File I/O
#include <stdlib.h> // for exit(0)
#include <memory.h> // for memset

// XLib Header Files
#include <X11/Xlib.h> 
#include <X11/Xutil.h>
#include <X11/XKBlib.h> // for XkbKeycodeToKeysym()
#include <X11/keysym.h> // for KeySym
#include <X11/Xatom.h> // for XA_ATOM constant

// Macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// Global Variables
const char* gpszAppName = "ARTR";

Display *gpDisplay = NULL;
XVisualInfo *gpXVisualInfo = NULL;
Window gWindow; 
Colormap gColorMap;

int winWidth = WIN_WIDTH;
int winHeight = WIN_HEIGHT;

Bool bFullScreen = False;
Bool bEscapeKeyPressed = False;
Bool bActiveWindow = False;
Bool bWindowMinimized = False;

FILE *fptr = NULL;

int main(int argc, char *argv[]) {
    // Function Declarations
    void ToggleFullScreen(void);
    Bool isWindowMinimized(void);
    int initialize(void);
    void resize(int, int);
    void display(void);
    void uninitialize(void);
    void update(void);

    // Variables
    XVisualInfo xVisualInfo;
    int iNumFBConfigs = 0;
    XSetWindowAttributes windowAttributes;
    int defaultScreen;
    int defaultDepth;
    int styleMask;
    Atom windowManagerDeleteAtom;
    int screenWidth, screenHeight;
    XEvent event;
    KeySym keySym;
    char keys[26];
    Bool bDone = False;

    // File I/O
    fptr = fopen("XWindowLog.txt", "w");
    if(fptr == NULL) {
        printf("Cannot Create File!!..Exiting Now...\n");
        exit(1);
    } else {
        fprintf(fptr, "File Created Successfully..\n");
    }

    // Open Display
    gpDisplay = XOpenDisplay(NULL);
	if(gpDisplay == NULL) {
		fprintf(fptr, "main(): Unable To Open X Display.\n");
		uninitialize();
		exit(1);
	} else {
        fprintf(fptr, "main(): X Display Opened Successfully!..\n");
    }

    // Get Default Screen
    defaultScreen = XDefaultScreen(gpDisplay);


    // Initialize Local XVisualInfo
    memset((void*)&xVisualInfo, 0, sizeof(XVisualInfo));
    xVisualInfo.screen = defaultScreen;

    gpXVisualInfo = XGetVisualInfo(gpDisplay, VisualScreenMask, &xVisualInfo, &iNumFBConfigs);
    if(gpXVisualInfo == NULL) {
        fprintf(fptr, "main(): Unable To Get XVisualInfo.\n");
        uninitialize();
        exit(1);
    } else {
        fprintf(fptr, "main(): XVisualInfo Obtained Successfully with %d FBConfigs!..\n", iNumFBConfigs);
    }

    // Create Color Map
    gColorMap = XCreateColormap(gpDisplay,
                XRootWindow(gpDisplay, xVisualInfo.screen),
                gpXVisualInfo->visual,
                AllocNone);

    // Initialize Window Attributes
    memset((void*)&windowAttributes, 0, sizeof(XSetWindowAttributes));
    windowAttributes.border_pixel = 0;
    windowAttributes.background_pixel = XBlackPixel(gpDisplay, defaultScreen);
    windowAttributes.background_pixmap = 0;
    windowAttributes.colormap = gColorMap;
    windowAttributes.event_mask = ExposureMask | VisibilityChangeMask | FocusChangeMask 
                        | KeyPressMask | StructureNotifyMask | PropertyChangeMask;
    
    // Specify Style Mask
    styleMask = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap;

    gWindow = XCreateWindow(gpDisplay,
                XRootWindow(gpDisplay, xVisualInfo.screen),
                0, 0,
                WIN_WIDTH, WIN_HEIGHT,
                0,
                gpXVisualInfo->depth,
                InputOutput,
                gpXVisualInfo->visual,
                styleMask,
                &windowAttributes);

    if(!gWindow) {
        fprintf(fptr, "main(): Failed To Create Window.\n");
        uninitialize();
        exit(1);
    } else {
        fprintf(fptr, "main(): Window Created Successfully!..\n");
    }

    // Set Window Caption
    XStoreName(gpDisplay, gWindow, gpszAppName);

    // Prepare Destruction of window by window manager
    windowManagerDeleteAtom = XInternAtom(gpDisplay, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(gpDisplay, gWindow, &windowManagerDeleteAtom, 1);

    // Show Window
    XMapWindow(gpDisplay, gWindow);

    // Center the Window
    screenWidth = XWidthOfScreen(XScreenOfDisplay(gpDisplay, defaultScreen));
    screenHeight = XHeightOfScreen(XScreenOfDisplay(gpDisplay, defaultScreen));

    XMoveWindow(gpDisplay, gWindow, (screenWidth - WIN_WIDTH) / 2, (screenHeight - WIN_HEIGHT) / 2);


    // Initialize
    int iResult = initialize();
    if(iResult != 0) {
        fprintf(fptr, "main(): Initialization Failed!.\n");
        uninitialize();
        exit(1);
    } else {
        fprintf(fptr, "main(): Initialization Successful!..\n");
    }

    // Event Loop
    while(!bDone) {
        while(XPending(gpDisplay)) {

            XNextEvent(gpDisplay, &event);

            switch(event.type) {
                case MapNotify: // WM_CREATE
                    break;

                case FocusIn: // WM_SETFOCUS
                    bActiveWindow = True;
                    break;

                case FocusOut: // WM_KILLFOCUS
                    bActiveWindow = False;
                    break;

                case KeyPress:
                    keySym = XkbKeycodeToKeysym(gpDisplay, event.xkey.keycode, 0, 0);
                    switch(keySym) {
                        case XK_Escape:
                            bEscapeKeyPressed = True;
                            break;

                        default:
                            break;
                    }

                    XLookupString(&event.xkey, keys, sizeof(keys), NULL, NULL);
                    switch(keys[0]) {
                        case 'F':
                        case 'f':
                            ToggleFullScreen();
                            bFullScreen = !bFullScreen;
                            break;

                        default:
                            break;
                    }
                    break;

                case ConfigureNotify: // WM_SIZE
                    winWidth = event.xconfigure.width;
                    winHeight = event.xconfigure.height;

                    resize(winWidth, winHeight);
                    break;

                case PropertyNotify: // WM_*PROPERTY*
                    if(isWindowMinimized() == True) {
                        fprintf(fptr, "main(): Window Minimized!.\n");
                        bWindowMinimized = True;
                    } else {
                        fprintf(fptr, "main(): Window Restored!.\n");
                        bWindowMinimized = False;
                    }
                    break;

                case DestroyNotify:
					break;

                case 33: // WM_DESTROY
                    bDone = True;
                    break;

                default:
                    break;
            }
        }
        if(bActiveWindow == True) {
            if(bEscapeKeyPressed == True) {
                bDone = True;
            }
            update();
            display();
        }
    }


    return(0);
}

void ToggleFullScreen(void) {

	Atom wm_state;
	Atom fullscreen;
	XEvent xeve = {0};

	//code
	wm_state = XInternAtom(gpDisplay, "_NET_WM_STATE", False);
	memset(&xeve, 0, sizeof(XEvent));

	xeve.type = ClientMessage;
	xeve.xclient.window = gWindow;
	xeve.xclient.message_type = wm_state;
	xeve.xclient.format = 32;
	xeve.xclient.data.l[0] = bFullScreen ? 0 : 1;

	fullscreen = XInternAtom(gpDisplay, "_NET_WM_STATE_FULLSCREEN", False);
	xeve.xclient.data.l[1] = fullscreen;

	XSendEvent(gpDisplay,
		   XRootWindow(gpDisplay, gpXVisualInfo->screen),
		   False,
		   StructureNotifyMask,
		   &xeve);

}

Bool isWindowMinimized(void) {

    // Function Declarations
    void uninitialize(void);

    // Variable Declarations
    Bool windowMinimized = False;
    int iResult = -1;
    Atom returned_property_type = None;
    int returned_property_format = -1;
    unsigned long no_returned_items = 0;
    unsigned long no_bytes_remained = 0;
    Atom *returned_property_data_array = NULL;
    
    Atom wm_state_atom = XInternAtom(gpDisplay, "_NET_WM_STATE", False);

    if(wm_state_atom == None) {
        fprintf(fptr, "isWindowMinimized(): Unable To Get _NET_WM_STATE Atom.\n");
        uninitialize();
        exit(1);
    }

    Atom wm_state_hidden_atom = XInternAtom(gpDisplay, "_NET_WM_STATE_HIDDEN", False);
    if(wm_state_hidden_atom == None) {
        fprintf(fptr, "isWindowMinimized(): Unable To Get _NET_WM_STATE_HIDDEN Atom.\n");
        uninitialize();
        exit(1);
    }
    

    iResult = XGetWindowProperty(gpDisplay,
                gWindow,
                wm_state_atom,
                0l,
                1024,
                False,
                XA_ATOM,
                &returned_property_type,
                &returned_property_format,
                &no_returned_items,
                &no_bytes_remained,
                (unsigned char**)&returned_property_data_array);

    if(iResult != Success || returned_property_data_array == NULL) {
        if(returned_property_data_array) {
            XFree(returned_property_data_array);
            returned_property_data_array = NULL;
        }
        return (False);
    } else {
        for(unsigned long i = 0; i < no_returned_items; i++) {
            // Check if any of the returned properties is _NET_WM_STATE_HIDDEN
            if(returned_property_data_array[i] == wm_state_hidden_atom) {
                windowMinimized = True;
                break;
            }
        }
    }

    if(returned_property_data_array) {
        XFree(returned_property_data_array);
        returned_property_data_array = NULL;
    }
    return (windowMinimized);
}

int initialize(void) {
    // Code

    return(0);
}

void resize(int width, int height) {
    // Code
}

void update(void) {
    // Code
}

void display(void) {
    // Code
}

void uninitialize(void) {

    // Functions
    void ToggleFullScreen(void);

    if(bFullScreen == True) {
        ToggleFullScreen();
        bFullScreen = False;
    }

    if(gWindow) {
        XDestroyWindow(gpDisplay, gWindow);
    }

    if(gColorMap) {
        XFreeColormap(gpDisplay, gColorMap);
    }

    if(gpXVisualInfo) {
        XFree((void*)gpXVisualInfo);
        gpXVisualInfo = NULL;
    }

    if(gpDisplay) {
        XCloseDisplay(gpDisplay);
        gpDisplay = NULL;
    }

    if(fptr) {
        fprintf(fptr, "\nuninitialize(): File Closed Successfully..\n");
        fclose(fptr);
        fptr = NULL;
    }
}
