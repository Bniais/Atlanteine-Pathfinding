#include<windows.h>
#include<stdio.h>

POINT grilleRef = {585, 579};

int main(int argc, char** argv)
{
    FARPROC getPixelColor;

    HINSTANCE _hGDI = LoadLibrary("gdi32.dll");
    if(_hGDI)
    {
        POINT windowSize = (POINT){GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        printf("size : %d %d", windowSize.x, windowSize.y);

        getPixelColor = GetProcAddress(_hGDI, "GetPixel");
        HDC _hdc = GetDC(NULL);

        if(_hdc)
        {
            for(int x=0; x<windowSize.x; x++){
                for(int y=0; y<windowSize.y; y++){
                    COLORREF _color = (*getPixelColor)(_hdc, x, y);
                    printf("pos : %d %d", x, y);
                    printf("color: 0x%02x\n", _color);
                }
            }
        }
        FreeLibrary(_hGDI);
    }
    return 0;
}
