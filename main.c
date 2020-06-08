#include<windows.h>
#include<SDL2/SDL.h>
#include<stdio.h>
#define NB_CASES 13

POINT grilleRef = {584, 579};
POINT grilleSize = {0, 0};
int caseDimRef = 39;
float coefSize = 1;
POINT initShift = {19+39,26+39};

POINT grilleStart = {0,0};

COLORREF grille_color[NB_CASES][NB_CASES];

COLORREF BACKGROUND_COLOR = 0x8f8a4e;

int main(int argc, char** argv)
{
	if(argc == 5){
        sscanf(argv[1], "%ld", &(grilleStart.x));
		sscanf(argv[2], "%ld", &(grilleStart.y));
		sscanf(argv[3], "%ld", &(grilleSize.x));
		sscanf(argv[4], "%ld", &(grilleSize.y));
	}
    FARPROC getPixelColor;

    HINSTANCE _hGDI = LoadLibrary("gdi32.dll");
    if(_hGDI)
    {
        POINT windowSize = (POINT){GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        printf("size : %ld %ld", windowSize.x, windowSize.y);

        getPixelColor = GetProcAddress(_hGDI, "GetPixel");
        HDC _hdc = GetDC(NULL);

        if(_hdc)
        {
            POINT _cursor;
            GetCursorPos(&_cursor);
			if(argc!=5){
				for(int x=0; x<windowSize.x && grilleStart.x == 0; x+=25){
	                for(int y=0; y<windowSize.y && grilleStart.x == 0; y+=25){
	                    COLORREF _color = (*getPixelColor)(_hdc, x, y);
						if(_color == BACKGROUND_COLOR){
							grilleStart.x = x;
							while(_color == BACKGROUND_COLOR){
								grilleStart.x--;
								_color = (*getPixelColor)(_hdc, grilleStart.x, y);
							}
							grilleStart.x++;

							do{
								grilleSize.x+=25;
								_color = (*getPixelColor)(_hdc, grilleStart.x+grilleSize.x, y);
							}while(_color == BACKGROUND_COLOR);

							do{
								grilleSize.x--;
								_color = (*getPixelColor)(_hdc, grilleStart.x+grilleSize.x, y);
							}while(_color != BACKGROUND_COLOR);

							grilleStart.y = y;
							do{
								grilleStart.y--;
								_color = (*getPixelColor)(_hdc, grilleStart.x, grilleStart.y);
							}while(_color == BACKGROUND_COLOR);
							grilleStart.y++;
						}
	                }
	            }
			}

			coefSize = grilleSize.x / (float)grilleRef.x;
			grilleSize.y = grilleRef.y * coefSize;
			printf("coef size%f\n", coefSize);
            printf("pos : %ld %ld\n", grilleStart.x, grilleStart.y);
			printf("size : %ld %ld\n", grilleSize.x, grilleSize.y);

			for(int i=0;i<NB_CASES;i++){
				for(int j=0; j<NB_CASES; j++){
					grille_color[i][j] = (*getPixelColor)(_hdc, (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i), (int)(grilleStart.y + initShift.y*coefSize + (caseDimRef*coefSize) * j));
					printf("0x%x", grille_color[i][j]);
				}
				printf("\n");
			}
        }
        FreeLibrary(_hGDI);
    }
    return 0;
}
