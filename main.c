#include<windows.h>
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<stdio.h>
#include <time.h>

#define NB_CASES 13
#define MAX_PATHS 20
const char* DIRECTION[4] = {"bas", "gauche", "haut", "droite"};

POINT grilleRef = {584, 579};
POINT grilleSize = {0, 0};
int caseDimRef = 39;
float coefSize = 1;
POINT initShift = {24+39,17+39};
#define SHIFT_HOLE 2
#define SHIFT_TP 7

POINT grilleStart = {0,0};

DWORD grille_color[NB_CASES][NB_CASES];
int grille[NB_CASES][NB_CASES];

COLORREF BACKGROUND_COLOR = 0x8f8a4e;
COLORREF _color;
unsigned int lastTime = 0, currentTime;

#define NB_TERRAIN 8
enum{PLAIN, ROCK, WATER, PUMPKIN, HOLE, CAISSE, TELEPORT, UNKNOWN};

int isPlain(DWORD color){
	return( (GetBValue(color) > 140 && GetBValue(color) < 220) && (GetGValue(color) > 150 && GetGValue(color) < 220) && (GetRValue(color) > 140 && GetRValue(color) < 200) );
}

int isRock(DWORD color){
	return( (GetBValue(color) > 45 && GetBValue(color) < 120) && (GetGValue(color) > 45 && GetGValue(color) < 120) && (GetRValue(color) > 45 && GetRValue(color) < 120) );
} //entre 70/55pour b et 120

int isWater(DWORD color){
	return( (GetBValue(color) > 70 && GetBValue(color) < 91) && (GetGValue(color) > 135 && GetGValue(color) < 141) && (GetRValue(color) > 140 && GetRValue(color) < 146) );
}

int isPumpkin(DWORD color){
	return( (GetBValue(color) > 220) &&  (GetRValue(color) < 150));
}

int isCaisse(DWORD color){
	return( (GetBValue(color) > 175 && GetBValue(color) < 215) && (GetGValue(color) > 135 && GetGValue(color) < 185) && (GetRValue(color) > 75 && GetRValue(color) < 115) );
}

int isTeleport(DWORD color){
	return(GetBValue(color) + GetGValue(color) + GetRValue(color) > 3*230);
}

int isHole(DWORD color){
	return( (GetBValue(color) > 70 && GetBValue(color) <= 91) && (GetGValue(color) > 85 && GetGValue(color) <= 135) && (GetRValue(color) > 85 && GetRValue(color) < 140) );
}

int isUnknown(DWORD color){
	return 1;
}

int (*isTerrain[NB_TERRAIN])(DWORD) =  {isPlain, isRock, isWater, isPumpkin, isHole, isCaisse, isTeleport, isUnknown};

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

enum {LOSE, MOVED, WIN};
int move(int grilleCpy[NB_CASES][NB_CASES], int dir, int *xP, int *yP){
	if(dir==0){//down
		int move;
		for(move=1; *yP+move < NB_CASES && grilleCpy[*xP][*yP+move] == PLAIN; move++);
		if (*yP+move >= NB_CASES || grilleCpy[*xP][*yP+move] == WATER)
			return LOSE;
		else if (grilleCpy[*xP][*yP+move] == HOLE)
			return WIN;
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP][*yP+move-1] = PUMPKIN;
			*yP += move-1;
			return MOVED;
		}
	}
	else if(dir==1){//left
		int move;
		for(move=1; *xP-move >=0 && grilleCpy[*xP-move][*yP] == PLAIN; move++);
		if (*xP-move < 0 || grilleCpy[*xP-move][*yP] == WATER)
			return LOSE;
		else if (grilleCpy[*xP-move][*yP] == HOLE)
			return WIN;
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP-move+1][*yP] = PUMPKIN;
			*xP -= move-1;
			return MOVED;
		}
	}
	else if(dir==2){//up
		int move;
		for(move=1; *yP-move >=0 && grilleCpy[*xP][*yP-move] == PLAIN; move++);
		if (*yP-move < 0 || grilleCpy[*xP][*yP-move] == WATER)
			return LOSE;
		else if (grilleCpy[*xP][*yP-move] == HOLE)
			return WIN;
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP][*yP-move+1] = PUMPKIN;
			*yP -= move-1;
			return MOVED;
		}
	}
	else {//right
		int move;
		for(move=1; *xP+move < NB_CASES && grilleCpy[*xP+move][*yP] == PLAIN; move++);
		if (*xP+move >= NB_CASES || grilleCpy[*xP+move][*yP] == WATER)
			return LOSE;
		else if (grilleCpy[*xP+move][*yP] == HOLE)
			return WIN;
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP+move-1][*yP] = PUMPKIN;
			*xP += move-1;
			return MOVED;
		}
	}
}

int findPathRecur(int grille[NB_CASES][NB_CASES], int path[MAX_PATHS+1], int bestPath[MAX_PATHS+1], int *minLenght, int lastDir, int iPath, int xPi, int yPi, int *canUseBox){
	int dir = -1;
	int xP, yP;
	int lenght = -1;

	if(iPath >MAX_PATHS || iPath >= *minLenght)
		return LOSE;

	int grilleCpy[NB_CASES][NB_CASES];

	while(1){
		if(lastDir==-1)
			printf("START\n");
		do{
			dir++;
		}while(dir == lastDir || (lastDir != -1 && dir == (lastDir+2)%4));

		if(dir > 3)
			return lenght;
		else{
			printf("%d : %s",iPath, DIRECTION[dir] );
			xP = xPi;
			yP = yPi;
			memcpy(&(grilleCpy[0][0]), &(grille[0][0]), NB_CASES* NB_CASES*sizeof(int));
			path[iPath]=dir;

			int result = move(grilleCpy, dir, &xP, &yP);
			printf(" mv%d\n", result );
			if(result == WIN){
				path[iPath+1] = -1;
				lenght = iPath+1;
				if(lenght > iPath  && lenght < *minLenght){
					printf("Writing new best path : ");
					for(int p = 0; p<lenght+1; p++){
						bestPath[p] = path[p];
						printf("%s ", DIRECTION[bestPath[p]]);
					}
					printf("\n");
					*minLenght = lenght;
				}
				return iPath+1;
			}
			else if(result == MOVED && (xP != xPi || yP != yPi)){
				lenght = findPathRecur(grilleCpy, path, bestPath, minLenght, dir, iPath+1, xP, yP);
				printf("%d : %s lg%d\n",iPath, DIRECTION[dir], lenght);

				if(lenght > 0 && iPath > lenght - 3)
					return lenght;
			}
		}
	}
}


int findPath(int grille[NB_CASES][NB_CASES], int path[MAX_PATHS+1], int bestPath[MAX_PATHS+1], int *minLenght, int canUseBox){
	int xPi, yPi;
	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			if(grille[i][j] == PUMPKIN){
				xPi=i;
				yPi=j;
			}
		}
	}

	findPathRecur(grille, path, bestPath,minLenght, -1, 0, xPi, yPi, &canUseBox);
	printf("lenght : %d\n", minLenght);
	return minLenght;
}

void scanScreen(){
	xCopyScreen();
	if(grilleStart.x == 0){// scan grille size (only first time)
		for(int x=0; x<Screen.cx && grilleStart.x == 0; x+=25){
	        for(int y=0; y<Screen.cy && grilleStart.x == 0; y+=25){

	            _color = RGB2BGR(Screen.pixels[(y * Screen.cx) + x]);
				printf("%d %d : %d %d %d\n", x, y, GetRValue(_color),GetGValue(_color),GetBValue(_color));
	            if(_color == BACKGROUND_COLOR){
	                grilleStart.x = x;
	                while(_color == BACKGROUND_COLOR){
	                    grilleStart.x--;
	                    _color = RGB2BGR(Screen.pixels[(y * Screen.cx) + grilleStart.x]);
	                }
	                grilleStart.x++;

	                do{
	                    grilleSize.x+=25;
	                    _color = RGB2BGR(Screen.pixels[(y * Screen.cx) + grilleStart.x + grilleSize.x]);
	                }while(_color == BACKGROUND_COLOR);

	                while(_color != BACKGROUND_COLOR){
	                    grilleSize.x--;
	                    _color = RGB2BGR(Screen.pixels[(y * Screen.cx) + grilleStart.x + grilleSize.x]);
	                };

	                grilleStart.y = y;
	                do{
	                    grilleStart.y--;
	                    _color = RGB2BGR(Screen.pixels[(grilleStart.y * Screen.cx) + grilleStart.x]);
	                }while(_color == BACKGROUND_COLOR);
	                grilleStart.y++;
	            }
	        }
	    }
		coefSize = grilleSize.x / (float)grilleRef.x;
		grilleSize.y = grilleRef.y * coefSize;
		printf("coef size%f\n", coefSize);
		printf("pos : %ld %ld\n", grilleStart.x, grilleStart.y);
		printf("size : %ld %ld\n", grilleSize.x, grilleSize.y);
	}

	//analyse
	for(int i=0;i<NB_CASES;i++){
		for(int j=0; j<NB_CASES; j++){
			grille_color[i][j] = Screen.pixels[((int)(grilleStart.y + initShift.y*coefSize + (caseDimRef*coefSize) * j) * Screen.cx) + (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i)];
			int found = SDL_FALSE;
			DWORD color = Screen.pixels[((int)(grilleStart.y + (initShift.y+SHIFT_TP)*coefSize + (caseDimRef*coefSize) * j) * Screen.cx) + (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i)];
			if( isTerrain[TELEPORT](color) ){
				grille[i][j] = TELEPORT;
				found = SDL_TRUE;
			}
			if(!found)
				for(int terrain=0; terrain < NB_TERRAIN-1; terrain++){
					if( terrain != TELEPORT && isTerrain[terrain](grille_color[i][j]) ){
						if(terrain == WATER || terrain == HOLE){
							DWORD color = Screen.pixels[((int)(grilleStart.y + (initShift.y+SHIFT_HOLE)*coefSize + (caseDimRef*coefSize) * j) * Screen.cx) + (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i)];
							if( isTerrain[WATER](color) )
								terrain = WATER;
							else
								terrain = HOLE;
						}
						grille[i][j]=terrain;
						found = SDL_TRUE;
						break;
					}
				}
			if(!found)
				for(int terrain=0; terrain < NB_TERRAIN-1; terrain++){
					DWORD color = Screen.pixels[((int)(grilleStart.y + (initShift.y+SHIFT_TP)*coefSize + (caseDimRef*coefSize) * j) * Screen.cx) + (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i)];

					if( terrain != TELEPORT && isTerrain[terrain](color) ){
						grille[i][j]=terrain;
						break;
					}
				}


			//printf("%d %d %d", GetRValue(grille_color[i][j]),GetGValue(grille_color[i][j]),GetBValue(grille_color[i][j]));
			//printf("%d\n", grille[i][j]);

		}
		//printf("\n");
	}
	grille[NB_CASES-1][NB_CASES-1] = WATER;

	//find solution


}

int main(int argc, char** argv)
{
	srand(time(NULL));
	/*HWND hWnd = GetConsoleWindow();
	ShowWindow( hWnd, SW_HIDE );*/
	SDL_Init(SDL_INIT_VIDEO );
	xSetupScreenBitmap();


    printf("size : %d %d\n", (int)Screen.cx, (int)Screen.cy);










	SDL_Window* window = SDL_CreateWindow("Atlentaine Pathfinder", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 130, 130, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	SDL_Texture *texture = IMG_LoadTexture(renderer, "texture.png");
	if( texture == NULL ){
		printf("Erreur lors de la creation de texture %s", SDL_GetError());
		return SDL_FALSE;
	}

	SDL_RenderClear(renderer);
	while(1){


		SDL_Event event;
		while( SDL_PollEvent(&event) ){
			switch( event.type ){
				case SDL_QUIT:
					// fermer
					return 0;
					break;
			}
		}

		int findSolution = SDL_FALSE;
		if((GetKeyState(VK_SPACE) & 0x8000)){//attente espace
			scanScreen();
			findSolution = SDL_TRUE;
		}

		SDL_Rect dest = {0,0, 10, 10};
		SDL_Rect src = {0,0, 39, 39};
		for(int j=0;j<NB_CASES;j++){
			for(int i=0; i<NB_CASES; i++){
				src.x = grille[i][j] * src.w;
				SDL_RenderCopy(renderer, texture, &src, &dest);
				/*SDL_SetRenderDrawColor(renderer, grille_color[i][j].r,grille_color[i][j].g,grille_color[i][j].b,255);
				SDL_RenderFillRect(renderer, &dest);*/
				dest.x+=10;
			}

			dest.y += 10;
			dest.x = 0;
		}

		SDL_RenderPresent(renderer);

		currentTime = SDL_GetTicks();
		while( currentTime - lastTime < 1000./30 )
			currentTime = SDL_GetTicks();

		lastTime = currentTime;

		SDL_RenderClear(renderer);
		if(findSolution){
			int path[MAX_PATHS+1];
			int bestPath[MAX_PATHS+1];
			int minLenght = MAX_PATHS+2;
			int canUseBox = SDL_FALSE;
			findPath(grille, path, bestPath, SDL_FALSE);
			if(minLenght == MAX_PATHS+2){
				canUseBox = SDL_TRUE;
				findPath(grille, path, bestPath, SDL_TRUE);
			}


			for(int i=0; i<MAX_PATHS && bestPath[i]!=-1; i++){
				printf("%s - ",DIRECTION[bestPath[i]]);
			}
			printf("FINI !\n");
		}
	}
    return 0;
}
