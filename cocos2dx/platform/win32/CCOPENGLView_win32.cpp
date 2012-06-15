/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "GL/glew.h"
#include "CCEGLView.h"

#include "CCSet.h"
#include "ccMacros.h"
#include "CCDirector.h"
#include "CCTouch.h"
#include "CCTouchDispatcher.h"
#include "CCIMEDispatcher.h"
#include "CCKeypadDispatcher.h"
#include "CCApplication.h"

NS_CC_BEGIN;

//////////////////////////////////////////////////////////////////////////
// impliment CCOPENGL
//////////////////////////////////////////////////////////////////////////

class CCOPENGL
{
public:
	~CCOPENGL() 
	{
		if (m_HDC)
		{
			ReleaseDC(m_HWND, m_HDC);
		}

		if (m_HWND && !DestroyWindow(m_HWND))					// Are We Able To Destroy The Window?
		{
			MessageBox(NULL,L"Could Not Release hWnd.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
			m_HWND=NULL;										// Set hWnd To NULL
		}
	}

	static CCOPENGL * create(CCEGLView * pWindow)
	{
		CCOPENGL * pOpenGL = new CCOPENGL;
		BOOL bSuccess = FALSE;
		do 
		{
			CC_BREAK_IF(! pOpenGL);

			pOpenGL->m_HWND = pWindow->getHWnd();

			pOpenGL->m_HDC = GetDC(pOpenGL->m_HWND);

			static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
			{
				sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
				1,											// Version Number
				PFD_DRAW_TO_WINDOW |						// Format Must Support Window
				PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
				PFD_DOUBLEBUFFER,							// Must Support Double Buffering
				PFD_TYPE_RGBA,								// Request An RGBA Format
				24,											// Select Our Color Depth
				0, 0, 0, 0, 0, 0,							// Color Bits Ignored
				0,											// No Alpha Buffer
				0,											// Shift Bit Ignored
				0,											// No Accumulation Buffer
				0, 0, 0, 0,									// Accumulation Bits Ignored
				16,											// 16Bit Z-Buffer (Depth Buffer)  
				0,											// No Stencil Buffer
				0,											// No Auxiliary Buffer
				PFD_MAIN_PLANE,								// Main Drawing Layer
				0,											// Reserved
				0, 0, 0										// Layer Masks Ignored
			};
			pfd.cColorBits = (BYTE) GetDeviceCaps( pOpenGL->m_HDC, BITSPIXEL );

			GLuint		PixelFormat;			// Holds The Results After Searching For A Match
			if (!(PixelFormat=ChoosePixelFormat(pOpenGL->m_HDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
			{							// Reset The Display
				MessageBox(NULL,L"Can't Find A Suitable PixelFormat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;								// Return FALSE
			}

			if(!SetPixelFormat(pOpenGL->m_HDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
			{								// Reset The Display
				MessageBox(NULL,L"Can't Set The PixelFormat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;								// Return FALSE
			}

			HGLRC hRC;
			if (!(hRC=wglCreateContext(pOpenGL->m_HDC)))				// Are We Able To Get A Rendering Context?
			{
				MessageBox(NULL,L"Can't Create A GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;								// Return FALSE
			}

			if(!wglMakeCurrent(pOpenGL->m_HDC,hRC))					// Try To Activate The Rendering Context
			{
				MessageBox(NULL,L"Can't Activate The GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;								// Return FALSE
			}

			//wglMakeCurrent(pOpenGL->m_HDC,wglCreateContext(pOpenGL->m_HDC));

			GLenum err = glewInit();
			if ( GLEW_OK != err )
			{
				WCHAR errInfo[100];
				const char* errs = (const char*)glewGetErrorString(err);
				mbstowcs(errInfo, errs, 100 );
				//MultiByteToWideChar(CP_ACP, 0, glewGetErrorString(err), -1, errInfo, 100);
				MessageBox(NULL,L"glewInit Failed!", errInfo, MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				bSuccess = TRUE;
			}			
		} while (0);

		if (! bSuccess)
		{
			CC_SAFE_DELETE(pOpenGL);  
		}

		return pOpenGL;
	}

	void resizeSurface()
	{
		// to do...
	}

	void swapBuffers()
	{
		SwapBuffers(m_HDC);
	}
private:
	CCOPENGL() 
		: m_HWND(NULL)
		, m_HDC(0)
	{}

	HWND		m_HWND;
	HDC			m_HDC;
};

//////////////////////////////////////////////////////////////////////////
// impliment CCEGLView
//////////////////////////////////////////////////////////////////////////
static CCEGLView * s_pMainWindow;
static const WCHAR * kWindowClassName = L"Cocos2dxWin32";

static LRESULT CALLBACK _WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (s_pMainWindow && s_pMainWindow->getHWnd() == hWnd)
	{
		return s_pMainWindow->WindowProc(uMsg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

CCEGLView::CCEGLView()
: m_bCaptured(false)
, m_bOrientationReverted(false)
, m_bOrientationInitVertical(false)
, m_pDelegate(NULL)
, m_pOpenGL(NULL)
, m_hWnd(NULL)
, m_eInitOrientation(CCDeviceOrientationPortrait)
, m_fScreenScaleFactor(1.0f)
{
    m_pTouch    = new CCTouch;
    m_pSet      = new CCSet;
    m_tSizeInPoints.cx = m_tSizeInPoints.cy = 0;
    SetRectEmpty(&m_rcViewPort);
}

CCEGLView::~CCEGLView()
{
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
bool CCEGLView::Create(LPCTSTR pTitle, int width, int height, int bits, bool fullscreen)
{
	//GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	HINSTANCE hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) _WindowProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= kWindowClassName;						// Set The Class Name
	
	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,L"Failed To Register The Window Class.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,L"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?",L"NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,L"Program Will Now Close.",L"ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	GetWindowRect(GetDesktopWindow(), &WindowRect);

	//DWORD flags;
	//flags = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE  ;

	// create window
	m_hWnd = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,	// Extended Style For The Window
		kWindowClassName,									// Class Name
		pTitle,												// Window Title
		/*WS_CAPTION |*/ WS_POPUPWINDOW/* | WS_MINIMIZEBOX*/,		// Defined Window Style
		0, 0,								                // Window Position
		0,                                                  // Window Width
		0,                                                  // Window Height
		NULL,												// No Parent Window
		NULL,												// No Menu
		hInstance,											// Instance
		NULL );

	//CC_BREAK_IF(! m_hWnd);

    m_eInitOrientation = CCDirector::sharedDirector()->getDeviceOrientation();
    m_bOrientationInitVertical = (CCDeviceOrientationPortrait == m_eInitOrientation
        || kCCDeviceOrientationPortraitUpsideDown == m_eInitOrientation) ? true : false;
    m_tSizeInPoints.cx = width;
    m_tSizeInPoints.cy = height;
    resize(width, height);

	// init egl
	m_pOpenGL = CCOPENGL::create(this);

	if (! m_pOpenGL)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}

	s_pMainWindow = this;

	return true;
}

LRESULT CCEGLView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	switch (message)
	{
	case WM_LBUTTONDOWN:
		if (m_pDelegate && m_pTouch && MK_LBUTTON == wParam)
		{
            POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
            if (PtInRect(&m_rcViewPort, pt))
            {
                m_bCaptured = true;
                SetCapture(m_hWnd);
                m_pTouch->SetTouchInfo(0, (float)(pt.x - m_rcViewPort.left) / m_fScreenScaleFactor,
                    (float)(pt.y - m_rcViewPort.top) / m_fScreenScaleFactor);
                m_pSet->addObject(m_pTouch);
                m_pDelegate->touchesBegan(m_pSet, NULL);
            }
		}
		break;

	case WM_MOUSEMOVE:
		if (MK_LBUTTON == wParam && m_bCaptured)
		{
            m_pTouch->SetTouchInfo(0, (float)((short)LOWORD(lParam)- m_rcViewPort.left) / m_fScreenScaleFactor,
                (float)((short)HIWORD(lParam) - m_rcViewPort.top) / m_fScreenScaleFactor);
            m_pDelegate->touchesMoved(m_pSet, NULL);
		}
		break;

	case WM_LBUTTONUP:
		if (m_bCaptured)
		{
			m_pTouch->SetTouchInfo(0, (float)((short)LOWORD(lParam)- m_rcViewPort.left) / m_fScreenScaleFactor,
                (float)((short)HIWORD(lParam) - m_rcViewPort.top) / m_fScreenScaleFactor);
			m_pDelegate->touchesEnded(m_pSet, NULL);
			m_pSet->removeObject(m_pTouch);
            ReleaseCapture();
			m_bCaptured = false;
		}
		break;
	case WM_SIZE:
		switch (wParam)
		{
		case SIZE_RESTORED:
			CCApplication::sharedApplication().applicationWillEnterForeground();
			break;
		case SIZE_MINIMIZED:
			CCApplication::sharedApplication().applicationDidEnterBackground();
			break;
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_F1 || wParam == VK_F2)
		{
			if (GetKeyState(VK_LSHIFT) < 0 ||  GetKeyState(VK_RSHIFT) < 0 || GetKeyState(VK_SHIFT) < 0)
				CCKeypadDispatcher::sharedDispatcher()->dispatchKeypadMSG(wParam == VK_F1 ? kTypeBackClicked : kTypeMenuClicked);
		}
		break;
    case WM_CHAR:
        {
            if (wParam < 0x20)
            {
                if (VK_BACK == wParam)
                {
                    CCIMEDispatcher::sharedDispatcher()->dispatchDeleteBackward();
                }
                else if (VK_RETURN == wParam)
                {
                    CCIMEDispatcher::sharedDispatcher()->dispatchInsertText("\n", 1);
                }
                else if (VK_TAB == wParam)
                {
                    // tab input
                }
                else if (VK_ESCAPE == wParam)
                {
                    // ESC input
					CCDirector::sharedDirector()->end();
                }
            }
            else if (wParam < 128)
            {
                // ascii char
                CCIMEDispatcher::sharedDispatcher()->dispatchInsertText((const char *)&wParam, 1);
            }
            else
            {
                char szUtf8[8] = {0};
                int nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&wParam, 1, szUtf8, sizeof(szUtf8), NULL, NULL);

                CCIMEDispatcher::sharedDispatcher()->dispatchInsertText(szUtf8, nLen);
            }
        }
        break;

	case WM_PAINT:
		BeginPaint(m_hWnd, &ps);
		EndPaint(m_hWnd, &ps);
		break;

	case WM_CLOSE:
		CCDirector::sharedDirector()->end();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(m_hWnd, message, wParam, lParam);
	}
	return 0;
}

CCSize CCEGLView::getSize()
{
    if (m_bOrientationReverted)
    {
        return CCSize((float)(m_tSizeInPoints.cy), (float)(m_tSizeInPoints.cx));
    }
    return CCSize((float)(m_tSizeInPoints.cx), (float)(m_tSizeInPoints.cy));
}

bool CCEGLView::isOpenGLReady()
{
    return (NULL != m_pOpenGL);
}

void CCEGLView::release()
{
	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
	s_pMainWindow = NULL;
	UnregisterClass(kWindowClassName, GetModuleHandle(NULL));

    CC_SAFE_DELETE(m_pSet);
    CC_SAFE_DELETE(m_pTouch);
    CC_SAFE_DELETE(m_pDelegate);
    CC_SAFE_DELETE(m_pOpenGL);
    delete this;
}

void CCEGLView::setTouchDelegate(EGLTouchDelegate * pDelegate)
{
    m_pDelegate = pDelegate;
}

void CCEGLView::swapBuffers()
{
    if (m_pOpenGL)
    {
        m_pOpenGL->swapBuffers();
    }
}

int CCEGLView::setDeviceOrientation(int eOritation)
{
	do 
	{
		bool bVertical = (CCDeviceOrientationPortrait == eOritation
			|| kCCDeviceOrientationPortraitUpsideDown == eOritation) ? true : false;

		CC_BREAK_IF(m_bOrientationReverted && bVertical != m_bOrientationInitVertical);
		CC_BREAK_IF(! m_bOrientationReverted && bVertical == m_bOrientationInitVertical);

        m_bOrientationReverted = (bVertical == m_bOrientationInitVertical) ? false : true;

        // swap width and height
		RECT rc;
		GetClientRect(m_hWnd, &rc);
        resize(rc.bottom - rc.top, rc.right - rc.left);

	} while (0);

	return m_eInitOrientation;
}

void CCEGLView::setViewPortInPoints(float x, float y, float w, float h)
{
    if (m_pOpenGL)
    {
        float factor = m_fScreenScaleFactor / CC_CONTENT_SCALE_FACTOR();
        glViewport((GLint)(x * factor) + m_rcViewPort.left,
            (GLint)(y * factor) + m_rcViewPort.top,
            (GLint)(w * factor),
            (GLint)(h * factor));
    }
}

void CCEGLView::setScissorInPoints(float x, float y, float w, float h)
{
    if (m_pOpenGL)
    {
        float factor = m_fScreenScaleFactor / CC_CONTENT_SCALE_FACTOR();
        glScissor((GLint)(x * factor) + m_rcViewPort.left,
            (GLint)(y * factor) + m_rcViewPort.top,
            (GLint)(w * factor),
            (GLint)(h * factor));
    }
}

void CCEGLView::setIMEKeyboardState(bool /*bOpen*/)
{
}

HWND CCEGLView::getHWnd()
{
    return m_hWnd;
}

void CCEGLView::resize(int width, int height)
{
    if (! m_hWnd)
    {
        return;
    }

    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    if (rcClient.right - rcClient.left == width &&
        rcClient.bottom - rcClient.top == height)
    {
        return;
    }
    // calculate new window width and height
    rcClient.right = rcClient.left + width;
    rcClient.bottom = rcClient.top + height;
    AdjustWindowRectEx(&rcClient, GetWindowLong(m_hWnd, GWL_STYLE), false, GetWindowLong(m_hWnd, GWL_EXSTYLE));

    // change width and height
    SetWindowPos(m_hWnd, 0, 0, 0, rcClient.right - rcClient.left, 
        rcClient.bottom - rcClient.top, SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    if (m_pOpenGL)
    {
        m_pOpenGL->resizeSurface();
    }

    // calculate view port in pixels
    int viewPortW = (int)(m_tSizeInPoints.cx * m_fScreenScaleFactor);
    int viewPortH = (int)(m_tSizeInPoints.cy * m_fScreenScaleFactor);
    if (m_bOrientationReverted)
    {
        int tmp = viewPortW;
        viewPortW = viewPortH;
        viewPortH = tmp;
    }
    GetClientRect(m_hWnd, &rcClient);

    // calculate client new width and height
    int newW = rcClient.right - rcClient.left;
    int newH = rcClient.bottom - rcClient.top;

    // calculate new view port
    m_rcViewPort.left   = rcClient.left + (newW - viewPortW) / 2;
    m_rcViewPort.top    = rcClient.top + (newH - viewPortH) / 2;
    m_rcViewPort.right  = m_rcViewPort.left + viewPortW;
    m_rcViewPort.bottom = m_rcViewPort.top + viewPortH;
}

void CCEGLView::centerWindow()
{
    if (! m_hWnd)
    {
        return;
    }

    RECT rcDesktop, rcWindow;
    GetWindowRect(GetDesktopWindow(), &rcDesktop);

    // substract the task bar
    HWND hTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hTaskBar != NULL)
    {
        APPBARDATA abd;

        abd.cbSize = sizeof(APPBARDATA);
        abd.hWnd = hTaskBar;

        SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
        SubtractRect(&rcDesktop, &rcDesktop, &abd.rc);
    }
    GetWindowRect(m_hWnd, &rcWindow);

    int offsetX = (rcDesktop.right - rcDesktop.left - (rcWindow.right - rcWindow.left)) / 2;
    offsetX = (offsetX > 0) ? offsetX : rcDesktop.left;
    int offsetY = (rcDesktop.bottom - rcDesktop.top - (rcWindow.bottom - rcWindow.top)) / 2;
    offsetY = (offsetY > 0) ? offsetY : rcDesktop.top;

    SetWindowPos(m_hWnd, 0, offsetX, offsetY, 0, 0, SWP_NOCOPYBITS | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

void CCEGLView::setScreenScale(float factor)
{
    m_fScreenScaleFactor = factor;
}

bool CCEGLView::canSetContentScaleFactor()
{
    return true;
}

void CCEGLView::setContentScaleFactor(float contentScaleFactor)
{
    m_fScreenScaleFactor = contentScaleFactor;
    if (m_bOrientationReverted)
    {
        resize((int)(m_tSizeInPoints.cy * contentScaleFactor), (int)(m_tSizeInPoints.cx * contentScaleFactor));
    }
    else
    {
        resize((int)(m_tSizeInPoints.cx * contentScaleFactor), (int)(m_tSizeInPoints.cy * contentScaleFactor));
    }
    centerWindow();
}

CCEGLView& CCEGLView::sharedOpenGLView()
{
    CC_ASSERT(s_pMainWindow);
    return *s_pMainWindow;
}

NS_CC_END;
