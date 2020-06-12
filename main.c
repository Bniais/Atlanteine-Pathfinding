#include<windows.h>
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<stdio.h>
#include <time.h>

#define NB_CASES 13
#define MAX_PATHS 50
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
int wrongTiles[NB_CASES][NB_CASES];
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
	//printf("moving %d %d \n", *xP, *yP);
	int move = 0;
	if(dir==0){//down
		for(move=1; *yP+move < NB_CASES && grilleCpy[*xP][*yP+move] == PLAIN; move++);
		if (*yP+move >= NB_CASES || grilleCpy[*xP][*yP+move] == WATER)
			return LOSE;
		else if (grilleCpy[*xP][*yP+move] == HOLE)
		{
			*yP += move;
			return WIN;
		}
		else if( move == 1 && grilleCpy[*xP][*yP+move] == CAISSE && *canUseBox && *yP+move+1 < NB_CASES && grilleCpy[*xP][*yP+move+1] == PLAIN){
			//printf("moved caisse\n\n\n\n");
			grilleCpy[*xP][*yP+move+1] = CAISSE;
			grilleCpy[*xP][*yP+move] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*yP)++;
			return MOVED;
		}
		else if (grilleCpy[*xP][*yP+move] == TELEPORT){	//printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP || j != *yP+move)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i][j+1] = PUMPKIN;
						*xP = i;
						*yP = j+1;
						int b = 0;
						if(grille[*xP][*yP] == HOLE)
							return -WIN;
						return -moveP(grilleCpy, dir, xP, yP, &b);
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
			{
				*xP -= move;
				return WIN;
			}
		else if(move == 1 && grilleCpy[*xP-move][*yP] == CAISSE && *canUseBox && *xP-move-1 >=0 && grilleCpy[*xP-move-1][*yP] == PLAIN){
			//printf("moved caisse\n\n\n\n");
			grilleCpy[*xP-move-1][*yP] = CAISSE;
			grilleCpy[*xP-move][*yP] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*xP)--;
			return MOVED;
		}
		else if (grilleCpy[*xP-move][*yP] == TELEPORT){//printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP-move || j != *yP)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i-1][j] = PUMPKIN;
						*xP = i-1;
						*yP = j;
						int b = 0;
						if(grille[*xP][*yP] == HOLE)
							return -WIN;
						return -moveP(grilleCpy, dir, xP, yP, &b);
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
			{
				*yP -= move;
				return WIN;
			}
		else if(move == 1 && grilleCpy[*xP][*yP-move] == CAISSE && *canUseBox && *yP-move-1 >=0 && grilleCpy[*xP][*yP-move-1] == PLAIN){
			//printf("moved caisse\n\n\n\n");
			grilleCpy[*xP][*yP-move-1] = CAISSE;
			grilleCpy[*xP][*yP-move] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*yP)--;
			return MOVED;
		}
		else if (grilleCpy[*xP][*yP-move] == TELEPORT){//printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP || j != *yP-move)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i][j-1] = PUMPKIN;
						*xP = i;
						*yP = j-1;
						int b = 0;
						if(grille[*xP][*yP] == HOLE)
							return -WIN;
						return -moveP(grilleCpy, dir, xP, yP, &b);
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
			{
				printf("hole\n");
				*xP += move;
				return WIN;
			}
		else if(move == 1 && grilleCpy[*xP+move][*yP] == CAISSE && *canUseBox && *xP+move+1 < NB_CASES && grilleCpy[*xP+move+1][*yP] == PLAIN){
			//printf("moved caisse\n\n\n\n");
			grilleCpy[*xP+move+1][*yP] = CAISSE;
			grilleCpy[*xP+move][*yP] = PUMPKIN;
			grilleCpy[*xP][*yP] = PLAIN;
			*canUseBox = SDL_FALSE;
			(*xP)++;
			return MOVED;
		}
		else if (grilleCpy[*xP+move][*yP] == TELEPORT){//printf("tp\n");
			for(int j=0;j<NB_CASES;j++){
				for(int i=0;i<NB_CASES;i++){
					if(grille[i][j] == TELEPORT && (i != *xP+move || j != *yP)  ){
						grilleCpy[*xP][*yP] = PLAIN;
						grilleCpy[i+1][j] = PUMPKIN;
						*xP = i+1;
						*yP = j;
						int b=0;
						if(grille[*xP][*yP] == HOLE)
							return -WIN;
						return -moveP(grilleCpy, dir, xP, yP, &b);
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

int findPathRecur(int grille[NB_CASES][NB_CASES], int path[MAX_PATHS+1], int bestPath[MAX_PATHS+1], SDL_Point pos[MAX_PATHS+1], SDL_Point bestPos[MAX_PATHS+1],int *minLenght, int lastDir, int iPath, int xPi, int yPi, int *canUseBox){
	int dir = -1;
	int xP, yP;
	int lenght = -1;
	int canUseBoxI = *canUseBox;


	if(iPath >MAX_PATHS || iPath >= *minLenght)
		return LOSE;

	int grilleCpy[NB_CASES][NB_CASES];

	while(1){
		*canUseBox = canUseBoxI;
		/*if(lastDir==-1)
			printf("START\n");*/
		do{
			dir++;
			//printf("dir : %d (%d) box : %d\n", dir, lastDir, *canUseBox );
		}while( dir == -1 || (dir == lastDir && !(*canUseBox)) || (lastDir != -1 && dir == (lastDir+2)%4) );

		//printf("%d\n",dir );
		if(dir > 3)
			return LOSE;
		else{
			if(iPath > 0)
				pos[iPath-1] = (SDL_Point){xPi, yPi};

			if(iPath>8 && (pos[iPath-1].x==pos[iPath-5].x && pos[iPath-5].x==pos[iPath-9].x) && (pos[iPath-1].y==pos[iPath-5].y && pos[iPath-5].y==pos[iPath-9].y))
				return LOSE;
			xP = (xPi<0?xPi+100:xPi);
			yP = (yPi<0?yPi+100:yPi);
			printf("%d : %s, %d %d \n",iPath, DIRECTION[dir], xP, yP );
			memcpy(&(grilleCpy[0][0]), &(grille[0][0]), NB_CASES* NB_CASES*sizeof(int));
			path[iPath]=dir;

			int result = moveP(grilleCpy, dir, &xP, &yP, canUseBox);
			//printf(" mv%d\n", result );
			if(abs(result) == WIN){
				path[iPath+1] = -1;
				pos[iPath] = (SDL_Point){(result<0?-100:0)+xP, (result<0?-100:0)+yP};
				pos[iPath+1] = (SDL_Point){-1, -1};
				lenght = iPath+1;
				if(lenght > iPath  && lenght < *minLenght){
					//printf("Writing new best path : ");
					for(int p = 0; p<lenght+1; p++){
						bestPos[p] = pos[p];
						bestPath[p] = path[p];
						//printf("%s ", DIRECTION[bestPath[p]]);
					}
					//printf("\n");
					*minLenght = lenght;
				}
				*canUseBox = canUseBoxI;
				return iPath+1;
			}
			else if(abs(result) == MOVED && (xP != (xPi<0?xPi+100:xPi) || yP != (yPi<0?yPi+100:yPi))){
				lenght = findPathRecur(grilleCpy, path, bestPath, pos, bestPos, minLenght, dir, iPath+1, (result<0?-100:0)+xP, (result<0?-100:0)+yP, canUseBox);
				//printf("%d : %s lg%d\n",iPath, DIRECTION[dir], lenght);

				if(lenght > 0 && iPath > lenght - 3){
					*canUseBox = canUseBoxI;
					return lenght;
				}

			}
		}
	}
}


int findPath(int grille[NB_CASES][NB_CASES], int path[MAX_PATHS+1], int bestPath[MAX_PATHS+1],  SDL_Point pos[MAX_PATHS+1], SDL_Point bestPos[MAX_PATHS+1],int *minLenght, int canUseBox){
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


	findPathRecur(grille, path, bestPath, pos, bestPos, minLenght, -1, 0, xPi, yPi, &canUseBox);
	//printf("lenght : %d\n", *minLenght);
	return *minLenght;
}

int wrongArroundingTile(int wrongTiles[NB_CASES][NB_CASES], int i, int j){
	return (
		wrongTiles[i-1][j-1] || wrongTiles[i-1][j] || wrongTiles[i-1][j+1] ||
		wrongTiles[i][j-1] || wrongTiles[i][j] || wrongTiles[i][j+1] ||
		wrongTiles[i+1][j-1] || wrongTiles[i+1][j] || wrongTiles[i+1][j+1]
	);
}

void scanScreen(int scanWrong, int wrongTiles[NB_CASES][NB_CASES]){
	xCopyScreen();
	if(grilleStart.x == 0){// scan grille size (only first time)
		for(int x=0; x<Screen.cx && grilleStart.x == 0; x+=25){
	        for(int y=0; y<Screen.cy && grilleStart.x == 0; y+=25){

	            _color = RGB2BGR(Screen.pixels[(y * Screen.cx) + x]);
				//printf("%d %d : %d %d %d\n", x, y, GetRValue(_color),GetGValue(_color),GetBValue(_color));
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
			int  wrongArround = 0;
			if(!scanWrong || (wrongArround=wrongArroundingTile(wrongTiles, i, j) ) ){
				int remindTile = grille[i][j];
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
				if(scanWrong && wrongArround && remindTile != TELEPORT && grille[i][j]==TELEPORT)
					 grille[i][j] = remindTile;
			}

		}
		//printf("\n");
	}

	if(grille[NB_CASES-1][NB_CASES-1] == TELEPORT)
		grille[NB_CASES-1][NB_CASES-1] = WATER;

	/*//protection pumpkins

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
	}*/

	//protection hole
	int xPi = -1;
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
	if(xPi == 100){
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
	int flag = 0;

	if(cpt[PUMPKIN] > 1)
		flag++;

	if(cpt[HOLE] > 1)
		flag+=2;

	if(cpt[TELEPORT] != 0 && cpt[TELEPORT] != 2)
		flag+=4;

	if(cpt[HOLE] == 0)
		flag+=8;

	if(cpt[PUMPKIN] == 0)
		flag+=16;


	return flag;
}

void getWrongTiles(int grille[NB_CASES][NB_CASES], int wrongTiles[NB_CASES][NB_CASES], int flagWrong){
	//printf("flag : %d\n",flagWrong );
	for(int j=0;j<NB_CASES;j++){
		for(int i=0;i<NB_CASES;i++){
			if( ((flagWrong%2 == 1) && grille[i][j] == PUMPKIN) || ((flagWrong%4 >= 2) && grille[i][j] == HOLE) || (flagWrong%8 >= 4 && grille[i][j] == TELEPORT))
				wrongTiles[i][j] = 1;
			else
				wrongTiles[i][j] = 0;
		}
	}
}

void SDL_RenderDrawThiccLine(SDL_Renderer* renderer,int x1,int y1,int x2,int y2){
	SDL_RenderDrawLine(renderer, x1, y1,x2,y2);
	if(x1==x2){
		SDL_RenderDrawLine(renderer, x1-1, y1,x2-1,y2);
		SDL_RenderDrawLine(renderer, x1+1, y1,x2+1,y2);

	}
	else{
		SDL_RenderDrawLine(renderer, x1, y1-1,x2,y2-1);
		SDL_RenderDrawLine(renderer, x1, y1+1,x2,y2+1);
	}
}
int signOf( float f){
	return (f > 0) ? 1 : ((f < 0 )? -1 : 0);
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	HWND hWnd = GetConsoleWindow();
	ShowWindow( hWnd, SW_HIDE );
	SDL_Init(SDL_INIT_VIDEO );
	xSetupScreenBitmap();


    printf("size : %d %d\n", (int)Screen.cx, (int)Screen.cy);

	SDL_Point pos[MAX_PATHS+1];
	SDL_Point bestPos[MAX_PATHS+1];
	SDL_Point bestPosBox[MAX_PATHS+1];
	int path[MAX_PATHS+1];
	int bestPath[MAX_PATHS+1];
	int bestPathBox[MAX_PATHS+1];
	int xPump, yPump;
	int grilleValide = SDL_FALSE;
	int drawBoxPath = SDL_FALSE;
	int rdyToSpace, rdyToTab, rdyToAlt;

	SDL_Point mouse;
	SDL_Window* window = SDL_CreateWindow("Atlentaine Pathfinder", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 260, 297, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	SDL_Texture *texture = IMG_LoadTexture(renderer, "texture.png");
	if( texture == NULL ){
		printf("Erreur lors de la creation de texture %s", SDL_GetError());
		return SDL_FALSE;
	}
	int scroll;
	int flagWrong = 1+2+4+8+16;
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
					//printf("%d\n",scroll );
					break;
			}
		}

		int findSolution = SDL_FALSE;
		int grilleChanged = SDL_FALSE;
		int buildTile = -1;

		for(int i=0x31; i<0x31+NB_TERRAIN-1; i++)
			if((GetKeyState(i) & 0x8000))
				buildTile = i-0x31;

		if( (GetKeyState(0x52) & 0x8000)){//r
			for(int j=0;j<NB_CASES;j++){
				for(int i=0; i<NB_CASES; i++){
					grille[i][j] = 0;
				}
			}
			grilleChanged = SDL_TRUE;
			flagWrong = validGrille(grille);
		}
		if( (GetKeyState(VK_ESCAPE) & 0x8000)){
			grilleStart.x = 0;
		}
		if( (GetKeyState(VK_SPACE) & 0x8000)){// espace
			if(rdyToSpace){
				grilleChanged  = SDL_TRUE;
				scanScreen(0, wrongTiles);
				if(!(flagWrong = validGrille(grille))){
					//grilleValide = SDL_TRUE;
					findSolution = SDL_TRUE;
				}
				else{
					grilleValide = SDL_FALSE;
				}
				rdyToSpace = SDL_FALSE;
			}

		}
		else{
			rdyToSpace = SDL_TRUE;
		}

		if( (GetKeyState(VK_MENU) & 0x8000)){// espace
			if(rdyToAlt){
				grilleChanged  = SDL_TRUE;
				scanScreen(1, wrongTiles);
				if(!(flagWrong = validGrille(grille))){
					//grilleValide = SDL_TRUE;
					findSolution = SDL_TRUE;
				}
				else{
					grilleValide = SDL_FALSE;
				}
				rdyToAlt = SDL_FALSE;
			}

		}
		else{
			rdyToAlt = SDL_TRUE;
		}

		if((GetKeyState(VK_TAB) & 0x8000) ){// entrer
			if(rdyToTab){
				grilleChanged  = SDL_TRUE;
				if(!(flagWrong = validGrille(grille))){
					//grilleValide = SDL_TRUE;
					findSolution = SDL_TRUE;
				}
				else{
					grilleValide = SDL_FALSE;
				}
				rdyToTab = SDL_FALSE;
			}
		}
		else{
			rdyToTab = SDL_TRUE;
		}

		if(grilleChanged){
			xPump = -1;
			yPump = -1;
			for(int j=0;j<NB_CASES && xPump == -1;j++){
				for(int i=0;i<NB_CASES && xPump == -1;i++){
					if(grille[i][j] == PUMPKIN){
						xPump = i;
						yPump = j;
					}
				}
			}
			getWrongTiles(grille, wrongTiles, flagWrong);
		}


		SDL_SetRenderDrawColor(renderer, 255,0,0,255);
		SDL_Rect dest = {0,0, 20, 20};
		SDL_Rect src = {0,0, 40, 40};
		for(int j=0;j<NB_CASES;j++){
			for(int i=0; i<NB_CASES; i++){
				src.x = grille[i][j] * src.w;
				SDL_RenderCopy(renderer, texture, &src, &dest);
				/*SDL_SetRenderDrawColor(renderer, grille_color[i][j].r,grille_color[i][j].g,grille_color[i][j].b,255);
				SDL_RenderFillRect(renderer, &dest);*/
				if(wrongTiles[i][j]){
					SDL_RenderDrawRect(renderer, &dest);
				}

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
		SDL_SetRenderDrawColor(renderer, 0,0,0,255);
		SDL_RenderDrawRect(renderer, &dest);

		dest.w=260; dest.h=37; src.x=0; src.w=280;
		dest.y=260; dest.x=0;
		SDL_RenderCopy(renderer, texture, &src, &dest);

		dest.x = 0; dest.y =0; dest.h= 260; dest.w = 260;
		SDL_RenderDrawRect(renderer, &dest);
		dest.h= 37;
		dest.y = 260;

		if(mouse.x >= 0 && mouse.x < NB_CASES && mouse.y >= 0 && mouse.y < NB_CASES ){
			if(buildTile !=-1)
				grille[mouse.x][mouse.y] = buildTile;
			grille[mouse.x][mouse.y] += scroll;
			if(scroll){
				wrongTiles[mouse.x][mouse.y] = 0;
				grilleValide = SDL_FALSE;
			}

			while(grille[mouse.x][mouse.y] < 0)
				grille[mouse.x][mouse.y]+=(NB_TERRAIN-1);

			if(grille[mouse.x][mouse.y]==HOLE && flagWrong%16 >= 8 )
				flagWrong-=8;

			if(grille[mouse.x][mouse.y]==PUMPKIN && flagWrong%32 >= 16 )
				flagWrong-=16;

			grille[mouse.x][mouse.y] = grille[mouse.x][mouse.y]%(NB_TERRAIN-1);

			dest.w = 37; src.w=40; src.x = grille[mouse.x][mouse.y] * 40;
			dest.x = grille[mouse.x][mouse.y] * 37;
			SDL_RenderDrawRect(renderer, &dest);
		}
		dest.w = 37; src.w=40;
		SDL_SetRenderDrawColor(renderer, 255,0,0,255);
		if(flagWrong%16 >= 8){
			dest.x = HOLE * 37;
			SDL_RenderDrawRect(renderer, &dest);
		}
		if(flagWrong%32 >= 16){
			dest.x = PUMPKIN * 37;
			SDL_RenderDrawRect(renderer, &dest);
		}


		if(grilleValide){

			if(bestPos[0].x < 0){
				//printf("first tp\n");
				for(int i=0; i<NB_CASES; i++){
					if(grille[xPump][i] == TELEPORT && (bestPath[0]!=1 && bestPath[0]!=3)){
						SDL_RenderDrawThiccLine(renderer, xPump * 20 +10, yPump * 20+10, xPump *20+10, i *20+10);
						break;
					}
					else if(grille[i][yPump] == TELEPORT && (bestPath[0]!=0 && bestPath[0]!=2)){
						SDL_RenderDrawThiccLine(renderer, xPump * 20 +10, yPump * 20+10, i *20+10, yPump *20+10);
						break;
					}
				}
				for(int j=0;j<NB_CASES ;j++){
					for(int i=0; i<NB_CASES ; i++){
						if(grille[i][j] == TELEPORT && (j!=yPump || (bestPath[0]!=1 && bestPath[0]!=3)) && (i!=xPump || (bestPath[0]!=0 && bestPath[0]!=2))){
							//printf("tp out %d %d\n", i, j );
							SDL_RenderDrawThiccLine(renderer, i * 20 +10, j * 20+10, (bestPos[0].x+100) *20+10, (bestPos[0].y+100) *20+10);
						}
					}
				}
			}else{
				SDL_RenderDrawThiccLine(renderer, xPump * 20 +10, yPump * 20+10, bestPos[0].x *20+10, bestPos[0].y *20+10);
			}

			int r=255,g=0, b=0;
			int sR=-1, sG = 0, sB=1;
			for(int iTraj=1; iTraj<MAX_PATHS && bestPath[iTraj]!=-1; iTraj++){
				r+=sR*30;
				if(r<0 || r>255){
					if(r<0)
						r=0;
					else
						r=255;
					sR*=-1;
					if(sG==0)
						sG=1;
				}
				g += sG *30;
				if(g<0 || g>255){
					if(g<0)
						g=0;
					else
						g=255;
					sG*=-1;
				}
				b += sB *30;
				if(b<0 || b>255){
					if(b<0)
						b=0;
					else
						b=255;
					sB*=-1;
				}
				printf("%d %d (path : %s) : ",iTraj, bestPos[iTraj].x, DIRECTION[bestPath[iTraj]]);
				SDL_SetRenderDrawColor(renderer, r,g,b,255);
				if(bestPos[iTraj].x < 0){
					//printf("aie %d %d \n", bestPos[iTraj].x, bestPos[iTraj].y);
					SDL_Point realPosM = {(bestPos[iTraj-1].x<0?bestPos[iTraj-1].x+100:bestPos[iTraj-1].x), (bestPos[iTraj-1].y<0?bestPos[iTraj-1].y+100:bestPos[iTraj-1].y)};
					//printf("real %d %d\n", realPosM.x, realPosM.y);
					//i-1 vers tp
					for(int k=0; k<NB_CASES; k++){
						if(grille[realPosM.x][k] == TELEPORT)
							SDL_RenderDrawThiccLine(renderer, realPosM.x * 20 +10, realPosM.y * 20+10, realPosM.x *20+10, k *20+10);
						else if(grille[k][realPosM.y] == TELEPORT)
							SDL_RenderDrawThiccLine(renderer, realPosM.x * 20 +10, realPosM.y * 20+10, k *20+10, realPosM.y *20+10);
					}
					//tp vers i
					for(int j=0;j<NB_CASES ;j++){
						for(int k=0; k<NB_CASES ; k++){
							if(grille[k][j] == TELEPORT && (j!= realPosM.y || (bestPath[iTraj]!=1 && bestPath[iTraj]!=3)) && (k!= realPosM.x || (bestPath[iTraj]!=0 && bestPath[iTraj]!=2))){
								printf("tp vers i : %d %d %d %d ", k * 20 +10, j * 20+10, (bestPos[iTraj].x+100) *20+10, (bestPos[iTraj].y+100) *20+10);
								SDL_RenderDrawThiccLine(renderer, k * 20 +10, j * 20+10, (bestPos[iTraj].x+100) *20+10, (bestPos[iTraj].y+100) *20+10);
							}
						}
					}
					/*for(int j=0;j<NB_CASES ;j++){
						for(int i=0; i<NB_CASES ; i++){
							if(grille[i][j] == TELEPORT && j!=realPosM.y && i!=realPosM.x){
								printf(" idk :%d %d %d %d", i * 20 +10, j * 20+10, (bestPos[iTraj].x+100) *20+10, (bestPos[iTraj].y+100) *20+10);
								SDL_RenderDrawThiccLine(renderer, i * 20 +10, j * 20+10, (bestPos[iTraj].x+100) *20+10, (bestPos[iTraj].y+100) *20+10);
							}
						}
					}*/
				}
				else{
					SDL_RenderDrawThiccLine(renderer, (bestPos[iTraj-1].x<0?bestPos[iTraj-1].x+100:bestPos[iTraj-1].x) * 20+10, (bestPos[iTraj-1].y<0?bestPos[iTraj-1].y+100:bestPos[iTraj-1].y) * 20+10, bestPos[iTraj].x *20+10, bestPos[iTraj].y *20+10);
				}
				//printf("%s (%d %d)- ",DIRECTION[bestPath[i]], bestPos[i].x, bestPos[i].y);
				printf("\n");
				if(bestPos[iTraj].x >= 0 && grille[bestPos[iTraj].x][bestPos[iTraj].y] == CAISSE){
					int dX = (bestPath[iTraj] == 1 ? -1: (bestPath[iTraj] == 3 ?1:0));
					int dY = (bestPath[iTraj] == 0 ? 1: (bestPath[iTraj] == 2 ?-1:0));
					dest.w = 20; dest.h=20; dest.x = 20*(bestPos[iTraj].x+dX); dest.y = 20*(bestPos[iTraj].y+dY);
					src.w = 40; src.h =40; src.x = UNKNOWN * 40;
					SDL_RenderCopy(renderer, texture, &src, &dest);
					SDL_SetRenderDrawColor(renderer, 0x56,0x5d,0x36, 255);
					if(dX==1)
						SDL_RenderDrawThiccLine(renderer, dest.x - 10, dest.y + 10, dest.x - 10 + dX*20, dest.y + 10 + dY*20);
					if(dX==-1)
						SDL_RenderDrawThiccLine(renderer, dest.x + 30, dest.y + 10, dest.x + 30 + dX*20, dest.y + 10 + dY*20);

					if(dY==1)
						SDL_RenderDrawThiccLine(renderer, dest.x +10, dest.y - 10, dest.x +10 + dX*20, dest.y - 10 + dY*20);
					if(dY==-1)
						SDL_RenderDrawThiccLine(renderer, dest.x + 10, dest.y +30, dest.x + 10 + dX*20, dest.y + 30 + dY*20);

				}
			}
			if(bestPos[0].x >= 0 && grille[bestPos[0].x][bestPos[0].y] == CAISSE){
				int dX = (bestPath[0] == 1 ? -1: (bestPath[0] == 3 ?1:0));
				int dY = (bestPath[0] == 0 ? 1: (bestPath[0] == 2 ?-1:0));
				dest.w = 20; dest.h=20; dest.x = 20*(bestPos[0].x+dX); dest.y = 20*(bestPos[0].y+dY);
				src.w = 40; src.h =40; src.x = UNKNOWN * 40;
				SDL_RenderCopy(renderer, texture, &src, &dest);
				SDL_SetRenderDrawColor(renderer, 0x56,0x5d,0x36, 255);
				if(dX==1)
					SDL_RenderDrawLine(renderer, dest.x - 10, dest.y + 10, dest.x - 10 + dX*20, dest.y + 10 + dY*20);
				if(dX==-1)
					SDL_RenderDrawLine(renderer, dest.x + 30, dest.y + 10, dest.x + 30 + dX*20, dest.y + 10 + dY*20);

				if(dY==1)
					SDL_RenderDrawLine(renderer, dest.x +10, dest.y - 10, dest.x +10 + dX*20, dest.y - 10 + dY*20);
				if(dY==-1)
					SDL_RenderDrawLine(renderer, dest.x + 10, dest.y +30, dest.x + 10 + dX*20, dest.y + 30 + dY*20);

			}



			//boxpath
			for(int iTraj=1; iTraj<MAX_PATHS && bestPathBox[iTraj]!=-1; iTraj++){
				if(drawBoxPath){
					SDL_SetRenderDrawColor(renderer, 0,255,255,255);
					if(bestPosBox[0].x < 0){
						//printf("first tp\n");
						for(int i=0; i<NB_CASES; i++){
							if(grille[xPump][i] == TELEPORT && (bestPathBox[0]!=1 && bestPathBox[0]!=3)){
								SDL_RenderDrawLine(renderer, xPump * 20 +10, yPump * 20+10, xPump *20+10, i *20+10);
								break;
							}
							else if(grille[i][yPump] == TELEPORT && (bestPathBox[0]!=0 && bestPathBox[0]!=2)){
								SDL_RenderDrawLine(renderer, xPump * 20 +10, yPump * 20+10, i *20+10, yPump *20+10);
								break;
							}
						}
						for(int j=0;j<NB_CASES ;j++){
							for(int i=0; i<NB_CASES ; i++){
								if(grille[i][j] == TELEPORT && (j!=yPump || (bestPathBox[0]!=1 && bestPathBox[0]!=3)) && (i!=xPump || (bestPathBox[0]!=0 && bestPathBox[0]!=2))){
									//printf("tp out %d %d\n", i, j );
									SDL_RenderDrawLine(renderer, i * 20 +10, j * 20+10, (bestPosBox[0].x+100) *20+10, (bestPosBox[0].y+100) *20+10);
								}
							}
						}
					}else{
						SDL_RenderDrawLine(renderer, xPump * 20 +10, yPump * 20+10, bestPosBox[0].x *20+10, bestPosBox[0].y *20+10);
					}

					int r=255,g=0, b=0;
					int sR=-1, sG = 0, sB=1;
					for(int iTraj=1; iTraj<MAX_PATHS && bestPathBox[iTraj]!=-1; iTraj++){
						r+=sR*30;
						if(r<0 || r>255){
							if(r<0)
								r=0;
							else
								r=255;
							sR*=-1;
							if(sG==0)
								sG=1;
						}
						g += sG *30;
						if(g<0 || g>255){
							if(g<0)
								g=0;
							else
								g=255;
							sG*=-1;
						}
						b += sB *30;
						if(b<0 || b>255){
							if(b<0)
								b=0;
							else
								b=255;
							sB*=-1;
						}
						printf("%d %d (path : %s) : ",iTraj, bestPosBox[iTraj].x, DIRECTION[bestPathBox[iTraj]]);
						SDL_SetRenderDrawColor(renderer, 255-r,255-g,255-b,255);
						if(bestPosBox[iTraj].x < 0){
							//printf("aie %d %d \n", bestPosBox[iTraj].x, bestPosBox[iTraj].y);
							SDL_Point realPosM = {(bestPosBox[iTraj-1].x<0?bestPosBox[iTraj-1].x+100:bestPosBox[iTraj-1].x), (bestPosBox[iTraj-1].y<0?bestPosBox[iTraj-1].y+100:bestPosBox[iTraj-1].y)};
							//printf("real %d %d\n", realPosM.x, realPosM.y);
							//i-1 vers tp
							for(int k=0; k<NB_CASES; k++){
								if(grille[realPosM.x][k] == TELEPORT)
									SDL_RenderDrawLine(renderer, realPosM.x * 20 +10, realPosM.y * 20+10, realPosM.x *20+10, k *20+10);
								else if(grille[k][realPosM.y] == TELEPORT)
									SDL_RenderDrawLine(renderer, realPosM.x * 20 +10, realPosM.y * 20+10, k *20+10, realPosM.y *20+10);
							}
							//tp vers i
							for(int j=0;j<NB_CASES ;j++){
								for(int k=0; k<NB_CASES ; k++){
									if(grille[k][j] == TELEPORT && (j!= realPosM.y || (bestPathBox[iTraj]!=1 && bestPathBox[iTraj]!=3)) && (k!= realPosM.x || (bestPathBox[iTraj]!=0 && bestPathBox[iTraj]!=2))){
										printf("tp vers i : %d %d %d %d ", k * 20 +10, j * 20+10, (bestPosBox[iTraj].x+100) *20+10, (bestPosBox[iTraj].y+100) *20+10);
										SDL_RenderDrawLine(renderer, k * 20 +10, j * 20+10, (bestPosBox[iTraj].x+100) *20+10, (bestPosBox[iTraj].y+100) *20+10);
									}
								}
							}
							/*for(int j=0;j<NB_CASES ;j++){
								for(int i=0; i<NB_CASES ; i++){
									if(grille[i][j] == TELEPORT && j!=realPosM.y && i!=realPosM.x){
										printf(" idk :%d %d %d %d", i * 20 +10, j * 20+10, (bestPosBox[iTraj].x+100) *20+10, (bestPosBox[iTraj].y+100) *20+10);
										SDL_RenderDrawLine(renderer, i * 20 +10, j * 20+10, (bestPosBox[iTraj].x+100) *20+10, (bestPosBox[iTraj].y+100) *20+10);
									}
								}
							}*/
						}
						else{
							SDL_RenderDrawLine(renderer, (bestPosBox[iTraj-1].x<0?bestPosBox[iTraj-1].x+100:bestPosBox[iTraj-1].x) * 20+10, (bestPosBox[iTraj-1].y<0?bestPosBox[iTraj-1].y+100:bestPosBox[iTraj-1].y) * 20+10, bestPosBox[iTraj].x *20+10, bestPosBox[iTraj].y *20+10);
						}
						//printf("%s (%d %d)- ",DIRECTION[bestPathBox[i]], bestPosBox[i].x, bestPosBox[i].y);
						printf("\n");
						if(bestPosBox[iTraj].x >= 0 && grille[bestPosBox[iTraj].x][bestPosBox[iTraj].y] == CAISSE){
							int dX = (bestPathBox[iTraj] == 1 ? -1: (bestPathBox[iTraj] == 3 ?1:0));
							int dY = (bestPathBox[iTraj] == 0 ? 1: (bestPathBox[iTraj] == 2 ?-1:0));

							dest.w = 20; dest.h=20; dest.x = 20*(bestPosBox[iTraj].x+dX); dest.y = 20*(bestPosBox[iTraj].y+dY);
							src.w = 40; src.h =40; src.x = UNKNOWN * 40;
							SDL_RenderCopy(renderer, texture, &src, &dest);
							SDL_SetRenderDrawColor(renderer, 0x56,0x5d,0x36, 255);
							if(dX==1)
								SDL_RenderDrawLine(renderer, dest.x - 10, dest.y + 10, dest.x - 10 + dX*20, dest.y + 10 + dY*20);
							if(dX==-1)
								SDL_RenderDrawLine(renderer, dest.x + 30, dest.y + 10, dest.x + 30 + dX*20, dest.y + 10 + dY*20);

							if(dY==1)
								SDL_RenderDrawLine(renderer, dest.x +10, dest.y - 10, dest.x +10 + dX*20, dest.y - 10 + dY*20);
							if(dY==-1)
								SDL_RenderDrawLine(renderer, dest.x + 10, dest.y +30, dest.x + 10 + dX*20, dest.y + 30 + dY*20);

						}


					}
					if(bestPosBox[0].x >= 0 && grille[bestPosBox[0].x][bestPosBox[0].y] == CAISSE){
						int dX = (bestPathBox[0] == 1 ? -1: (bestPathBox[0] == 3 ?1:0));
						int dY = (bestPathBox[0] == 0 ? 1: (bestPathBox[0] == 2 ?-1:0));
						dest.w = 20; dest.h=20; dest.x = 20*(bestPosBox[0].x+dX); dest.y = 20*(bestPosBox[0].y+dY);
						src.w = 40; src.h =40; src.x = UNKNOWN * 40;
						SDL_RenderCopy(renderer, texture, &src, &dest);
						SDL_SetRenderDrawColor(renderer, 0x56,0x5d,0x36, 255);
						if(dX==1)
							SDL_RenderDrawLine(renderer, dest.x - 10, dest.y + 10, dest.x - 10 + dX*20, dest.y + 10 + dY*20);
						if(dX==-1)
							SDL_RenderDrawLine(renderer, dest.x + 30, dest.y + 10, dest.x + 30 + dX*20, dest.y + 10 + dY*20);

						if(dY==1)
							SDL_RenderDrawLine(renderer, dest.x +10, dest.y - 10, dest.x +10 + dX*20, dest.y - 10 + dY*20);
						if(dY==-1)
							SDL_RenderDrawLine(renderer, dest.x + 10, dest.y +30, dest.x + 10 + dX*20, dest.y + 30 + dY*20);

					}
				}
			}

		}



		SDL_RenderPresent(renderer);

		currentTime = SDL_GetTicks();
		//printf("%ld\n", currentTime - lastTime);
		while( currentTime - lastTime < 1000./30 )
			currentTime = SDL_GetTicks();
		//printf("%ld\n", currentTime - lastTime);
		lastTime = currentTime;




		SDL_RenderClear(renderer);
		if(findSolution){
			drawBoxPath = SDL_FALSE;
			int minLenght = MAX_PATHS+2;
			int minLenghtBox = MAX_PATHS+2;
			int canUseBox = SDL_FALSE;
			findPath(grille, path, bestPath, pos, bestPos, &minLenght, canUseBox);
			canUseBox = SDL_TRUE;
			if(minLenght == MAX_PATHS+2){
				findPath(grille, path, bestPath, pos, bestPos, &minLenght, canUseBox);
			}
			else{
				findPath(grille, path, bestPathBox, pos, bestPosBox, &minLenghtBox, canUseBox);
			}

			if(minLenghtBox < minLenght)
				drawBoxPath = SDL_TRUE;

			if(minLenght < MAX_PATHS+2){
				grilleValide = SDL_TRUE;

				for(int i=0; i<MAX_PATHS && bestPath[i]!=-1; i++){
					printf("%s - ",DIRECTION[bestPath[i]]);
				}
				printf("FINI !\n");
			}
			else{
				printf("NO WAY\n");
				grilleValide = SDL_FALSE;
			}

		}
	}
    return 0;
}
