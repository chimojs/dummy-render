// RawOpenGLWindow.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RawOpenGLWindow.h"
#include <windows.h>
#include <iostream>

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#if defined(GLEW_EGL)
#include <GL/eglew.h>
#elif defined(GLEW_OSMESA)
#define GLAPI extern
#include <GL/osmesa.h>
#elif defined(_WIN32)
#include <GL/wglew.h>
#elif !defined(__APPLE__) && !defined(__HAIKU__) || defined(GLEW_APPLE_GLX)
#include <GL/glxew.h>
#endif
}

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma comment(lib, "glew32sd.lib")
#pragma  comment (lib, "opengl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RAWOPENGLWINDOW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RAWOPENGLWINDOW));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RAWOPENGLWINDOW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RAWOPENGLWINDOW);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, SW_MAXIMIZE);
   UpdateWindow(hWnd);
   PostMessage(hWnd, WM_TIMER, 0, 0);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
static const char *vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 textCord;\n"
"out vec2 tex;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"   tex = textCord;\n"
"}\0";

static const char *fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 tex;\n"
"uniform sampler2D texture1;\n"
"void main()\n"
"{\n"
"   vec4 color = vec4(tex, 0.2, 1.f);\n"
"   FragColor = mix(color, texture(texture1, tex), 0.4f);\n"
"   //FragColor = color;\n"
"}\n\0";

static const float vertices[] = {
    -0.5f, -0.5f, 0.0f,  0.f, 0.f,
    0.5f, -0.5f, 0.0f, 1.f, 0.f,
    0.5f,  0.5f, 0.0f,  1.f, 1.f,
    0.5f,  0.5f, 0.0f,   1.f, 1.f,
    -0.5f,  0.5f, 0.0f,  0.f, 1.f,
    -0.5f, -0.5f, 0.0f, 0.f, 0.f,
};

const unsigned int indices[] = {
    0, 1, 2, // first triangle
    3, 4, 5  // second triangle
};

// opengl
unsigned int texture;
unsigned int PBO;
unsigned int shaderProgram;
unsigned int VAO;
unsigned int VBO;
unsigned int EBO;

static void initOpenglResource()
{
    // vertex shader compile
    //--------------------
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    int success;
    char infolog[512] = { 0 };
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infolog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infolog << std::endl;
    }

    // fragment shader compile
    //------------------------
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infolog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED" << infolog << std::endl;
    }

    // link shader program
    //--------------------
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infolog);
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED" << infolog << std::endl;
    }

    // once we linked out program, shader is no need anymore
    //-----------------------------------------------------
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // prepare texture
    //----------------
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("1.png", &width, &height, &channels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
    else
    {
        std::cout << "Failed to load texture" <<std::endl;
    }
    stbi_image_free(data);

    glGenBuffers(1, &PBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * channels, nullptr, GL_STREAM_COPY);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // create vertex array object
    //---------------------------
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // prepare vertex buffer object
    //-----------------------------
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    //-------------------
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // color attribute
    //-------------------
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

HWND wnd = NULL;
HDC dc = NULL;
HGLRC rc = NULL;

struct createParams
{
    int         pixelformat;
    int         major, minor;  /* GL context version number */

    /* https://www.opengl.org/registry/specs/ARB/glx_create_context.txt */
    int         profile;       /* core = 1, compatibility = 2 */
    int         flags;         /* debug = 1, forward compatible = 2 */
};

void uninitOpenglResource()
{
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &PBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}

GLboolean glewCreateContext(struct createParams* params)
{
    PIXELFORMATDESCRIPTOR pfd;
#if 0
    WNDCLASS wc;

    /* register window class */
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = L"GLEW";
    if (0 == RegisterClass(&wc)) return GL_TRUE;
    /* create window */
    wnd = CreateWindow(L"GLEW", L"GLEW", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (NULL == wnd) return GL_TRUE;
    /* get the device context */
#endif
    dc = GetDC(wnd);
    if (NULL == dc) return GL_TRUE;
    /* find pixel format */
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    if (params->pixelformat == -1) /* find default */
    {
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
        params->pixelformat = ChoosePixelFormat(dc, &pfd);
        if (params->pixelformat == 0) return GL_TRUE;
    }
    /* set the pixel format for the dc */
    if (FALSE == SetPixelFormat(dc, params->pixelformat, &pfd)) return GL_TRUE;
    /* create rendering context */
    rc = wglCreateContext(dc);
    if (NULL == rc) return GL_TRUE;
    if (FALSE == wglMakeCurrent(dc, rc)) return GL_TRUE;
    if (params->major || params->profile || params->flags)
    {
        HGLRC oldRC = rc;
        int contextAttrs[20];
        int i;

        wglewInit();

        /* Intel HD 3000 has WGL_ARB_create_context, but not WGL_ARB_create_context_profile */
        if (!wglewGetExtension("WGL_ARB_create_context"))
            return GL_TRUE;

        i = 0;
        if (params->major)
        {
            contextAttrs[i++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
            contextAttrs[i++] = params->major;
            contextAttrs[i++] = WGL_CONTEXT_MINOR_VERSION_ARB;
            contextAttrs[i++] = params->minor;
        }
        if (params->profile)
        {
            contextAttrs[i++] = WGL_CONTEXT_PROFILE_MASK_ARB;
            contextAttrs[i++] = params->profile;
        }
        if (params->flags)
        {
            contextAttrs[i++] = WGL_CONTEXT_FLAGS_ARB;
            contextAttrs[i++] = params->flags;
        }
        contextAttrs[i++] = 0;
        rc = wglCreateContextAttribsARB(dc, 0, contextAttrs);

        if (NULL == rc) return GL_TRUE;
        if (!wglMakeCurrent(dc, rc)) return GL_TRUE;

        wglDeleteContext(oldRC);
    }
    return GL_FALSE;
}

void glewDestroyContext()
{
    if (NULL != rc) wglMakeCurrent(NULL, NULL);
    if (NULL != rc) wglDeleteContext(rc);
    if (NULL != wnd && NULL != dc) ReleaseDC(wnd, dc);
}

const char* path = "1.png";

void loadTexture()
{
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data)
    {
        std::cout << "Failed to load texture" <<std::endl;
        return;
    }

    //makeCurrent();

    // See also : http://www.songho.ca/opengl/gl_pbo.html
    glBufferData(PBO, width * height * channels, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    GLvoid* pTextureData = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (pTextureData)
        memcpy(pTextureData, data, width * height * channels);
    else
    {
        GLenum error = glGetError();
        error = error;
    }
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, texture);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            wnd = hWnd;

            GLuint err;
            struct createParams params =
            {
              -1,  /* pixelformat */
              0,   /* major */
              0,   /* minor */
              0,   /* profile mask */
              1    /* flags */
            };

            if (GL_TRUE == glewCreateContext(&params))
            {
                fprintf(stderr, "Error: glewCreateContext failed\n");
                glewDestroyContext();
                return 1;
            }
            glewExperimental = GL_TRUE;
            err = glewInit();

            initOpenglResource();

            //SetTimer(hWnd, 100, 40, NULL);
        }
        break;
    case WM_ERASEBKGND:
        {
            return 1;
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO pMinMaxInfo = (LPMINMAXINFO)lParam;
            pMinMaxInfo->ptMinTrackSize.y += 64;
            return 0;
        }
        break;
        case WM_PAINT:
        {
            ValidateRect(hWnd, __nullptr);
            return 0;
        }
        break;
    case WM_TIMER:
        {
            if (PBO == 0) break;
            bool static useFirst = true;
            path = useFirst ? "1.png" : "2.png";
            useFirst = !useFirst;
        
            BOOL b = wglMakeCurrent(dc, rc);
            // TODO: Add any drawing code that uses hdc here...
            glClearColor(0.f, 2.0f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            loadTexture();
            // bind textures on corresponding texture units
            //---------------------------------------------
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture);

            // bind program and set texture
            //-----------------------------
            glUseProgram(shaderProgram);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 1);

            // set vertex buffer object data, attribute and element object data.
            //-----------------------------------------------------------------
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            return 0;
        }
        break;
    case WM_SIZE:
        {
            int nHeight = HIWORD(lParam);
            int nWidth = LOWORD(lParam);
            glViewport(0, 0, nWidth, nHeight);
            return 0;
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(hWnd, 100);
            uninitOpenglResource();
            glewDestroyContext();
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
