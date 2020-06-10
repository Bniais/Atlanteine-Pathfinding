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
int grilleOmg[NB_CASES][NB_CASES]={
	{2,2,1,1,0,0,0,2,2,1,1,1,1},
	{0,0,5,0,0,5,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,5},
	{1,0,0,0,0,0,0,0,5,0,5,0,0},
	{1,5,0,0,0,0,5,5,0,0,0,0,1},
	{0,0,0,0,5,1,0,0,0,0,0,0,0},
	{5,0,0,0,0,0,0,0,0,6,0,0,0},
	{0,0,0,5,0,0,0,0,0,0,0,0,0},
	{0,5,0,0,0,0,6,0,0,0,0,0,0},
	{1,0,0,1,5,0,0,4,0,5,1,0,0},
	{2,0,0,0,0,0,0,0,0,0,0,0,0},
	{2,0,0,0,0,0,5,0,0,0,0,0,0},
	{2,2,2,2,1,0,0,0,0,5,0,5,3},
};

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
int moveP(int grilleCpy[NB_CASES][NB_CASES], int dir, int *xP, int *yP, int* canUseBox){
	printf("moving %d %d \n", *xP, *yP);
	int move = 0;
	if(dir==0){//down
		for(move=1; *yP+move < NB_CASES && grilleCpy[*xP][*yP+move] == PLAIN; move++);
		if (*yP+move >= NB_CASES || grilleCpy[*xP][*yP+move] == WATER)
			return LOSE;
		else if (grilleCpy[*xP][*yP+move] == HOLE)
			return WIN;
		else if( move == 1 && grilleCpy[*xP][*yP+move] == CAISSE && *canUseBox && *yP+move+1 < NB_CASES && grilleCpy[*xP][*yP+move+1] == PLAIN){
			printf("moved caisse\n\n\n\n");grilleCpy[*xP][*yP+move+1] = CAISSE;
			grilleCpy[*xP][*yP+move] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*yP)++;
			return MOVED;
		}
		else if (grilleCpy[*xP][*yP+move] == TELEPORT){	printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP || j != *yP+move)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i][j+1] = PUMPKIN;
						*xP = i;
						*yP = j+1;
						int b = 0;
						return moveP(grilleCpy, dir, xP, yP, &b);
					}
				}
			}
		}
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP][*yP+move-1] = PUMPKIN;
			*yP += move-1;
			return MOVED;
		}
	}
	else if(dir==1){//left
		for(move=1; *xP-move >=0 && grilleCpy[*xP-move][*yP] == PLAIN; move++);
		if (*xP-move < 0 || grilleCpy[*xP-move][*yP] == WATER)
			return LOSE;
		else if (grilleCpy[*xP-move][*yP] == HOLE)
			return WIN;
		else if(move == 1 && grilleCpy[*xP-move][*yP] == CAISSE && *canUseBox && *xP-move-1 >=0 && grilleCpy[*xP-move-1][*yP] == PLAIN){
			printf("moved caisse\n\n\n\n");grilleCpy[*xP-move-1][*yP] = CAISSE;
			grilleCpy[*xP-move][*yP] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*xP)--;
			return MOVED;
		}
		else if (grilleCpy[*xP-move][*yP] == TELEPORT){printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP-move || j != *yP)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i-1][j] = PUMPKIN;
						*xP = i-1;
						*yP = j;
						int b = 0;
						return moveP(grilleCpy, dir, xP, yP, &b);
					}
				}
			}
		}
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP-move+1][*yP] = PUMPKIN;
			*xP -= move-1;
			return MOVED;
		}
	}
	else if(dir==2){//up
		for(move=1; *yP-move >=0 && grilleCpy[*xP][*yP-move] == PLAIN; move++);
		if (*yP-move < 0 || grilleCpy[*xP][*yP-move] == WATER)
			return LOSE;
		else if (grilleCpy[*xP][*yP-move] == HOLE)
			return WIN;
		else if(move == 1 && grilleCpy[*xP][*yP-move] == CAISSE && *canUseBox && *yP-move-1 >=0 && grilleCpy[*xP][*yP-move-1] == PLAIN){
			printf("moved caisse\n\n\n\n");grilleCpy[*xP][*yP-move-1] = CAISSE;
			grilleCpy[*xP][*yP-move] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*yP)--;
			return MOVED;
		}
		else if (grilleCpy[*xP][*yP-move] == TELEPORT){printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP || j != *yP-move)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i][j-1] = PUMPKIN;
						*xP = i;
						*yP = j-1;
						int b = 0;
						return moveP(grilleCpy, dir, xP, yP, &b);
					}
				}
			}
		}
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP][*yP-move+1] = PUMPKIN;
			*yP -= move-1;
			return MOVED;
		}
	}
	else {//right
		for(move=1; *xP+move < NB_CASES && grilleCpy[*xP+move][*yP] == PLAIN; move++);
		if (*xP+move >= NB_CASES || grilleCpy[*xP+move][*yP] == WATER)
			return LOSE;
		else if (grilleCpy[*xP+move][*yP] == HOLE)
			return WIN;
		else if(move == 1 && grilleCpy[*xP+move][*yP] == CAISSE && *canUseBox && *xP+move+1 < NB_CASES && grilleCpy[*xP+move+1][*yP] == PLAIN){
			printf("moved caisse\n\n\n\n");
			grilleCpy[*xP+move+1][*yP] = CAISSE;
			grilleCpy[*xP+move][*yP] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*xP)++;
			return MOVED;
		}
		else if (grilleCpy[*xP+move][*yP] == TELEPORT){printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP+move || j != *yP)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i+1][j] = PUMPKIN;
						*xP = i+1;
						*yP = j;
						int b=0;
						return moveP(grilleCpy, dir, xP, yP, &b);
					}
				}
			}
		}
		else{
			grilleCpy[*xP][*yP] = PLAIN;
			grilleCpy[*xP+move-1][*yP] = PUMPKIN;
			*xP += move-1;
			return MOVED;
		}
	}
	return MOVED;
}

int findPathRecur(int grille[NB_CASES][NB_CASES], int path[MAX_PATHS+1], int bestPath[MAX_PATHS+1], int *minLenght, int lastDir, int iPath, int xPi, int yPi, int *canUseBox){
	int dir = -1;
	int xP, yP;
	int lenght = -1;
	int canUseBoxI = *canUseBox;

	if(iPath >MAX_PATHS || iPath >= *minLenght)
		return LOSE;

	int grilleCpy[NB_CASES][NB_CASES];

	while(1){
		*canUseBox = canUseBoxI;
		if(lastDir==-1)
			printf("START\n");

		do{
			dir++;
			printf("dir : %d (%d) box : %d\n", dir, lastDir, *canUseBox );
		}while( dir == -1 || (dir == lastDir && !(*canUseBox)) || (lastDir != -1 && dir == (lastDir+2)%4) );

		printf("%d\n",dir );
		if(dir > 3)
			return LOSE;
		else{

			xP = xPi;
			yP = yPi;
			printf("%d : %s, %d %d",iPath, DIRECTION[dir], xP, yP );
			memcpy(&(grilleCpy[0][0]), &(grille[0][0]), NB_CASES* NB_CASES*sizeof(int));
			path[iPath]=dir;

			int result = moveP(grilleCpy, dir, &xP, &yP, canUseBox);
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
				*canUseBox = canUseBoxI;
				return iPath+1;
			}
			else if(result == MOVED && (xP != xPi || yP != yPi)){
				lenght = findPathRecur(grilleCpy, path, bestPath, minLenght, dir, iPath+1, xP, yP, canUseBox);
				printf("%d : %s lg%d\n",iPath, DIRECTION[dir], lenght);

				if(lenght > 0 && iPath > lenght - 3){
					*canUseBox = canUseBoxI;
					return lenght;
				}

			}
		}
	}
}


int findPath(int grille[NB_CASES][NB_CASES], int path[MAX_PATHS+1], int bestPath[MAX_PATHS+1], int *minLenght, int canUseBox){
	int xPi = -1, yPi = -1;
	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			if(grille[i][j] == PUMPKIN){
				xPi=i;
				yPi=j;
			}
		}
	}
	if(xPi == -1){
		xPi=NB_CASES-1;
		yPi=NB_CASES-1;
		grille[NB_CASES-1][NB_CASES-1] = PUMPKIN;
	}


	findPathRecur(grille, path, bestPath,minLenght, -1, 0, xPi, yPi, &canUseBox);
	printf("lenght : %d\n", *minLenght);
	return *minLenght;
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
						if(terrain == WATER ){
							DWORD color = Screen.pixels[((int)(grilleStart.y + (initShift.y+SHIFT_HOLE)*coefSize + (caseDimRef*coefSize) * j) * Screen.cx) + (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i)];
							if( isTerrain[WATER](color) )
								terrain = WATER;
							else
								terrain = HOLE;
						}
						else if(terrain == HOLE || terrain == ROCK ){
							DWORD color = Screen.pixels[((int)(grilleStart.y + (initShift.y+SHIFT_HOLE)*coefSize + (caseDimRef*coefSize) * j) * Screen.cx) + (int)(grilleStart.x + initShift.x*coefSize + (caseDimRef*coefSize) * i)];
							if( isTerrain[WATER](color) )
								terrain = WATER;
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

	if(grille[NB_CASES-1][NB_CASES-1] == TELEPORT)
		grille[NB_CASES-1][NB_CASES-1] = WATER;

	//protection pumpkins
	int xPi = -1;
	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			if(grille[i][j] == PUMPKIN){
				if(xPi != -1)
					printf("SEVERAL PUMPKINS\n");
				xPi=i;
			}
		}
	}
	if(xPi == -1){
		grille[NB_CASES-1][NB_CASES-1] = PUMPKIN;
	}

	//protection hole

	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			if(grille[i][j] == WATER && ((i!=0 && i!=NB_CASES-1) || (j!=0 && j!=NB_CASES-1))){
				grille[i][j] = HOLE;
			}
		}
	}

	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			if(grille[i][j] == HOLE){
				if(xPi != -1){
					xPi=100;
				}
				else{
					xPi=i;
				}
			}
		}
	}
	if(xPi == -1){
		grille[5][5] = HOLE;
	}
	else if(xPi == 100){
		for(int j=0;j<NB_CASES;j++){
			for(int i=0;i<NB_CASES;i++){
				if(grille[i][j] == HOLE && ((i==0 || i==NB_CASES-1) || (j==0 || j==NB_CASES-1)) ){
					grille[i][j] = WATER;
				}
			}
		}
	}

	/*for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			grille[i][j] = grilleOmg[j][i];
		}
	}*/

}

int validGrille(int grille[NB_CASES][NB_CASES]){
	int cpt[NB_TERRAIN-1]={0,0,0,0,0,0,0};
	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			cpt[grille[i][j]]++;
		}
	}

	return(cpt[HOLE] == 1 && cpt[PUMPKIN] == 1 && (cpt[TELEPORT] == 0 || cpt[TELEPORT] == 2));
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	/*HWND hWnd = GetConsoleWindow();
	ShowWindow( hWnd, SW_HIDE );*/
	SDL_Init(SDL_INIT_VIDEO );
	xSetupScreenBitmap();


    printf("size : %d %d\n", (int)Screen.cx, (int)Screen.cy);









	SDL_Point mouse;
	SDL_Window* window = SDL_CreateWindow("Atlentaine Pathfinder", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 260, 260, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	SDL_Texture *texture = IMG_LoadTexture(renderer, "texture.png");
	if( texture == NULL ){
		printf("Erreur lors de la creation de texture %s", SDL_GetError());
		return SDL_FALSE;
	}
	int scroll = 0;
	SDL_RenderClear(renderer);
	while(1){
		scroll = 0;
		SDL_Event event;
		while( SDL_PollEvent(&event) ){
			switch( event.type ){
				case SDL_QUIT:
					// fermer
					return 0;
					break;
					// SI EVENMENT DE ROULETTE SOURIS
				case SDL_MOUSEWHEEL:
					scroll = event.wheel.y;
					printf("%d\n",scroll );
					break;
			}
		}

		int findSolution = SDL_FALSE;
		if((GetKeyState(VK_SPACE) & 0x8000)){// espace
			scanScreen();
			if(validGrille(grille))
				findSolution = SDL_TRUE;
		}
		else if((GetKeyState(VK_RETURN) & 0x8000)){// entrer
			if(validGrille(grille))
				findSolution = SDL_TRUE;
		}

		SDL_Rect dest = {0,0, 21, 21};
		SDL_Rect src = {0,0, 39, 39};
		for(int j=0;j<NB_CASES;j++){
			for(int i=0; i<NB_CASES; i++){
				src.x = grille[i][j] * src.w;
				SDL_RenderCopy(renderer, texture, &src, &dest);
				/*SDL_SetRenderDrawColor(renderer, grille_color[i][j].r,grille_color[i][j].g,grille_color[i][j].b,255);
				SDL_RenderFillRect(renderer, &dest);*/
				dest.x+=20;
			}

			dest.y += 20;
			dest.x = 0;
		}
		SDL_CaptureMouse(SDL_TRUE);
		SDL_GetMouseState(&(mouse.x),&(mouse.y));
		//printf("%d %d\n", mouse.x, mouse.y);

		if(mouse.x < 0)
			mouse.x-=20;
		if(mouse.y < 0)
			mouse.y-=20;

		mouse.x/=20;
		mouse.y/=20;
		//printf("%d %d\n", mouse.x, mouse.y);
		dest.x=mouse.x*20;
		dest.y=mouse.y*20;
		SDL_RenderDrawRect(renderer, &dest);

		if(mouse.x >= 0 && mouse.x < NB_CASES && mouse.y >= 0 && mouse.y < NB_CASES ){
			grille[mouse.x][mouse.y] += scroll;
			while(grille[mouse.x][mouse.y] < 0)
				grille[mouse.x][mouse.y]+=(NB_TERRAIN-1);

			grille[mouse.x][mouse.y] = grille[mouse.x][mouse.y]%(NB_TERRAIN-1);
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
			findPath(grille, path, bestPath, &minLenght, canUseBox);
			if(minLenght == MAX_PATHS+2){
				printf("FINDING WITH BOX\n\n\n\n\n\n" );
				canUseBox = SDL_TRUE;
				findPath(grille, path, bestPath, &minLenght, canUseBox);
			}


			for(int i=0; i<MAX_PATHS && bestPath[i]!=-1; i++){
				printf("%s - ",DIRECTION[bestPath[i]]);
			}
			printf("FINI !\n");
		}
	}
    return 0;
}
