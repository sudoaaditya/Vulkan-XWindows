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

// vulkan related header files
#define VK_USE_PLATFORM_XLIB_KHR
#include<vulkan/vulkan.h>

// Macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define _ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))

// Global Variables
const char* gpszAppName = "AMK_VulkanXWindowsApp : Instance";

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

// Vulkan Related Global Variables
// instance extension related variables
uint32_t enabledInstanceExtensionCount = 0;
const char *enabledInstanceExtensionNames_array[2]; // VK_KHR_SURFACE_EXTENSION_NAME & VK_KHR_XLIB_SURFACE_EXTENSION_NAME
// vulkan instance
VkInstance vkInstance = VK_NULL_HANDLE;

int main(int argc, char *argv[]) {
    // Function Declarations
    void ToggleFullScreen(void);
    Bool isWindowMinimized(void);
    VkResult initialize(void);
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
    
    VkResult vkResult = VK_SUCCESS;

    // File I/O
    fptr = fopen("_VulkanXWindowsLog.txt", "w");
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
    vkResult = initialize();
    if(vkResult != VK_SUCCESS) {
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
                        bWindowMinimized = True;
                    } else {
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


VkResult initialize(void) {

    // function declarations
    VkResult createVulkanInstance(void);

    // varibales
    VkResult vkResult = VK_SUCCESS;

    // code
    vkResult = createVulkanInstance();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createVulkanInstance() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createVulkanInstance() Successful!.\n");
    }

    return (vkResult);
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

    // Vulkan Uninitialization Code
    // destroy vkInstance
    if(vkInstance) {
        vkDestroyInstance(vkInstance, NULL);
        vkInstance = VK_NULL_HANDLE;
		fprintf(fptr,"\nuninitialize(): vkDestroyInstance() Succeed\n");

    }

    if(fptr) {
        fprintf(fptr, "\nuninitialize(): File Closed Successfully..\n");
        fclose(fptr);
        fptr = NULL;
    }
}


//! //////////////////////////////////////// Definations of vulkan Related Functions ///////////////////////////////////////////////

VkResult createVulkanInstance (void) {
    // function declarations
    VkResult fillInstanceExtensionNames(void);

    // varibales
    VkResult vkResult = VK_SUCCESS;

    // code
    vkResult = fillInstanceExtensionNames();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVulkanInstance(): fillInstanceExtensionNames() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createVulkanInstance(): fillInstanceExtensionNames() Successful!.\n");
    }

    // step 2:
    VkApplicationInfo vkApplicationInfo;
    memset((void*)&vkApplicationInfo, 0, sizeof(VkApplicationInfo));

    vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkApplicationInfo.pNext = NULL;
    vkApplicationInfo.pApplicationName = gpszAppName;
    vkApplicationInfo.applicationVersion = 1;
    vkApplicationInfo.pEngineName = gpszAppName;
    vkApplicationInfo. engineVersion = 1;
    vkApplicationInfo.apiVersion = VK_API_VERSION_1_3;  // change it VK_API_VERSION_1_4 once you update vulkan

    // Step 3: initialize struct VkInstanceCreateInfo
    VkInstanceCreateInfo vkInstanceCreateInfo;
    memset((void*)&vkInstanceCreateInfo, 0, sizeof(VkInstanceCreateInfo));

    vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkInstanceCreateInfo.pNext = NULL;
    vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
    vkInstanceCreateInfo.enabledExtensionCount = enabledInstanceExtensionCount;
    vkInstanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensionNames_array;

    // Step 4: Create instance using vkCreateInstance
    vkResult = vkCreateInstance(&vkInstanceCreateInfo, NULL, &vkInstance);
    if(vkResult == VK_ERROR_INCOMPATIBLE_DRIVER) {
        fprintf(fptr, "createVulkanInstance(): vkCreateInstance() Failed Due to Incompatible Driver (%d)!.\n", vkResult);
        return (vkResult);
    } else if(vkResult == VK_ERROR_EXTENSION_NOT_PRESENT) {
        fprintf(fptr, "createVulkanInstance(): vkCreateInstance() Failed Due to Extention Not Present (%d)!.\n", vkResult);
        return (vkResult);
    } else if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVulkanInstance(): vkCreateInstance() Failed Due to Unknown Reason (%d)!.\n", vkResult);
        return (vkResult);
    } else {
        fprintf(fptr, "createVulkanInstance(): vkCreateInstance() Successful!.\n\n");
    }

    return (vkResult);

}

VkResult fillInstanceExtensionNames (void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // Step 1: Find how many instance extension are supported by this vulkan driver & keep it in local variable
    uint32_t instanceExtensionCount = 0;

    vkResult = vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, NULL);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "fillInstanceExtensionNames(): vkEnumerateInstanceExtensionProperties() First Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillInstanceExtensionNames(): vkEnumerateInstanceExtensionProperties() First Call Successful!.\n");
    }

    // step 2: Allocate & fill struct vk Extenstions array correspoinding to above acount
    VkExtensionProperties *vkExtensionProperties_array = NULL;
    vkExtensionProperties_array = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * instanceExtensionCount);
    vkResult = vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, vkExtensionProperties_array);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "fillInstanceExtensionNames(): vkEnumerateInstanceExtensionProperties() Second Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillInstanceExtensionNames(): vkEnumerateInstanceExtensionProperties() Second Call Successful!.\n");
    }

    // Step 3: 
    char **instanceExtensionNames_array = NULL;
    instanceExtensionNames_array = (char**)malloc(sizeof(char*) * instanceExtensionCount);
    for(uint32_t i = 0; i < instanceExtensionCount; i++) {
        instanceExtensionNames_array[i] = (char*)malloc(sizeof(char) * strlen(vkExtensionProperties_array[i].extensionName) + 1);
        memcpy(
            instanceExtensionNames_array[i], 
            vkExtensionProperties_array[i].extensionName, 
            strlen(vkExtensionProperties_array[i].extensionName) + 1
        );
        fprintf(fptr, "fillInstanceExtensionNames(): Vulkan Extension Name = %s \n", instanceExtensionNames_array[i]);
    }

    // step 4:
    free(vkExtensionProperties_array);

    // step 5
    VkBool32 surfaceExtensionFound = VK_FALSE;
    VkBool32 win32vulkanSurfaceExtensionFound = VK_FALSE;
    for(uint32_t i = 0; i < instanceExtensionCount; i++) {
        if(strcmp(instanceExtensionNames_array[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            surfaceExtensionFound = VK_TRUE;
            enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
        }
        if(strcmp(instanceExtensionNames_array[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) ==  0) {
            win32vulkanSurfaceExtensionFound = VK_TRUE;
            enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
        }
    }

    // step 6
    for(uint32_t i = 0; i < instanceExtensionCount; i++) {
        free(instanceExtensionNames_array[i]);
    }
    free(instanceExtensionNames_array);

    // step 7:
    if(surfaceExtensionFound == VK_FALSE) {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
        fprintf(fptr, "fillInstanceExtensionNames(): VK_KHR_SURFACE_EXTENSION_NAME Not Found!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillInstanceExtensionNames(): VK_KHR_SURFACE_EXTENSION_NAME Found!.\n");
    }

    if(win32vulkanSurfaceExtensionFound == VK_FALSE) {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
        fprintf(fptr, "fillInstanceExtensionNames(): VK_KHR_WIN32_SURFACE_EXTENSION_NAME Not Found!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillInstanceExtensionNames(): VK_KHR_WIN32_SURFACE_EXTENSION_NAME Found!.\n");
    }

    // step 8:
    for(uint32_t i = 0; i < enabledInstanceExtensionCount; i++) {
        fprintf(fptr, "fillInstanceExtensionNames(): Enabled Vulkan Instance Extension Name = %s \n", enabledInstanceExtensionNames_array[i]);
    }

    return vkResult;
}