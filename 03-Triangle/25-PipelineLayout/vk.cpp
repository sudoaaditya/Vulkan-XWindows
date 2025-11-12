// Standard Header Files
#include <stdio.h> // for File I/O
#include <stdlib.h> // for exit(0)
#include <memory.h> // for memset
// for math functions
#include <math.h>
using namespace std;

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
const char* gpszAppName = "AMK_VulkanXWindowsApp : Pipeline Layout";

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
// VK_KHR_SURFACE_EXTENSION_NAME & VK_KHR_WIN32_SURFACE_EXTENSION_NAME & VK_EXT_DEBUG_REPORT_EXTENSION_NAME
const char *enabledInstanceExtensionNames_array[3]; 
// vulkan instance
VkInstance vkInstance = VK_NULL_HANDLE;

//vulkan presentation surface object
VkSurfaceKHR vkSurfaceKHR = VK_NULL_HANDLE;

// vulkan physical device rekated variables
VkPhysicalDevice vkPhysicalDevice_selected = VK_NULL_HANDLE;
uint32_t graphicsQueueFamilyIndex_selected = UINT32_MAX;
VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;

//
uint32_t physicalDeviceCount = 0;
VkPhysicalDevice *vkPhysicalDevice_array = NULL;

// Device Extension related variables
uint32_t enabledDeviceExtensionCount = 0;
const char *enabledDeviceExtensionNames_array[1]; // VK_KHR_SWAPCHAIN_EXTENSION_NAME

// Vulkan Device
VkDevice vkDevice = VK_NULL_HANDLE;

// Device Queue
VkQueue vkQueue = VK_NULL_HANDLE;

// Surface Format & Surface ColorSpace
VkFormat vkFormat_color = VK_FORMAT_UNDEFINED;
VkColorSpaceKHR vkColorSpaceKHR = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

// Presentation Mode
VkPresentModeKHR vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;

// Swapchain
VkSwapchainKHR vkSwapchainKHR = VK_NULL_HANDLE;
VkExtent2D vkExtent2D_swapchain;

// Swapchain Images & Image Views
uint32_t swapchainImageCount = UINT32_MAX;
VkImage *swapchainImage_array = NULL;
VkImageView *swapchainImageView_array = NULL;

// Command Pool
VkCommandPool vkCommandPool = VK_NULL_HANDLE;

// Command Buffer
VkCommandBuffer *vkCommandBuffer_array;

// Frame Buffer
VkFramebuffer *vkFramebuffer_array = NULL;

// Render Pass
VkRenderPass vkRenderPass = VK_NULL_HANDLE;

// Fences & Semaphore
VkSemaphore vkSemaphore_backbuffer = VK_NULL_HANDLE;
VkSemaphore vkSemaphore_rendercomplete = VK_NULL_HANDLE;
VkFence *vkFence_array = NULL;

// Build Command Buffers
VkClearColorValue vkClearColorValue;

// Render Variables
Bool bInitialized = False;
uint32_t currentImageIndex = UINT32_MAX;

// Validation Layer
Bool bValidation = True;
uint32_t enabledValidationLayerCount = 0;
const char *enabledValidationLayerNames_array[1]; //VK_LAYER_KHRONOS_validation
VkDebugReportCallbackEXT vkDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT_fnptr = NULL;

// Vertex Buffer
typedef struct {
    VkBuffer vkBuffer;
    VkDeviceMemory vkDeviceMemory;
} VertexData;

// Position
VertexData vertexData_position;

// Shader Variables
VkShaderModule vkShaderModule_vertex = VK_NULL_HANDLE;
VkShaderModule vkShaderModule_fragment = VK_NULL_HANDLE;

// Descriptor Set Layout
VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;

// Pipeline Layout
VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;

int main(int argc, char *argv[]) {
    // Function Declarations
    void ToggleFullScreen(void);
    Bool isWindowMinimized(void);
    VkResult initialize(void);
    void resize(int, int);
    VkResult display(void);
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
        fprintf(fptr, "\nmain(): Initialization Successful!..\n");
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
            if(bWindowMinimized == False) {
                if(bActiveWindow == True) {
                    update();
                }
                vkResult = display();
                if(vkResult != VK_FALSE && vkResult != VK_SUCCESS 
                    && vkResult != VK_SUBOPTIMAL_KHR && vkResult != VK_ERROR_OUT_OF_DATE_KHR) {
                    fprintf(fptr, "WinMain(): display() Failed!.\n");
                    bDone = True;
                }
            }
        }
    }


    uninitialize();

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
    VkResult getSupportedSurface(void);
    VkResult getPhysicalDevice(void);
    VkResult printVKInfo(void);
    VkResult createVulkanDevice(void);
    void getDeviceQueue(void);
    VkResult createSwapchain(VkBool32);
    VkResult createSwapchainImagesAndImageViews(void);
    VkResult createCommandPool(void);
    VkResult createCommandBuffers(void);
    VkResult createVertexBuffer(void);
    VkResult createShaders(void);
    VkResult createDescriptorSetLayout(void);
    VkResult createPipelineLayout(void);
    VkResult createRenderPass(void);
    VkResult createFramebuffers(void);
    VkResult createSemaphores(void);
    VkResult createFences(void);
    VkResult buildCommandBuffers(void);

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

    // create vulkan presentation surface
    vkResult = getSupportedSurface();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): getSupportedSurface() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): getSupportedSurface() Successful!.\n");
    }

    // Get Physical Device, enumerate and select it's queue family index
    vkResult = getPhysicalDevice();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): getPhysicalDevice() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): getPhysicalDevice() Successful!.\n");
    }

    // Print Vulkan Info
    vkResult = printVKInfo();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): printVKInfo() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): printVKInfo() Successful!.\n");
    }

    vkResult = createVulkanDevice();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createVulkanDevice() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createVulkanDevice() Successful!.\n");
    }

    // Device Queue
    getDeviceQueue();

    // Swapchain
    vkResult = createSwapchain(VK_FALSE);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createSwapchain() Failed!.\n");
        return (VK_ERROR_INITIALIZATION_FAILED);
    } else {
        fprintf(fptr, "initialize(): createSwapchain() Successful!.\n\n");
    }

    // Swapchain Images & Image Views
    vkResult = createSwapchainImagesAndImageViews();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createSwapchainImagesAndImageViews() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createSwapchainImagesAndImageViews() Successful!.\n\n");
    }

    // Command Pool
    vkResult = createCommandPool();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createCommandPool() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createCommandPool() Successful!.\n\n");
    }

    // Command Buffer
    vkResult = createCommandBuffers();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createCommandBuffers() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createCommandBuffers() Successful!.\n\n");
    }

    // Create Vertex Buffer
    vkResult = createVertexBuffer();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createVertexBuffer() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createVertexBuffer() Successful!.\n\n");
    }

    // Create Shaders
    vkResult = createShaders();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createShaders() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createShaders() Successful!.\n\n");
    }

    // Create Descriptor Set Layout
    vkResult = createDescriptorSetLayout();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createDescriptorSetLayout() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createDescriptorSetLayout() Successful!.\n\n");
    }

    // Create Pipeline Layout
    vkResult = createPipelineLayout();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createPipelineLayout() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createPipelineLayout() Successful!.\n\n");
    }

    // Render Pass
    vkResult = createRenderPass();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createRenderPass() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createRenderPass() Successful!.\n\n");
    }

    // Framebuffers
    vkResult = createFramebuffers();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createFramebuffers() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createFramebuffers() Successful!.\n\n");
    }

    // Create Semaphores
    vkResult = createSemaphores();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createSemaphores() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createSemaphores() Successful!.\n\n");
    }

    // Create Fences
    vkResult = createFences();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): createFences() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): createFences() Successful!.\n\n");
    }

    // initialize clear color values
    memset((void*)&vkClearColorValue, 0, sizeof(VkClearColorValue));
    vkClearColorValue.float32[0] = 0.0f;
    vkClearColorValue.float32[1] = 0.0f;
    vkClearColorValue.float32[2] = 1.0f;
    vkClearColorValue.float32[3] = 1.0f; // analogous to glClearColor

    // Build Command Buffers
    vkResult = buildCommandBuffers();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "initialize(): buildCommandBuffers() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "initialize(): buildCommandBuffers() Successful!.\n\n");
    }

    // Initialization is completed!
    bInitialized = True;

    return (vkResult);
}

void resize(int width, int height) {
    // Code
}

void update(void) {
    // Code
}

VkResult display(void) {
    // Variables
    VkResult vkResult = VK_SUCCESS;

    // Code
    //if control comes here before initialization is done, then return false
    if(bInitialized == False) {
        vkResult = (VkResult)VK_FALSE;
        fprintf(fptr, "display(): bInitialized is FALSE!.\n");
        return (vkResult);
    }

    // Acquire index of next swapchain image
    vkResult = vkAcquireNextImageKHR(
        vkDevice, 
        vkSwapchainKHR,
        UINT64_MAX, // timeout in nanoseconds
        vkSemaphore_backbuffer,
        VK_NULL_HANDLE,
        &currentImageIndex
    );

    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "display(): vkAcquireNextImageKHR() Failed!.\n");
        return (vkResult);
    }

    // Use Fence to allow host to wait for complition of execution of prev command buffer
    vkResult = vkWaitForFences(vkDevice, 1, &vkFence_array[currentImageIndex], VK_TRUE, UINT64_MAX);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "display(): vkWaitForFences() Failed!.\n");
        return (vkResult);
    }

    // Now ready the facnces for execution of next command buffer
    vkResult = vkResetFences(vkDevice, 1, &vkFence_array[currentImageIndex]);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "display(): vkResetFences() Failed!.\n");
        return (vkResult);
    }

    // One of the memeber of vkSubmitInfo structure requires array of pipeline stages, we haveonly one have of 
    // complition of color attachment, so we need to create array of size 1
    const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // declare, memset & initialize vkSubmitInfo structure
    VkSubmitInfo vkSubmitInfo;
    memset((void*)&vkSubmitInfo, 0, sizeof(VkSubmitInfo));

    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.pNext = NULL;
    vkSubmitInfo.pWaitDstStageMask = &waitDstStageMask;
    vkSubmitInfo.waitSemaphoreCount = 1;
    vkSubmitInfo.pWaitSemaphores = &vkSemaphore_backbuffer;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = &vkCommandBuffer_array[currentImageIndex];
    vkSubmitInfo.signalSemaphoreCount = 1;
    vkSubmitInfo.pSignalSemaphores = &vkSemaphore_rendercomplete;

    // Now submit command buffer to queue for execution
    vkResult = vkQueueSubmit(vkQueue, 1, &vkSubmitInfo, vkFence_array[currentImageIndex]);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "display(): vkQueueSubmit() Failed!.\n");
        return (vkResult);
    }

    // We are going to present rendered image after declaring & initializing vkPresentInfoKHR structure
    VkPresentInfoKHR vkPresentInfoKHR;
    memset((void*)&vkPresentInfoKHR, 0, sizeof(VkPresentInfoKHR));

    vkPresentInfoKHR.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vkPresentInfoKHR.pNext = NULL;
    vkPresentInfoKHR.waitSemaphoreCount = 1;
    vkPresentInfoKHR.pWaitSemaphores = &vkSemaphore_rendercomplete;
    vkPresentInfoKHR.swapchainCount = 1;
    vkPresentInfoKHR.pSwapchains = &vkSwapchainKHR;
    vkPresentInfoKHR.pImageIndices = &currentImageIndex;
    vkPresentInfoKHR.pResults = NULL; // this is optional, so we are not using it

    // Present the queue!
    vkResult = vkQueuePresentKHR(vkQueue, &vkPresentInfoKHR);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "display(): vkQueuePresentKHR() Failed!.\n");
        return (vkResult);
    }

    vkDeviceWaitIdle(vkDevice); // VALIDATION USE CASE 1: Comment this line to see the error

    return (vkResult);
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

    // wait til vkDevice is idle
    if(vkDevice) {
        vkDeviceWaitIdle(vkDevice); // this basically waits on til all the operations are done using the device and then this function call returns
        fprintf(fptr, "\nuninitialize(): vkDeviceWaitIdle is done!\n");
    }

    // Destroy Fence
    // VALIDATION USE CASE 3: Comment this line to see the error
    if(vkFence_array) {
        for(uint32_t i = 0; i < swapchainImageCount; i++) {
            vkDestroyFence(vkDevice, vkFence_array[i], NULL);
            fprintf(fptr, "uninitialize(): vkDestroyFence() Succeed for {%d}!.\n", i);
            vkFence_array[i] = VK_NULL_HANDLE;
        }
    }

    if(vkFence_array) {
        free(vkFence_array);
        fprintf(fptr, "uninitialize(): freed vkFence_array!.\n");
        vkFence_array = NULL;
    }

    // Destroy Semaphore
    if(vkSemaphore_rendercomplete) {
        vkDestroySemaphore(vkDevice, vkSemaphore_rendercomplete, NULL);
        fprintf(fptr, "uninitialize(): vkDestroySemaphore() for Render Complete Succeed!\n");
        vkSemaphore_rendercomplete = VK_NULL_HANDLE;
    }

    if(vkSemaphore_backbuffer) {
        vkDestroySemaphore(vkDevice, vkSemaphore_backbuffer, NULL);
        fprintf(fptr, "uninitialize(): vkDestroySemaphore() for Back Buffer Succeed!\n");
        vkSemaphore_backbuffer = VK_NULL_HANDLE;
    }

    // Destroy Frame Buffers
    if(vkFramebuffer_array) {
        for(uint32_t i = 0; i < swapchainImageCount; i++) {
            vkDestroyFramebuffer(vkDevice, vkFramebuffer_array[i], NULL);
            fprintf(fptr, "uninitialize(): vkDestroyFramebuffer() Succeed for {%d}!.\n", i);
            vkFramebuffer_array[i] = VK_NULL_HANDLE;
        }
    }

    if(vkFramebuffer_array) {
        free(vkFramebuffer_array);
        fprintf(fptr, "uninitialize(): freed vkFramebuffer_array!.\n");
        vkFramebuffer_array = NULL;
    }

    // Destroy Render Pass
    if(vkRenderPass) {
        vkDestroyRenderPass(vkDevice, vkRenderPass, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyRenderPass() Succeed!\n");
        vkRenderPass = VK_NULL_HANDLE;
    }

    // Destroy Pipeline Layout
    if(vkPipelineLayout) {
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyPipelineLayout() Succeed!\n");
        vkPipelineLayout = VK_NULL_HANDLE;
    }

    // Destroy Descriptor Set Layout
    if(vkDescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyDescriptorSetLayout() Succeed!\n");
        vkDescriptorSetLayout = VK_NULL_HANDLE;
    }

    // Destroy Shader
    if(vkShaderModule_fragment) {
        vkDestroyShaderModule(vkDevice, vkShaderModule_fragment, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyShaderModule() Succeed for Fragment Shader!\n");
        vkShaderModule_fragment = VK_NULL_HANDLE;
    }

    if(vkShaderModule_vertex) {
        vkDestroyShaderModule(vkDevice, vkShaderModule_vertex, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyShaderModule() Succeed for Vertex Shader!\n");
        vkShaderModule_vertex = VK_NULL_HANDLE;
    }

    // Destroy Vertex Buffer
    if(vertexData_position.vkDeviceMemory) {
        vkFreeMemory(vkDevice, vertexData_position.vkDeviceMemory, NULL);
        fprintf(fptr, "uninitialize(): vkFreeMemory() Succeed for Vertex Buffer!\n");
        vertexData_position.vkDeviceMemory = VK_NULL_HANDLE;
    }

    if(vertexData_position.vkBuffer) {
        vkDestroyBuffer(vkDevice, vertexData_position.vkBuffer, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyBuffer() Succeed for Vertex Buffer!\n");
        vertexData_position.vkBuffer = VK_NULL_HANDLE;
    }

    // Destroy  Command Buffers
    if(vkCommandBuffer_array) {
        for(uint32_t i = 0; i < swapchainImageCount; i++) {
            vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &vkCommandBuffer_array[i]);
            fprintf(fptr, "uninitialize(): vkFreeCommandBuffers() Succeed for {%d}\n", i);
            vkCommandBuffer_array[i] = VK_NULL_HANDLE;
        }
    }

    if(vkCommandBuffer_array) {
        free(vkCommandBuffer_array);
        fprintf(fptr, "uninitialize(): freed vkCommandBuffer_array!.\n");
        vkCommandBuffer_array = NULL;
    }

    // Destroy the command pool
    if(vkCommandPool) {
        vkDestroyCommandPool(vkDevice, vkCommandPool, NULL);
        fprintf(fptr, "uninitialize(): vkDestroyCommandPool Successful!.\n");
        vkCommandPool = VK_NULL_HANDLE;
    }

    // Destroy Vulkan Swapchain Image Views
    if(swapchainImageView_array) {
        for(uint32_t i = 0; i < swapchainImageCount; i++) {
            if(swapchainImageView_array[i]) {
                vkDestroyImageView(vkDevice, swapchainImageView_array[i], NULL);
                fprintf(fptr, "uninitialize(): vkDestroyImageView() Succeed for {%d}\n", i);
                swapchainImageView_array[i] = VK_NULL_HANDLE;
            }
        }
    }

    if(swapchainImageView_array) {
        free(swapchainImageView_array);
        fprintf(fptr, "uninitialize(): freed swapchainImageView_array!.\n");
        swapchainImageView_array = NULL;
    }

    // Destroy vulkan Images
    // VALIDATION USE CASE 4: uncomment the given block to see the error
    /* if(swapchainImage_array) {
        for(uint32_t i = 0; i < swapchainImageCount; i++) {
            if(swapchainImage_array[i]) {
                vkDestroyImage(vkDevice, swapchainImage_array[i], NULL);
                fprintf(fptr, "uninitialize(): vkDestroyImage() Succeed for {%d}\n", i);
                fflush(fptr);
                swapchainImage_array[i] = VK_NULL_HANDLE;
            }
        }
    } */

    if(swapchainImage_array) {
        free(swapchainImage_array);
        fprintf(fptr, "uninitialize(): freed swapchainImage_array!.\n");
        swapchainImage_array = NULL;
    }


    // Destroy Vulkan Swapchain
    if(vkSwapchainKHR) {
        vkDestroySwapchainKHR(vkDevice, vkSwapchainKHR, NULL);
        vkSwapchainKHR = VK_NULL_HANDLE;
    }
    
    // No need to destroy device queue

    // Destroy Vulkan Device
    if(vkDevice) {
        vkDestroyDevice(vkDevice, NULL);
        vkDevice = VK_NULL_HANDLE;
    }
    
    //No need to destroy selected physical device!

    // destroy surface
    if(vkSurfaceKHR) {
        vkDestroySurfaceKHR(vkInstance, vkSurfaceKHR, NULL);
        vkSurfaceKHR = VK_NULL_HANDLE;
		fprintf(fptr,"uninitialize(): vkDestroySurfaceKHR() Succeed\n");
    }

    if(vkDebugReportCallbackEXT && vkDestroyDebugReportCallbackEXT_fnptr) {
        vkDestroyDebugReportCallbackEXT_fnptr(vkInstance, vkDebugReportCallbackEXT, NULL);
        vkDebugReportCallbackEXT = VK_NULL_HANDLE;
        vkDestroyDebugReportCallbackEXT_fnptr = NULL;
        fprintf(fptr,"uninitialize(): vkDestroyDebugReportCallbackEXT_fnptr() Succeed\n");
    }

    // destroy vkInstance
    if(vkInstance) {
        vkDestroyInstance(vkInstance, NULL);
        vkInstance = VK_NULL_HANDLE;
		fprintf(fptr,"uninitialize(): vkDestroyInstance() Succeed\n");
    }

    if(fptr) {
        fprintf(fptr, "uninitialize(): File Closed Successfully..\n");
        fclose(fptr);
        fptr = NULL;
    }
}


//! //////////////////////////////////////// Definations of vulkan Related Functions ///////////////////////////////////////////////

VkResult createVulkanInstance (void) {
    // function declarations
    VkResult fillInstanceExtensionNames(void);
    VkResult fillValidationLayerNames(void);
    VkResult createValidationCallbackFunction(void);

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

    if(bValidation == True) {
        //fill validation layers names
        vkResult = fillValidationLayerNames();
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "createVulkanInstance(): fillValidationLayerNames() Failed!.\n");
            return (vkResult);
        } else {
            fprintf(fptr, "createVulkanInstance(): fillValidationLayerNames() Successful!.\n");
        }
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

    // if validation layer is enabled/valid then fill data else keep it null
    if(bValidation == True) {
        vkInstanceCreateInfo.enabledLayerCount = enabledValidationLayerCount;
        vkInstanceCreateInfo.ppEnabledLayerNames = enabledValidationLayerNames_array;
    } else {
        vkInstanceCreateInfo.enabledLayerCount = 0;
        vkInstanceCreateInfo.ppEnabledLayerNames = NULL;
    }

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

    // Step 5: Create Validation Layer Callback Function [ Do this for validation callbaaks ]
    if(bValidation == True) {
        vkResult = createValidationCallbackFunction();
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "createVulkanInstance(): createValidationCallbackFunction() Failed!.\n");
            return (vkResult);
        } else {
            fprintf(fptr, "createVulkanInstance(): createValidationCallbackFunction() Successful!.\n\n");
        }
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

    // Step 3: fill all supoorted extensions names in array of char pointers
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
    VkBool32 debugReportExtensionFound = VK_FALSE;
    for(uint32_t i = 0; i < instanceExtensionCount; i++) {
        if(strcmp(instanceExtensionNames_array[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            surfaceExtensionFound = VK_TRUE;
            enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
        }
        if(strcmp(instanceExtensionNames_array[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) ==  0) {
            win32vulkanSurfaceExtensionFound = VK_TRUE;
            enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
        }
        if(strcmp(instanceExtensionNames_array[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) ==  0) {
            debugReportExtensionFound = VK_TRUE;
            if(bValidation == True) {
                enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            } else {
                // array will not have entry of VK_EXT_DEBUG_REPORT_EXTENSION_NAME
            }
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

    if(debugReportExtensionFound == VK_FALSE) {
        if(bValidation == True) {
            vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
            fprintf(fptr, "fillInstanceExtensionNames(): Validation is ON but VK_EXT_DEBUG_REPORT_EXTENSION_NAME Not Supported!.\n");
            return (vkResult);
        } else {
            fprintf(fptr, "fillInstanceExtensionNames(): Validation is OFF and VK_EXT_DEBUG_REPORT_EXTENSION_NAME Not Supported!.\n");
        }
    } else {
        if(bValidation == True) {
            fprintf(fptr, "fillInstanceExtensionNames(): Validation is ON but VK_EXT_DEBUG_REPORT_EXTENSION_NAME is Supported!.\n");
        } else {
            fprintf(fptr, "fillInstanceExtensionNames(): Validation is OFF and VK_EXT_DEBUG_REPORT_EXTENSION_NAME is Supported!.\n");
        }
    }

    // step 8:
    for(uint32_t i = 0; i < enabledInstanceExtensionCount; i++) {
        fprintf(fptr, "fillInstanceExtensionNames(): Enabled Vulkan Instance Extension Name = %s \n", enabledInstanceExtensionNames_array[i]);
    }

    return vkResult;
}

VkResult fillValidationLayerNames(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // code
    // step 1: Find how many validation layers are supported by this vulkan driver & keep it in local variable
    uint32_t validationLayerCount = 0;

    vkResult = vkEnumerateInstanceLayerProperties(&validationLayerCount, NULL);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "fillValidationLayerNames(): vkEnumerateInstanceLayerProperties() First Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillValidationLayerNames(): vkEnumerateInstanceLayerProperties() First Call Successful!.\n");
    }

    // step 2: Allocate & fill struct vk Validation Layers array correspoinding to above acount
    VkLayerProperties *vkLayerProperties_array = NULL;
    vkLayerProperties_array = (VkLayerProperties*)malloc(sizeof(VkLayerProperties) * validationLayerCount);
    vkResult = vkEnumerateInstanceLayerProperties(&validationLayerCount, vkLayerProperties_array);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "fillValidationLayerNames(): vkEnumerateInstanceLayerProperties() Second Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillValidationLayerNames(): vkEnumerateInstanceLayerProperties() Second Call Successful!.\n");
    }

    // step 3: fill all supoorted layers names in array of char pointers
    char **validationLayerNames_array = NULL;
    validationLayerNames_array = (char**)malloc(sizeof(char*) * validationLayerCount);
    for(uint32_t i = 0; i < validationLayerCount; i++) {
        validationLayerNames_array[i] = (char*)malloc(sizeof(char) * strlen(vkLayerProperties_array[i].layerName) + 1);
        memcpy(
            validationLayerNames_array[i], 
            vkLayerProperties_array[i].layerName, 
            strlen(vkLayerProperties_array[i].layerName) + 1
        );
        fprintf(fptr, "fillValidationLayerNames(): Vulkan Validation Layer Name = %s \n", validationLayerNames_array[i]);
    }

    // step 4: free vkLayerProperties_array
    free(vkLayerProperties_array);

    // step 5: check if validation layer is supported or not
    VkBool32 validationLayerFound = VK_FALSE;
    for(uint32_t i = 0; i < validationLayerCount; i++) {
        if(strcmp(validationLayerNames_array[i], "VK_LAYER_KHRONOS_validation") == 0) {
            validationLayerFound = VK_TRUE;
            enabledValidationLayerNames_array[enabledValidationLayerCount++] = "VK_LAYER_KHRONOS_validation";
        }
    }

    // step 6
    for(uint32_t i = 0; i < validationLayerCount; i++) {
        free(validationLayerNames_array[i]);
    }
    free(validationLayerNames_array);

    // step 7
    if(validationLayerFound == VK_FALSE) {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
        fprintf(fptr, "fillValidationLayerNames(): VK_LAYER_KHRONOS_validation Not Supported!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillValidationLayerNames(): VK_LAYER_KHRONOS_validation Supported!.\n");
    }

    // step 8: print all the supported layers
    for(uint32_t i = 0; i < enabledValidationLayerCount; i++) {
        fprintf(fptr, "fillValidationLayerNames(): Enabled Vulkan Validation Layer Name = %s \n", enabledValidationLayerNames_array[i]);
    }

    return (vkResult);
}

VkResult createValidationCallbackFunction(void) {
    // function declaratons
    VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
        VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT,
        uint64_t, size_t, int32_t, const char*, const char*,
        void*
    );

    // variables
    VkResult vkResult = VK_SUCCESS;
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT_fnptr = NULL;

    // code
    // get the required function pointers
    vkCreateDebugReportCallbackEXT_fnptr = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugReportCallbackEXT");
    if(vkCreateDebugReportCallbackEXT_fnptr == NULL) {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
        fprintf(fptr, "createValidationCallbackFunction(): vkGetInstanceProcAddr() for vkCreateDebugReportCallbackEXT Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createValidationCallbackFunction(): vkGetInstanceProcAddr() for vkCreateDebugReportCallbackEXT Successful!.\n");
    }

    vkDestroyDebugReportCallbackEXT_fnptr = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT");
    if(vkCreateDebugReportCallbackEXT_fnptr == NULL) {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
        fprintf(fptr, "createValidationCallbackFunction(): vkGetInstanceProcAddr() for vkDestroyDebugReportCallbackEXT Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createValidationCallbackFunction(): vkGetInstanceProcAddr() for vkDestroyDebugReportCallbackEXT Successful!.\n");
    }

    // fill struct VkDebugReportCallbackCreateInfoEXT to get vulkan debug report callback object
    VkDebugReportCallbackCreateInfoEXT vkDebugReportCallbackCreateInfoEXT;
    memset((void*)&vkDebugReportCallbackCreateInfoEXT, 0, sizeof(VkDebugReportCallbackCreateInfoEXT));

    vkDebugReportCallbackCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    vkDebugReportCallbackCreateInfoEXT.pNext = NULL;
    vkDebugReportCallbackCreateInfoEXT.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    vkDebugReportCallbackCreateInfoEXT.pfnCallback = debugReportCallback;
    vkDebugReportCallbackCreateInfoEXT.pUserData = NULL;

    vkResult = vkCreateDebugReportCallbackEXT_fnptr(vkInstance, &vkDebugReportCallbackCreateInfoEXT, NULL, &vkDebugReportCallbackEXT);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createValidationCallbackFunction(): vkCreateDebugReportCallbackEXT_fnptr() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createValidationCallbackFunction(): vkCreateDebugReportCallbackEXT_fnptr() Successful!.\n");
    }

    return (vkResult);
}

// get supported surface
VkResult getSupportedSurface(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    //code
    VkXlibSurfaceCreateInfoKHR vkXlibSurfaceCreateInfoKHR;
    memset((void*)&vkXlibSurfaceCreateInfoKHR, 0, sizeof(VkXlibSurfaceCreateInfoKHR));

    vkXlibSurfaceCreateInfoKHR.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    vkXlibSurfaceCreateInfoKHR.pNext = NULL;
    vkXlibSurfaceCreateInfoKHR.flags = 0;
    // vkXlibSurfaceCreateInfoKHR.dpy = XOpenDisplay(NULL);
    vkXlibSurfaceCreateInfoKHR.dpy = gpDisplay;
    vkXlibSurfaceCreateInfoKHR.window = gWindow;

    vkResult = vkCreateXlibSurfaceKHR(
        vkInstance, 
        &vkXlibSurfaceCreateInfoKHR,
        NULL,
        &vkSurfaceKHR
    );

    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getSupportedSurface(): vkCreateXlibSurfaceKHR() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "getSupportedSurface(): vkCreateXlibSurfaceKHR() Successful!.\n");
    }


    return vkResult;
}

VkResult getPhysicalDevice() {
    // variables
    VkResult vkResult = VK_SUCCESS;

    //code
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, NULL);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getPhysicalDevice(): vkEnumeratePhysicalDevices() First Call Failed!.\n");
        return (vkResult);
    } else if(physicalDeviceCount == 0) {
        fprintf(fptr, "getPhysicalDevice(): vkEnumeratePhysicalDevices() Resulted in Zero Physical Devices!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } else {
        fprintf(fptr, "getPhysicalDevice(): vkEnumeratePhysicalDevices() First Call Successful!.\n");
    }

    vkPhysicalDevice_array = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);

    vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, vkPhysicalDevice_array);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getPhysicalDevice(): vkEnumeratePhysicalDevices() Second Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "getPhysicalDevice(): vkEnumeratePhysicalDevices() Second Call Successful!.\n");
    }

    VkBool32 bFound = VK_FALSE;

    for(uint32_t i = 0; i < physicalDeviceCount; i++) {
        uint32_t queueCount = UINT32_MAX;

        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_array[i], &queueCount, NULL);
        VkQueueFamilyProperties *vkQueueFamilyProperties_array = NULL;
        vkQueueFamilyProperties_array = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_array[i], &queueCount, vkQueueFamilyProperties_array);

        VkBool32 *isQueueSurfaceSupported_array = NULL;
        isQueueSurfaceSupported_array = (VkBool32*)malloc(sizeof(VkBool32) * queueCount);

        for(uint32_t j = 0; j < queueCount; j++) {
            vkGetPhysicalDeviceSurfaceSupportKHR(
                vkPhysicalDevice_array[i],
                j,
                vkSurfaceKHR,
                &isQueueSurfaceSupported_array[j]
            );
        }

        for(uint32_t j = 0; j < queueCount; j++) {
            if(vkQueueFamilyProperties_array[j].queueFlags & VK_QUEUE_GRAPHICS_BIT
                && isQueueSurfaceSupported_array[j] == VK_TRUE) {
                vkPhysicalDevice_selected = vkPhysicalDevice_array[i];
                graphicsQueueFamilyIndex_selected = j;
                bFound = VK_TRUE;
                break;
            }
        }

        if(isQueueSurfaceSupported_array) {
            free(isQueueSurfaceSupported_array);
            isQueueSurfaceSupported_array = NULL;
            fprintf(fptr, "getPhysicalDevice(): freed isQueueSurfaceSupported_array!.\n");
        }
        
        if(vkQueueFamilyProperties_array) {
            free(vkQueueFamilyProperties_array);
            vkQueueFamilyProperties_array = NULL;
            fprintf(fptr, "getPhysicalDevice(): freed vkQueueFamilyProperties_array!.\n");
        }

        if(bFound == VK_TRUE) {
            break;
        }
    }


    if(bFound == VK_TRUE) {
        fprintf(fptr, "getPhysicalDevice(): Successful to get required graphics enabled physical device!.\n");
    } else {
        if(vkPhysicalDevice_array) {
            free(vkPhysicalDevice_array);
            vkPhysicalDevice_array = NULL;
            fprintf(fptr, "getPhysicalDevice(): freed vkPhysicalDevice_array!.\n");
        }
        fprintf(fptr, "getPhysicalDevice(): Failed to get required graphics enabled physical device!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    }

    memset((void*)&vkPhysicalDeviceMemoryProperties, 0, sizeof(VkPhysicalDeviceMemoryProperties));

    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice_selected, &vkPhysicalDeviceMemoryProperties);

    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
    memset((void*)&vkPhysicalDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

    vkGetPhysicalDeviceFeatures(vkPhysicalDevice_selected, &vkPhysicalDeviceFeatures);

    if(vkPhysicalDeviceFeatures.tessellationShader == VK_TRUE) {
        fprintf(fptr, "getPhysicalDevice(): Selected Physical Device Supports Tessellation Shader!.\n");
    } else {
        fprintf(fptr, "getPhysicalDevice(): Selected Physical Device Does Not Supports Tessellation Shader!.\n");
    }

    if(vkPhysicalDeviceFeatures.geometryShader == VK_TRUE) {
        fprintf(fptr, "getPhysicalDevice(): Selected Physical Device Supports Geometry Shader!.\n");
    } else {
        fprintf(fptr, "getPhysicalDevice(): Selected Physical Device Does Not Supports Geometry Shader!.\n");
    }

    return (vkResult);
}

VkResult printVKInfo (void) {
    // varibales
    VkResult vkResult = VK_SUCCESS;

    // code
    fprintf(fptr, "\n\nprintVKInfo(): Printing Vulkan Info: \n\n");

    for(uint32_t i = 0; i < physicalDeviceCount; i++) {

        VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
        memset((void*)&vkPhysicalDeviceProperties, 0, sizeof(VkPhysicalDeviceProperties));

        vkGetPhysicalDeviceProperties(vkPhysicalDevice_array[i], &vkPhysicalDeviceProperties);

        uint32_t majorVersion = VK_API_VERSION_MAJOR(vkPhysicalDeviceProperties.apiVersion);
        uint32_t minorVersion = VK_API_VERSION_MINOR(vkPhysicalDeviceProperties.apiVersion);
        uint32_t patchVersion = VK_API_VERSION_PATCH(vkPhysicalDeviceProperties.apiVersion);

        fprintf(fptr, "Physical Device [%d] Properties: \n", i);
        // API Version
        fprintf(fptr, "API Version: %d.%d.%d\n", majorVersion, minorVersion, patchVersion);
        //Device Name
        fprintf(fptr, "Device Name: %s\n", vkPhysicalDeviceProperties.deviceName);
        
        switch(vkPhysicalDeviceProperties.deviceType) {
    
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                fprintf(fptr, "Device Type: Integrated GPU (iGPU)\n");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                fprintf(fptr, "Device Type: Discrete GPU (dGPU)\n");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                fprintf(fptr, "Device Type: Virtual GPU (vGPU)\n");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                fprintf(fptr, "Device Type: GPU\n");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                fprintf(fptr, "Device Type: Nor iGPU, dGPU, vGPU, or CPU, Something Other\n");
                break;

            default:
                fprintf(fptr, "Device Type: Unknown\n");
                break;
        }

        // Vendor ID
        fprintf(fptr, "Vendor ID: 0x%04x\n", vkPhysicalDeviceProperties.vendorID);

        // Device ID
        fprintf(fptr, "Device ID: 0x%04x\n", vkPhysicalDeviceProperties.deviceID);

        fprintf(fptr, "\n");
    }

    if(vkPhysicalDevice_array) {
        free(vkPhysicalDevice_array);
        vkPhysicalDevice_array = NULL;
        fprintf(fptr, "printVKInfo(): freed vkPhysicalDevice_array!.\n");
    }

    return (vkResult);
}

VkResult fillDeviceExtensionNames (void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // Step 1: Find how many devices extension are supported by this vulkan driver & keep it in local variable
    uint32_t devicesExtensionCount = 0;

    vkResult = vkEnumerateDeviceExtensionProperties(vkPhysicalDevice_selected, NULL, &devicesExtensionCount, NULL);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "fillDeviceExtensionNames(): vkEnumerateDeviceExtensionProperties() First Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillDeviceExtensionNames(): vkEnumerateDeviceExtensionProperties() First Call Successful!.\n");
    }

    // step 2: Allocate & fill struct vk Extenstions array correspoinding to above count
    VkExtensionProperties *vkExtensionProperties_array = NULL;
    vkExtensionProperties_array = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * devicesExtensionCount);
    vkResult = vkEnumerateDeviceExtensionProperties(vkPhysicalDevice_selected, NULL, &devicesExtensionCount, vkExtensionProperties_array);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "fillDeviceExtensionNames(): vkEnumerateDeviceExtensionProperties() Second Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillDeviceExtensionNames(): vkEnumerateDeviceExtensionProperties() Second Call Successful!.\n");
    }

    // Step 3: 
    char **deviceExtensionNames_array = NULL;
    deviceExtensionNames_array = (char**)malloc(sizeof(char*) * devicesExtensionCount);
    fprintf(fptr, "fillDeviceExtensionNames(): Vulkan Device Extension Count = %d \n", devicesExtensionCount);
    for(uint32_t i = 0; i < devicesExtensionCount; i++) {
        deviceExtensionNames_array[i] = (char*)malloc(sizeof(char) * strlen(vkExtensionProperties_array[i].extensionName) + 1);
        memcpy(
            deviceExtensionNames_array[i], 
            vkExtensionProperties_array[i].extensionName, 
            strlen(vkExtensionProperties_array[i].extensionName) + 1
        );
        fprintf(fptr, "fillDeviceExtensionNames(): Vulkan Device Extension Name = %s \n", deviceExtensionNames_array[i]);
    }

    fprintf(fptr, "\n");


    // step 4:
    free(vkExtensionProperties_array);

    // step 5
    VkBool32 vulkanSwapchainExtensionFound = VK_FALSE;
    for(uint32_t i = 0; i < devicesExtensionCount; i++) {
        if(strcmp(deviceExtensionNames_array[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            vulkanSwapchainExtensionFound = VK_TRUE;
            enabledDeviceExtensionNames_array[enabledDeviceExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        }
    }

    // step 6
    for(uint32_t i = 0; i < devicesExtensionCount; i++) {
        free(deviceExtensionNames_array[i]);
    }
    free(deviceExtensionNames_array);

    // step 7:
    if(vulkanSwapchainExtensionFound == VK_FALSE) {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
        fprintf(fptr, "fillDeviceExtensionNames(): VK_KHR_SWAPCHAIN_EXTENSION_NAME Not Found!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "fillDeviceExtensionNames(): VK_KHR_SWAPCHAIN_EXTENSION_NAME Found!.\n");
    }

    // step 8:
    for(uint32_t i = 0; i < enabledDeviceExtensionCount; i++) {
        fprintf(fptr, "fillDeviceExtensionNames(): Enabled Vulkan Device Extension Name = %s \n", enabledDeviceExtensionNames_array[i]);
    }

    return vkResult;
}


VkResult createVulkanDevice () {
    
    // function definations
    VkResult fillDeviceExtensionNames(void);

    //variables
    VkResult vkResult = VK_SUCCESS;

    vkResult = fillDeviceExtensionNames();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVulkanDevice(): fillDeviceExtensionNames() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createVulkanDevice(): fillDeviceExtensionNames() Successful!.\n");
    }

    // !NEWLY ADDED CODE : intialize VkDeviceQueueCreateInfo
    float queuePriorities[] = { 1.0f };
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo;
    memset((void*)&vkDeviceQueueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));

    vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vkDeviceQueueCreateInfo.pNext = 0;
    vkDeviceQueueCreateInfo.flags = 0;
    vkDeviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex_selected;
    vkDeviceQueueCreateInfo.queueCount = 1;
    vkDeviceQueueCreateInfo.pQueuePriorities = queuePriorities;

    // initialize VkDeviceCreateInfo structure
    VkDeviceCreateInfo vkDeviceCreateInfo;
    memset((void*)&vkDeviceCreateInfo, 0, sizeof(VkDeviceCreateInfo));

    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.pNext = NULL;
    vkDeviceCreateInfo.flags = 0;
    vkDeviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionCount;
    vkDeviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames_array;
    vkDeviceCreateInfo.enabledLayerCount = 0; // these are deprecated in current version
    vkDeviceCreateInfo.ppEnabledLayerNames = NULL; // these are deprecated in current version
    vkDeviceCreateInfo.pEnabledFeatures = NULL;
    // !NEWLY ADDED CODE : set VkDeviceQueueCreateInfo
    vkDeviceCreateInfo.queueCreateInfoCount = 1;
    vkDeviceCreateInfo.pQueueCreateInfos = &vkDeviceQueueCreateInfo;

    vkResult = vkCreateDevice(
        vkPhysicalDevice_selected,
        &vkDeviceCreateInfo,
        NULL, &vkDevice
    );

    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVulkanDevice(): vkCreateDevice() Failed!. (%d)\n", vkResult);
        return (vkResult);
    } else {
        fprintf(fptr, "createVulkanDevice(): vkCreateDevice() Successful!.\n");
    }

    return(vkResult);
}

void getDeviceQueue (void) {

    vkGetDeviceQueue(
        vkDevice,
        graphicsQueueFamilyIndex_selected,
        0, &vkQueue
    );

    if(vkQueue == VK_NULL_HANDLE) {
        fprintf(fptr, "getDeviceQueue(): vkGetDeviceQueue() Failed!.\n");
    } else {
        fprintf(fptr, "getDeviceQueue(): vkGetDeviceQueue() Successful!.\n");
    }

}

VkResult getPhysicalDeviceSurfaceFormatAndColorSpace (void) {
    
    //variables
    VkResult vkResult = VK_SUCCESS;
    uint32_t formatCount = 0;

    // code

    // get the count of supported color formats
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
        vkPhysicalDevice_selected, 
        vkSurfaceKHR, &formatCount,
        NULL
    );

    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getPhysicalDeviceSurfaceFormatAndColorSpace(): vkGetPhysicalDeviceSurfaceFormatsKHR() frist Call Failed!.\n");
    } else if(formatCount == 0) {
        fprintf(fptr, "getPhysicalDeviceSurfaceFormatAndColorSpace(): vkGetPhysicalDeviceSurfaceFormatsKHR() Failed: 0 supported formats found!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return(vkResult);
    } else {
        fprintf(fptr, "getPhysicalDeviceSurfaceFormatAndColorSpace(): vkGetPhysicalDeviceSurfaceFormatsKHR() first Call Successful!. [Found %d Formats]\n", formatCount);
    }

    VkSurfaceFormatKHR *vkSurfaceFormatKHR_array = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    
    // fill the allocated array with supported formats
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
        vkPhysicalDevice_selected, 
        vkSurfaceKHR, &formatCount,
        vkSurfaceFormatKHR_array
    );
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getPhysicalDeviceSurfaceFormatAndColorSpace(): vkGetPhysicalDeviceSurfaceFormatsKHR() Second Call Failed!.\n");
    } else {
        fprintf(fptr, "getPhysicalDeviceSurfaceFormatAndColorSpace(): vkGetPhysicalDeviceSurfaceFormatsKHR() Second Call Successful!.\n");
    }

    // Decide the surface color format first!
    if(formatCount == 1 && vkSurfaceFormatKHR_array[0].format == VK_FORMAT_UNDEFINED) {
        vkFormat_color = VK_FORMAT_B8G8R8G8_422_UNORM;
    } else {
        vkFormat_color = vkSurfaceFormatKHR_array[0].format;
    }

    // Decide the Color Space
    vkColorSpaceKHR = vkSurfaceFormatKHR_array[0].colorSpace;

    if(vkSurfaceFormatKHR_array) {
        free(vkSurfaceFormatKHR_array);
        vkSurfaceFormatKHR_array = NULL;
        fprintf(fptr, "getPhysicalDeviceSurfaceFormatAndColorSpace(): vkSurfaceFormatKHR_array freed.\n");
    }

    return (vkResult);
}

VkResult getPhysicalDeviceSurfacePresentMode(void) {

    // Variables
    VkResult vkResult = VK_SUCCESS;
    uint32_t modeCount = 0;

    //code
    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(
        vkPhysicalDevice_selected, 
        vkSurfaceKHR, &modeCount,
        NULL
    );
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): vkGetPhysicalDeviceSurfacePresentModesKHR() frist Call Failed!.\n");
    } else if(modeCount == 0) {
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): vkGetPhysicalDeviceSurfacePresentModesKHR() Failed: 0 supported modes found!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return(vkResult);
    } else {
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): vkGetPhysicalDeviceSurfacePresentModesKHR() first Call Successful!. [Found %d Present Modes]\n", modeCount);
    }

    VkPresentModeKHR *vkPresentModeKHR_array = (VkPresentModeKHR*)malloc(modeCount * sizeof(VkPresentModeKHR));

    // fill the allocated array with supported present modes
    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(
        vkPhysicalDevice_selected, 
        vkSurfaceKHR, &modeCount,
        vkPresentModeKHR_array
    );
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): vkGetPhysicalDeviceSurfacePresentModesKHR() Second Call Failed!.\n");
    } else {
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): vkGetPhysicalDeviceSurfacePresentModesKHR() Second Call Successful!.\n");
    }

    for(uint32_t i = 0 ;  i < modeCount; i++) {
        if(vkPresentModeKHR_array[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            vkPresentModeKHR = vkPresentModeKHR_array[i];
            fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): VK_PRESENT_MODE_MAILBOX_KHR Present Mode found!.\n");
            break;
        }
    }

    if(vkPresentModeKHR != VK_PRESENT_MODE_MAILBOX_KHR) {
        // since we don't have mailbox as supported format let's settle for FIFO then!
        vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): Present Mode set to VK_PRESENT_MODE_FIFO_KHR!.\n");
    }

    if(vkPresentModeKHR_array) {
        free(vkPresentModeKHR_array);
        vkPresentModeKHR_array = NULL;
        fprintf(fptr, "getPhysicalDeviceSurfacePresentMode(): vkPresentModeKHR_array freed.\n");
    }

    return (vkResult);
}

VkResult createSwapchain (VkBool32 vSync) {

    // Functions
    VkResult getPhysicalDeviceSurfaceFormatAndColorSpace(void);
    VkResult getPhysicalDeviceSurfacePresentMode(void);

    // Variables
    VkResult vkResult = VK_SUCCESS;
    
    // Code
    // Surface Color & Color Space
    vkResult = getPhysicalDeviceSurfaceFormatAndColorSpace();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSwapchain(): getPhysicalDeviceSurfaceFormatAndColorSpace() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSwapchain(): getPhysicalDeviceSurfaceFormatAndColorSpace() Successful!.\n");
    }

    // Get Physical Device Surface Capabilities
    VkSurfaceCapabilitiesKHR vkSurfaceCapabilitiesKHR;
    memset((void*)&vkSurfaceCapabilitiesKHR, 0, sizeof(VkSurfaceCapabilitiesKHR));

    vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice_selected, vkSurfaceKHR, &vkSurfaceCapabilitiesKHR);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSwapchain(): vkGetPhysicalDeviceSurfaceCapabilitiesKHR() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSwapchain(): vkGetPhysicalDeviceSurfaceCapabilitiesKHR() Successful!.\n");
    }

    // Decide Image Count of Swapchain using minImageCount & maxImageCount from vkSurfaceCapabilitiesKHR
    uint32_t testingNumberOfSwapchainImages = vkSurfaceCapabilitiesKHR.minImageCount + 1;
    uint32_t desiredNumberOfSwapchainImages = 0;

    if(vkSurfaceCapabilitiesKHR.maxImageCount > 0 && vkSurfaceCapabilitiesKHR.maxImageCount < testingNumberOfSwapchainImages) {
        desiredNumberOfSwapchainImages = vkSurfaceCapabilitiesKHR.maxImageCount;
    } else {
        desiredNumberOfSwapchainImages = vkSurfaceCapabilitiesKHR.minImageCount;
    }

    fprintf(
        fptr, 
        "createSwapchain(): desiredNumberOfSwapchainImages is : %d, [Min: %d, Max: %d]\n", 
        desiredNumberOfSwapchainImages,  vkSurfaceCapabilitiesKHR.minImageCount,  
        vkSurfaceCapabilitiesKHR.maxImageCount
    );

    // Decide Size of Swapchain Image using currentExtent Size & window Size
    memset((void*)&vkExtent2D_swapchain, 0, sizeof(VkExtent2D));

    if(vkSurfaceCapabilitiesKHR.currentExtent.width != UINT32_MAX) {
        vkExtent2D_swapchain.width = vkSurfaceCapabilitiesKHR.currentExtent.width;
        vkExtent2D_swapchain.height = vkSurfaceCapabilitiesKHR.currentExtent.height;

        fprintf(
            fptr, 
            "createSwapchain(): Swapchain Image Width : %d X Height : %d\n", 
            vkExtent2D_swapchain.width, vkExtent2D_swapchain.height
        );
    } else {
        // if surface size is already defined then swapchain image size must match with it!
        VkExtent2D vkExtent2D;
        memset((void*)&vkExtent2D, 0, sizeof(VkExtent2D));

        vkExtent2D.width = (uint32_t)winWidth;
        vkExtent2D.height = (uint32_t)winHeight;

        vkExtent2D_swapchain.width = max(vkSurfaceCapabilitiesKHR.minImageExtent.width, min(vkSurfaceCapabilitiesKHR.maxImageExtent.width, vkExtent2D.width));
        vkExtent2D_swapchain.height = max(vkSurfaceCapabilitiesKHR.minImageExtent.height, min(vkSurfaceCapabilitiesKHR.maxImageExtent.height, vkExtent2D.height));

        fprintf(
            fptr, 
            "createSwapchain(): Swapchain Image (Derived from best of minImageExtent, maxImageExtent & Window Size) Width  : %d X Height : %d\n", 
            vkExtent2D_swapchain.width, vkExtent2D_swapchain.height
        );
    }

    // Set Swapchain Image Usage Flag
    VkImageUsageFlags vkImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // Whether to Consider Pre-Transform/Flipping or Not
    VkSurfaceTransformFlagBitsKHR vkSurfaceTransformFlagBitsKHR;

    if(vkSurfaceCapabilitiesKHR.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        vkSurfaceTransformFlagBitsKHR = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        vkSurfaceTransformFlagBitsKHR = vkSurfaceCapabilitiesKHR.currentTransform;
    }

    // Physical Device Presentation Mode
    vkResult = getPhysicalDeviceSurfacePresentMode();
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSwapchain(): getPhysicalDeviceSurfacePresentMode() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSwapchain(): getPhysicalDeviceSurfacePresentMode() Successful!.\n");
    }

    // Initalize VkSwapchainCreateInfoKHR
    VkSwapchainCreateInfoKHR vkSwapchainCreateInfoKHR;
    memset((void*)&vkSwapchainCreateInfoKHR, 0, sizeof(VkSwapchainCreateInfoKHR));

    vkSwapchainCreateInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkSwapchainCreateInfoKHR.pNext = NULL;
    vkSwapchainCreateInfoKHR.flags = 0;
    vkSwapchainCreateInfoKHR.surface = vkSurfaceKHR;
    vkSwapchainCreateInfoKHR.minImageCount = desiredNumberOfSwapchainImages;
    vkSwapchainCreateInfoKHR.imageFormat = vkFormat_color;
    vkSwapchainCreateInfoKHR.imageColorSpace = vkColorSpaceKHR;
    vkSwapchainCreateInfoKHR.imageExtent.width = vkExtent2D_swapchain.width;
    vkSwapchainCreateInfoKHR.imageExtent.height = vkExtent2D_swapchain.height;
    vkSwapchainCreateInfoKHR.imageUsage = vkImageUsageFlags;
    vkSwapchainCreateInfoKHR.preTransform = vkSurfaceTransformFlagBitsKHR;
    vkSwapchainCreateInfoKHR.imageArrayLayers = 1;
    vkSwapchainCreateInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkSwapchainCreateInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkSwapchainCreateInfoKHR.presentMode = vkPresentModeKHR;
    vkSwapchainCreateInfoKHR.clipped = VK_TRUE;

    vkResult = vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreateInfoKHR, NULL, &vkSwapchainKHR);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSwapchain(): vkCreateSwapchainKHR() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSwapchain(): vkCreateSwapchainKHR() Successful!.\n");
    }

    return (vkResult);
}

VkResult createSwapchainImagesAndImageViews(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // code
    // Step 1: Get Swapchain Image Count
    vkResult = vkGetSwapchainImagesKHR(vkDevice, vkSwapchainKHR, &swapchainImageCount, NULL);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSwapchainImagesAndImageViews(): vkGetSwapchainImagesKHR() First Call Failed!.\n");
        return (vkResult);
    } else if( swapchainImageCount == 0) {
        fprintf(fptr, "createSwapchainImagesAndImageViews(): vkGetSwapchainImagesKHR() Failed: 0 Swapchain Images found!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } else {
        fprintf(fptr, "createSwapchainImagesAndImageViews(): vkGetSwapchainImagesKHR() Successful!. : Swapchain Image Count : [%d]\n", swapchainImageCount);
    }

    // Step 2: Allocate Swapchain Image Array
    swapchainImage_array = (VkImage*)malloc(sizeof(VkImage) * swapchainImageCount);

    // Step 3: Fill Swapchain Image Array
    vkResult = vkGetSwapchainImagesKHR(vkDevice, vkSwapchainKHR, &swapchainImageCount, swapchainImage_array);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSwapchainImagesAndImageViews(): vkGetSwapchainImagesKHR() Second Call Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSwapchainImagesAndImageViews(): vkGetSwapchainImagesKHR() Second Call Successful!.\n");
    }

    // Setp 4: Allocate Swapchain Image Views Array
    swapchainImageView_array = (VkImageView*)malloc(sizeof(VkImageView) * swapchainImageCount);

    // Step 5: vkCreateImageView for each Swapchain Image
    VkImageViewCreateInfo vkImageViewCreateInfo;
    memset((void*)&vkImageViewCreateInfo, 0, sizeof(VkImageViewCreateInfo));
    
    vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkImageViewCreateInfo.pNext = NULL;
    vkImageViewCreateInfo.flags = 0;
    vkImageViewCreateInfo.format = vkFormat_color;
    vkImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    vkImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    vkImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    vkImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    vkImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    vkImageViewCreateInfo.subresourceRange.layerCount = 1;
    vkImageViewCreateInfo.subresourceRange.levelCount = 1;
    vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    // Step 6: Fill Imafe view Array  using above struct
    for(uint32_t i = 0; i < swapchainImageCount; i++) {
        vkImageViewCreateInfo.image = swapchainImage_array[i];
        vkResult = vkCreateImageView(vkDevice, &vkImageViewCreateInfo, NULL, &swapchainImageView_array[i]);
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "createSwapchainImagesAndImageViews(): vkCreateImageView() Failed at {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "createSwapchainImagesAndImageViews(): vkCreateImageView() Successful for {%d}!.\n", i);
        }
    }

    return (vkResult);
}

VkResult createCommandPool(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    VkCommandPoolCreateInfo vkCommandPoolCreateInfo;
    memset((void*)&vkCommandPoolCreateInfo, 0, sizeof(vkCommandPoolCreateInfo));

    vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkCommandPoolCreateInfo.pNext = NULL;
    vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCommandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex_selected;

    vkCreateCommandPool(vkDevice, &vkCommandPoolCreateInfo, NULL, &vkCommandPool);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createCommandPool(): vkCreateCommandPool() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createCommandPool(): vkCreateCommandPool() Successful!.\n");
    }

    return (vkResult);
}

VkResult createCommandBuffers(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // Step 1: Init and Allocate VkCommandBufferAllocateInfo
    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo;
    memset((void*)&vkCommandBufferAllocateInfo, 0, sizeof(VkCommandBufferAllocateInfo));

    vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkCommandBufferAllocateInfo.pNext = NULL;
    vkCommandBufferAllocateInfo.commandPool = vkCommandPool;
    vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkCommandBufferAllocateInfo.commandBufferCount = 1;

    // Step 2: Allocate Command Buffer Array to the size of swapchainImageCount
    vkCommandBuffer_array = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * swapchainImageCount);

    // Step 3: Allocat eeach command buffer in loop with allocateInfo struct
    for(uint32_t i = 0; i < swapchainImageCount; i++) {
        vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer_array[i]);
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "createCommandBuffers(): vkCreatvkAllocateCommandBufferseImageView() Failed at {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "createCommandBuffers(): vkAllocateCommandBuffers() Successful for {%d}!.\n", i);
        }
    }

    return (vkResult);
}

VkResult createVertexBuffer(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // Step 1
    float triangle_position[] = {
        0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f
    };

    // Step 2
    memset((void*)&vertexData_position, 0, sizeof(VertexData));

    // Step 3
    VkBufferCreateInfo vkBufferCreateInfo;
    memset((void*)&vkBufferCreateInfo, 0, sizeof(VkBufferCreateInfo));

    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.pNext = NULL;
    vkBufferCreateInfo.flags = 0; // No flags, Valid Flags are used in scattered buffer
    vkBufferCreateInfo.size = sizeof(triangle_position);
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    // Setp 4
    vkResult = vkCreateBuffer(vkDevice, &vkBufferCreateInfo, NULL, &vertexData_position.vkBuffer);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVertexBuffer(): vkCreateBuffer() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createVertexBuffer(): vkCreateBuffer() Successful!.\n");
    }

    // Step 5
    VkMemoryRequirements vkMemoryRequirements;
    memset((void*)&vkMemoryRequirements, 0, sizeof(VkMemoryRequirements));

    vkGetBufferMemoryRequirements(vkDevice, vertexData_position.vkBuffer, &vkMemoryRequirements);

    // Step 6
    VkMemoryAllocateInfo vkMemoryAllocateInfo;
    memset((void*)&vkMemoryAllocateInfo, 0, sizeof(VkMemoryAllocateInfo));

    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.pNext = NULL;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = 0; // this will be set in next step

    // Step A 
    for(uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        // Step B
        if((vkMemoryRequirements.memoryTypeBits & 1) == 1) {
            // Step C
            if(vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                // Step D
                vkMemoryAllocateInfo.memoryTypeIndex = i;
                break;
            }
        }
        // Step E
        vkMemoryRequirements.memoryTypeBits >>= 1;
    }

    //Setp 9
    vkResult = vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, NULL, &vertexData_position.vkDeviceMemory);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVertexBuffer(): vkAllocateMemory() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createVertexBuffer(): vkAllocateMemory() Successful!.\n");
    }

    // Step 10
    vkResult = vkBindBufferMemory(vkDevice, vertexData_position.vkBuffer, vertexData_position.vkDeviceMemory, 0);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVertexBuffer(): vkBindBufferMemory() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createVertexBuffer(): vkBindBufferMemory() Successful!.\n");
    }

    // Step 11
    void *data = NULL;

    vkResult = vkMapMemory(
        vkDevice,
        vertexData_position.vkDeviceMemory,
        0,
        vkMemoryAllocateInfo.allocationSize,
        0,
        &data
    );

    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createVertexBuffer(): vkMapMemory() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createVertexBuffer(): vkMapMemory() Successful!.\n");
    }

    // Step 12
    memcpy(data, triangle_position, sizeof(triangle_position));

    // Step 13
    vkUnmapMemory(vkDevice, vertexData_position.vkDeviceMemory);

    return(vkResult);
}

VkResult createShaders(void) {

    // variables
    VkResult vkResult = VK_SUCCESS;

    // for vertex shader
    const char* szFileName = "shader.vert.spv";
    FILE *fp = NULL;
    size_t fileSize = 0;

    fp = fopen(szFileName, "rb");
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createShaders(): fopen() failed to open Vertex Shader spir-v file!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } else {
        fprintf(fptr, "createShaders(): fopen() succeed to open Vertex Shader spir-v file!.\n");
    }

    fseek(fp, 0l, SEEK_END);
    fileSize = ftell(fp);
    if(fileSize == 0) {
        fprintf(fptr, "createShaders(): ftell() gave file size 0.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } 
    fseek(fp, 0l, SEEK_SET);

    char *shaderData = (char*)malloc(fileSize * sizeof(char));

    size_t retVal = fread(shaderData, fileSize, 1, fp);
    if(retVal != 1) {
        fprintf(fptr, "createShaders(): fread() failed to read Vertex Shader file!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } else {
        fprintf(fptr, "createShaders(): fread() succeed to read Vertex Shader file!.\n");
    }
    fclose(fp);
    fp = NULL;

    VkShaderModuleCreateInfo vkShaderModuleCreateInfo;
    memset((void*)&vkShaderModuleCreateInfo, 0, sizeof(VkShaderModuleCreateInfo));

    vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderModuleCreateInfo.pNext = NULL;
    vkShaderModuleCreateInfo.flags = 0;
    vkShaderModuleCreateInfo.codeSize = fileSize;
    vkShaderModuleCreateInfo.pCode = (uint32_t*)shaderData;

    vkResult = vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, NULL, &vkShaderModule_vertex);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createShaders(): vkCreateShaderModule() for Vertex Shader Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createShaders(): vkCreateShaderModule() for Vertex Shader Successful!.\n");
    }

    if(shaderData) {
        free(shaderData);
        shaderData = NULL;
    }
    fprintf(fptr, "createShaders(): Vertex Shader Module Created Successful!.\n");

    // for fragment shader
    szFileName = "shader.frag.spv";
    fileSize = 0;

    fp = fopen(szFileName, "rb");
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createShaders(): fopen() failed to open Fragment Shader spir-v file!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } else {
        fprintf(fptr, "createShaders(): fopen() succeed to open Fragment Shader spir-v file!.\n");
    }

    fseek(fp, 0l, SEEK_END);
    fileSize = ftell(fp);
    if(fileSize == 0) {
        fprintf(fptr, "createShaders(): ftell() gave file size: 0.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    }
    fseek(fp, 0l, SEEK_SET);

    shaderData = (char*)malloc(fileSize * sizeof(char));

    retVal = fread(shaderData, fileSize, 1, fp);
    if(retVal != 1) {
        fprintf(fptr, "createShaders(): fread() failed to read Fragment Shader file!.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return (vkResult);
    } else {
        fprintf(fptr, "createShaders(): fread() succeed to read Fragment Shader file!.\n");
    }
    fclose(fp);
    fp = NULL;

    memset((void*)&vkShaderModuleCreateInfo, 0, sizeof(VkShaderModuleCreateInfo));

    vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderModuleCreateInfo.pNext = NULL;
    vkShaderModuleCreateInfo.flags = 0;
    vkShaderModuleCreateInfo.codeSize = fileSize;
    vkShaderModuleCreateInfo.pCode = (uint32_t*)shaderData;

    vkResult = vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, NULL, &vkShaderModule_fragment);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createShaders(): vkCreateShaderModule() for Fragment Shader Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createShaders(): vkCreateShaderModule() for Fragment Shader Successful!.\n");
    }

    if(shaderData) {
        free(shaderData);
        shaderData = NULL;
    }
    fprintf(fptr, "createShaders(): Fragment Shader Module Created Successful!.\n");

    return (vkResult);
}

VkResult createDescriptorSetLayout(void) {
    // Variables
    VkResult vkResult = VK_SUCCESS;

    //Create Descriptor Set Layout Create Info
    VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo;
    memset((void*)&vkDescriptorSetLayoutCreateInfo, 0, sizeof(VkDescriptorSetLayoutCreateInfo));

    vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vkDescriptorSetLayoutCreateInfo.pNext = NULL;
    vkDescriptorSetLayoutCreateInfo.flags = 0;
    vkDescriptorSetLayoutCreateInfo.bindingCount = 0; // will change when we introduce resources/descriptors
    vkDescriptorSetLayoutCreateInfo.pBindings = NULL; // will change when we introduce resources/descriptors
    
    // Create Descriptor Set Layout
    vkResult = vkCreateDescriptorSetLayout(vkDevice, &vkDescriptorSetLayoutCreateInfo, NULL, &vkDescriptorSetLayout);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createDescriptorSetLayout(): vkCreateDescriptorSetLayout() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createDescriptorSetLayout(): vkCreateDescriptorSetLayout() Successful!.\n");
    }

    return (vkResult);
}

VkResult createPipelineLayout(void) {
    // Variables
    VkResult vkResult = VK_SUCCESS;

    // Create Pipeline Layout Create Info
    VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo;
    memset((void*)&vkPipelineLayoutCreateInfo, 0, sizeof(VkPipelineLayoutCreateInfo));

    vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkPipelineLayoutCreateInfo.pNext = NULL;
    vkPipelineLayoutCreateInfo.flags = 0;
    vkPipelineLayoutCreateInfo.setLayoutCount = 1; // we have only one descriptor set layout
    vkPipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
    vkPipelineLayoutCreateInfo.pushConstantRangeCount = 0; // no push constant range for now
    vkPipelineLayoutCreateInfo.pPushConstantRanges = NULL;

    // Create Pipeline Layout
    vkResult = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutCreateInfo, NULL, &vkPipelineLayout);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createPipelineLayout(): vkCreatePipelineLayout() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createPipelineLayout(): vkCreatePipelineLayout() Successful!.\n");
    }

    return (vkResult);
}

VkResult createRenderPass(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // Code
    //Step 1: Create Attachment Description stcture array
    VkAttachmentDescription vkAttachmentDescription_array[1];
    memset((void*)vkAttachmentDescription_array, 0, sizeof(VkAttachmentDescription) * _ARRAYSIZE(vkAttachmentDescription_array));

    vkAttachmentDescription_array[0].flags = 0;
    vkAttachmentDescription_array[0].format =  vkFormat_color;
    vkAttachmentDescription_array[0].samples = VK_SAMPLE_COUNT_1_BIT;
    vkAttachmentDescription_array[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vkAttachmentDescription_array[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vkAttachmentDescription_array[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkAttachmentDescription_array[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkAttachmentDescription_array[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkAttachmentDescription_array[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Step 2: Create Attachment Reference Structure
    VkAttachmentReference vkAttachmentReference;
    memset((void*)&vkAttachmentReference, 0, sizeof(VkAttachmentReference));

    vkAttachmentReference.attachment = 0; // From the array of attachment description, refer to 0th index, oth will be color attachment
    vkAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 

    // STep 3: create sub pass description strcture
    VkSubpassDescription vkSubpassDescription;
    memset((void*)&vkSubpassDescription, 0, sizeof(VkSubpassDescription));
    
    vkSubpassDescription.flags = 0;
    vkSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkSubpassDescription.inputAttachmentCount = 0;
    vkSubpassDescription.pInputAttachments = NULL;
    vkSubpassDescription.colorAttachmentCount = 1;
    vkSubpassDescription.pColorAttachments = &vkAttachmentReference;
    vkSubpassDescription.pResolveAttachments = NULL;
    vkSubpassDescription.pDepthStencilAttachment = NULL;
    vkSubpassDescription.preserveAttachmentCount = 0;
    vkSubpassDescription.pPreserveAttachments = NULL;

    // Step 4: Render Pass Create Info
    VkRenderPassCreateInfo vkRenderPassCreateInfo;
    memset((void*)&vkRenderPassCreateInfo, 0, sizeof(VkRenderPassCreateInfo));

    vkRenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    vkRenderPassCreateInfo.flags = 0;
    vkRenderPassCreateInfo.pNext = NULL;
    vkRenderPassCreateInfo.attachmentCount = _ARRAYSIZE(vkAttachmentDescription_array);
    vkRenderPassCreateInfo.pAttachments = vkAttachmentDescription_array;
    vkRenderPassCreateInfo.subpassCount = 1;
    vkRenderPassCreateInfo.pSubpasses = &vkSubpassDescription;
    vkRenderPassCreateInfo.dependencyCount = 0;
    vkRenderPassCreateInfo.pDependencies = NULL;
    

    // Step 5: Create Render Pass
    vkResult = vkCreateRenderPass(
        vkDevice,
        &vkRenderPassCreateInfo,
        NULL,
        &vkRenderPass
    );

    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createRenderPass(): vkCreateRenderPass() Failed!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createRenderPass(): vkCreateRenderPass() Successful!.\n");
    }

    return (vkResult);
}

VkResult createFramebuffers(void) {
    // Variables
    VkResult vkResult = VK_SUCCESS;

    // Step 1: create VkImageView array same as size of attachments
    VkImageView vkImageView_attachments_array[1];
    memset((void*)vkImageView_attachments_array, 0, sizeof(VkImageView) * _ARRAYSIZE(vkImageView_attachments_array));

    // Step 2: Create VkFrameBufferCreateInfo structure
    VkFramebufferCreateInfo vkFrameBufferCreateInfo;
    memset((void*)&vkFrameBufferCreateInfo, 0, sizeof(VkFramebufferCreateInfo));

    vkFrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vkFrameBufferCreateInfo.flags = 0;
    vkFrameBufferCreateInfo.pNext = NULL;
    vkFrameBufferCreateInfo.renderPass = vkRenderPass;
    vkFrameBufferCreateInfo.attachmentCount = _ARRAYSIZE(vkImageView_attachments_array);
    vkFrameBufferCreateInfo.pAttachments = vkImageView_attachments_array;
    vkFrameBufferCreateInfo.width = vkExtent2D_swapchain.width;
    vkFrameBufferCreateInfo.height = vkExtent2D_swapchain.height;
    vkFrameBufferCreateInfo.layers = 1; // VALIDATION USE CASE 2: Comment this line to see the error


    // allocate frame buffers array and create frame buffers in loop with counts of allocated swapchain images
    vkFramebuffer_array = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * swapchainImageCount);
    for(uint32_t i = 0; i < swapchainImageCount; i++) {

        vkImageView_attachments_array[0] = swapchainImageView_array[i];

        vkResult = vkCreateFramebuffer(vkDevice, &vkFrameBufferCreateInfo, NULL, &vkFramebuffer_array[i]);
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "createFramebuffers(): vkCreateFramebuffer() Failed at {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "createFramebuffers(): vkCreateFramebuffer() Successful for {%d}!.\n", i);
        }
    }

    return (vkResult);
}

VkResult createSemaphores(void) {
    // Variables
    VkResult vkResult = VK_SUCCESS;

    // Create Semaphore info
    VkSemaphoreCreateInfo vkSemaphoreCreateInfo;
    memset((void*)&vkSemaphoreCreateInfo, 0, sizeof(VkSemaphoreCreateInfo));

    vkSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkSemaphoreCreateInfo.pNext = NULL;
    vkSemaphoreCreateInfo.flags = 0; // it's reserved must be zero

    // By defualt if no type is specified, binary semaphore is created!

    // create semaphore for backbuffer
    vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, NULL, &vkSemaphore_backbuffer);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSemaphores(): vkCreateSemaphore() Failed for Back Buffer Semaphore!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSemaphores(): vkCreateSemaphore() Successful for Back Buffer Semaphore!.\n");
    }

    // create semaphore for render complete
    vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, NULL, &vkSemaphore_rendercomplete);
    if(vkResult != VK_SUCCESS) {
        fprintf(fptr, "createSemaphores(): vkCreateSemaphore() Failed for Render Complete Semaphore!.\n");
        return (vkResult);
    } else {
        fprintf(fptr, "createSemaphores(): vkCreateSemaphore() Successful for Render Complete Semaphore!.\n");
    }

    return (vkResult);
}

VkResult createFences(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    // VkFenceCreateInfo
    VkFenceCreateInfo vkFenceCreateInfo;
    memset((void*)&vkFenceCreateInfo, 0, sizeof(VkFenceCreateInfo));

    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkFenceCreateInfo.pNext = NULL;
    vkFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkFence_array = (VkFence*) malloc(sizeof(VkFence) * swapchainImageCount);


    for(uint32_t i = 0; i < swapchainImageCount; i++) {
        vkResult = vkCreateFence(vkDevice, &vkFenceCreateInfo, NULL, &vkFence_array[i]);
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "createFences(): vkCreateFence() Failed at {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "createFences(): vkCreateFence() Successful for {%d}!.\n", i);
        }
    }

    return (vkResult);
}

VkResult buildCommandBuffers(void) {
    // variables
    VkResult vkResult = VK_SUCCESS;

    for(uint32_t i = 0; i < swapchainImageCount; i++) {
        // Reset Command Buffers
        vkResult = vkResetCommandBuffer(vkCommandBuffer_array[i], 0); 
        // adding 0 here means sdon't release resources allocated by command pool
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "buildCommandBuffers(): vkResetCommandBuffer() Failed for {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "buildCommandBuffers(): vkResetCommandBuffer() Successful for {%d}!.\n", i);
        }

        // set VkCommandBufferBeginInfo
        VkCommandBufferBeginInfo vkCommandBufferBeginInfo;
        memset((void*)&vkCommandBufferBeginInfo, 0, sizeof(VkCommandBufferBeginInfo));

        vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkCommandBufferBeginInfo.pNext = NULL;
        vkCommandBufferBeginInfo.flags = 0; 
        // Zero indicates that we'll use primary command buffer and also specifying that we are not
        // using this buffer simultaniously between multiple threads

        vkResult = vkBeginCommandBuffer(vkCommandBuffer_array[i], &vkCommandBufferBeginInfo);
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "buildCommandBuffers(): vkBeginCommandBuffer() Failed for {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "buildCommandBuffers(): vkBeginCommandBuffer() Successful for {%d}!.\n", i);
        }

        // Set Clear Values
        VkClearValue vkClearValue_array[1];
        memset((void*)vkClearValue_array, 0, sizeof(VkClearValue) * _ARRAYSIZE(vkClearValue_array));

        vkClearValue_array[0].color = vkClearColorValue;

        // Render pass begin info
        VkRenderPassBeginInfo vkRenderPassBeginInfo;
        memset((void*)&vkRenderPassBeginInfo, 0, sizeof(VkRenderPassBeginInfo));

        vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vkRenderPassBeginInfo.pNext = NULL;
        vkRenderPassBeginInfo.renderPass = vkRenderPass;
        vkRenderPassBeginInfo.renderArea.offset.x = 0;
        vkRenderPassBeginInfo.renderArea.offset.y = 0;
        vkRenderPassBeginInfo.renderArea.extent.width = vkExtent2D_swapchain.width;
        vkRenderPassBeginInfo.renderArea.extent.height = vkExtent2D_swapchain.height;
        vkRenderPassBeginInfo.clearValueCount = _ARRAYSIZE(vkClearValue_array);
        vkRenderPassBeginInfo.pClearValues = vkClearValue_array;
        vkRenderPassBeginInfo.framebuffer = vkFramebuffer_array[i];

        // begin render pass
        vkCmdBeginRenderPass(vkCommandBuffer_array[i], &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        // content of this pass are subpass and part of primary command buffers so inline

        // Here we should call vulkan drawing functions!

        // End Render Pass
        vkCmdEndRenderPass(vkCommandBuffer_array[i]);

        // End command buffer recording
        vkResult = vkEndCommandBuffer(vkCommandBuffer_array[i]);
        if(vkResult != VK_SUCCESS) {
            fprintf(fptr, "buildCommandBuffers(): vkEndCommandBuffer() Failed for {%d}!.\n", i);
            return (vkResult);
        } else {
            fprintf(fptr, "buildCommandBuffers(): vkEndCommandBuffer() Successful for {%d}!.\n", i);
        }
    }

    return (vkResult);
}

// Always Keep this function at the end of this file
VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
    VkDebugReportFlagsEXT vkDebugReportFlagsEXIT, 
    VkDebugReportObjectTypeEXT vkDebugReportObjectTypeEXIT, 
    uint64_t object, 
    size_t location, 
    int32_t messageCode, 
    const char* pLayerPrefix, 
    const char* pMessage, 
    void* pUserData
) {
    fprintf(fptr, "AMK_VALIDATION: debugReportCallback() :  %s (%d) = %s\n", pLayerPrefix, messageCode, pMessage);
    return VK_FALSE; // return false to ignore this message
}
