#include <windows.h>
#include <wingdi.h>
#include <stdio.h>

struct tagScreen
{
    DWORD*     pixels;  /* Pointer to raw bitmap bits. Access with: pixels[(y * Screen.cx) + x] */
    size_t     cx, cy;  /* Width and height of bitmap. */
    HBITMAP    hBmp;
    HDC        hdcMem;
    HDC        hdcScreen;
} Screen;

/* Call once to setup.
 */
void xSetupScreenBitmap(void)
{
    BITMAPINFO bmp   = { 0 };

    Screen.cx = GetSystemMetrics(SM_CXSCREEN);
    Screen.cy = GetSystemMetrics(SM_CYSCREEN);

    bmp.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmp.bmiHeader.biWidth       = Screen.cx;
    bmp.bmiHeader.biHeight      = -(int) Screen.cy;
    bmp.bmiHeader.biPlanes      = 1;
    bmp.bmiHeader.biBitCount    = 32;
    bmp.bmiHeader.biCompression = BI_RGB;

    Screen.hBmp = CreateDIBSection(NULL, &bmp, DIB_RGB_COLORS, (void **)&Screen.pixels, NULL, 0);
    Screen.hdcMem = CreateCompatibleDC(NULL);
    SelectObject(Screen.hdcMem, Screen.hBmp);

    Screen.hdcScreen = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
}

/* Call to copy the current screen into the screen bitmap.
 */
void xCopyScreen(void)
{
    BitBlt(Screen.hdcMem, 0, 0, Screen.cx, Screen.cy, Screen.hdcScreen, 0, 0, SRCCOPY);
}

/* Convert a RGB value to the BGR used by Windows COLORREF values.
 */
COLORREF RGB2BGR(DWORD color)
{
    return RGB(GetBValue(color), GetGValue(color), GetRValue(color));
}


int main(void)
{
    size_t x, y;

    xSetupScreenBitmap(); /* Call once for setup. */
    xCopyScreen();        /* Call whenever you want a fresh copy of the screen. */

    /* Shows how to loop through all pixels. */
    for (x = 0; x < Screen.cx; x++)
    {
        for (y = 0; y < Screen.cy; y++)
        {
            /* Use Screen.pixels[(y * Screen.cx) + x] as required. */

            /* TEST CODE: Output slighlty altered pixels to the screen. */
            SetPixel(Screen.hdcScreen, x, y, RGB2BGR(Screen.pixels[(y * Screen.cx) + x]) + 100);
        }
    }

    return 0;
}
