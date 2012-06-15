
#if 0

#include "CCGlutView.h"

#include "GL/glew.h"
#include "GL/glut.h"

#include "CCSet.h"
#include "ccMacros.h"
#include "CCDirector.h"
#include "CCTouch.h"
#include "CCTouchDispatcher.h"
#include "CCIMEDispatcher.h"
#include "CCKeypadDispatcher.h"
#include "CCApplication.h"


NS_CC_BEGIN;

static CCGlutView * s_pMainWindow;

CCGlutView::CCGlutView()
	: m_bCaptured(false)
	, m_bOrientationReverted(false)
	, m_bOrientationInitVertical(false)
	, m_pDelegate(NULL)
	, m_eInitOrientation(CCDeviceOrientationPortrait)
	, m_fScreenScaleFactor(1.0f)
{
	m_pTouch    = new CCTouch;
	m_pSet      = new CCSet;
	m_tSizeInPoints.cx = m_tSizeInPoints.cy = 0;
	SetRectEmpty(&m_rcViewPort);
}

CCGlutView::~CCGlutView()
{

}

static void MouseProc(int button, int state, int x, int y)
{
	if ( s_pMainWindow )
	{
		if ( button == GLUT_LEFT_BUTTON )
		{
			if ( state == GLUT_DOWN )
				s_pMainWindow->onLButtonDown(x, y);
			else if ( state == GLUT_UP )
				s_pMainWindow->onLButtonUp(x, y);
		}
	
	}
}

static void MouseMotionProc(int x, int y)
{
	if ( s_pMainWindow )
	{
		s_pMainWindow->onMouseMove(x, y);
	}
}

static void KeyboardFunc(unsigned char key,	int x, int y)
{
	if ( s_pMainWindow )
	{
		s_pMainWindow->onKeyDown(key);
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
bool CCGlutView::Create(LPCTSTR pTitle, int width, int height, int bits, bool fullscreen)
{
	glutInitDisplayString( pTitle );
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

	int argc = 0;
	char* argv[] = 0;
	glutInit( &argc, argv );

	//glutCloseFunc(???);
	glutKeyboardFunc(KeyboardFunc);
	//glutSpecialFunc(???);
	glutMouseFunc(MouseProc);
	glutMotionFunc(MouseMotionProc);

	s_pMainWindow = this;

	return true;
}

CCSize CCGlutView::getSize()
{
	if (m_bOrientationReverted)
	{
		return CCSize((float)(m_tSizeInPoints.cy), (float)(m_tSizeInPoints.cx));
	}
	return CCSize((float)(m_tSizeInPoints.cx), (float)(m_tSizeInPoints.cy));
}

bool CCGlutView::isOpenGLReady()
{
	return true; // to do
}

void CCGlutView::release()
{
	glutExit();

	UnregisterClass(kWindowClassName, GetModuleHandle(NULL));

	CC_SAFE_DELETE(m_pSet);
	CC_SAFE_DELETE(m_pTouch);
	CC_SAFE_DELETE(m_pDelegate);
	delete this;
}

void CCGlutView::setTouchDelegate(EGLTouchDelegate * pDelegate)
{
	m_pDelegate = pDelegate;
}

void CCGlutView::swapBuffers()
{
	glutSwapBuffers();
}

int CCGlutView::setDeviceOrientation(int eOritation)
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

void CCGlutView::setViewPortInPoints(float x, float y, float w, float h)
{
	if (m_pEGL)
	{
		float factor = m_fScreenScaleFactor / CC_CONTENT_SCALE_FACTOR();
		glViewport((GLint)(x * factor) + m_rcViewPort.left,
			(GLint)(y * factor) + m_rcViewPort.top,
			(GLint)(w * factor),
			(GLint)(h * factor));
	}
}


void CCGlutView::setScissorInPoints(float x, float y, float w, float h)
{
	if (m_pEGL)
	{
		float factor = m_fScreenScaleFactor / CC_CONTENT_SCALE_FACTOR();
		glScissor((GLint)(x * factor) + m_rcViewPort.left,
			(GLint)(y * factor) + m_rcViewPort.top,
			(GLint)(w * factor),
			(GLint)(h * factor));
	}
}

void CCGlutView::setIMEKeyboardState(bool /*bOpen*/)
{
}

void CCGlutView::resize(int width, int height)
{
	glutReshapeWindow(width, height);

	// calculate new view port
	m_rcViewPort.left   = 0;
	m_rcViewPort.top    = 0;
	m_rcViewPort.right  = m_rcViewPort.left + width;
	m_rcViewPort.bottom = m_rcViewPort.top + height;
}

void CCGlutView::centerWindow()
{
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

	int offsetX = (rcDesktop.right - rcDesktop.left - (m_rcViewPort.right - m_rcViewPort.left)) / 2;
	offsetX = (offsetX > 0) ? offsetX : rcDesktop.left;
	int offsetY = (rcDesktop.bottom - rcDesktop.top - (m_rcViewPort.bottom - m_rcViewPort.top)) / 2;
	offsetY = (offsetY > 0) ? offsetY : rcDesktop.top;

	glutPositionWindow(offsetX, offsetY);
}

void CCGlutView::setScreenScale(float factor)
{
	m_fScreenScaleFactor = factor;
}

bool CCGlutView::canSetContentScaleFactor()
{
	return true;
}

void CCGlutView::setContentScaleFactor(float contentScaleFactor)
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

CCGlutView& CCGlutView::sharedOpenGLView()
{
	CC_ASSERT(s_pMainWindow);
	return *s_pMainWindow;
}

void CCGlutView::onLButtonDown( int x, int y )
{
	m_bCaptured = true;
	m_pTouch->SetTouchInfo(0, (float)(pt.x - m_rcViewPort.left) / m_fScreenScaleFactor,
		(float)(pt.y - m_rcViewPort.top) / m_fScreenScaleFactor);
	m_pSet->addObject(m_pTouch);
	m_pDelegate->touchesBegan(m_pSet, NULL);
}

void CCGlutView::onMouseMove( int x, int y )
{
	if (m_bCaptured)
	{
		m_pTouch->SetTouchInfo(0, (float)(x - m_rcViewPort.left) / m_fScreenScaleFactor,
			(float)(y - m_rcViewPort.top) / m_fScreenScaleFactor);
		m_pDelegate->touchesMoved(m_pSet, NULL);
	}
}

void CCGlutView::onLButtonUp( int x, int y )
{
	if (m_bCaptured)
	{
		m_pTouch->SetTouchInfo(0, (float)(x - m_rcViewPort.left) / m_fScreenScaleFactor,
			(float)(y - m_rcViewPort.top) / m_fScreenScaleFactor);
		m_pDelegate->touchesEnded(m_pSet, NULL);
		m_pSet->removeObject(m_pTouch);
		m_bCaptured = false;
	}
}

void CCGlutView::onKeyDown( unsigned char key )
{
	if (key == VK_F1 || key == VK_F2)
	{
		if (GetKeyState(VK_LSHIFT) < 0 ||  GetKeyState(VK_RSHIFT) < 0 || GetKeyState(VK_SHIFT) < 0)
			CCKeypadDispatcher::sharedDispatcher()->dispatchKeypadMSG(key == VK_F1 ? kTypeBackClicked : kTypeMenuClicked);
	}
	else if (key < 0x20)
	{
		if (VK_BACK == key)
		{
			CCIMEDispatcher::sharedDispatcher()->dispatchDeleteBackward();
		}
		else if (VK_RETURN == key)
		{
			CCIMEDispatcher::sharedDispatcher()->dispatchInsertText("\n", 1);
		}
		else if (VK_TAB == key)
		{
			// tab input
		}
		else if (VK_ESCAPE == key)
		{
			// ESC input
			CCDirector::sharedDirector()->end();
		}
	}
	else if (key < 128)
	{
		// ascii char
		CCIMEDispatcher::sharedDispatcher()->dispatchInsertText((const char *)&key, 1);
	}
	else
	{
		char szUtf8[8] = {0};
		int nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&key, 1, szUtf8, sizeof(szUtf8), NULL, NULL);

		CCIMEDispatcher::sharedDispatcher()->dispatchInsertText(szUtf8, nLen);
	}
}

NS_CC_END;

#endif