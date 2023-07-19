#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)
		
// Wave Function Collapse

// 0 Empty
// 1 Trees
// 2 Bushes
// 3 Grass
// 4 Sand
// 5 Lake
// 6 Water
// 7 Stone
#define mapSizeX 1024
#define mapSizeY 512
#define biomeSize 16
#define landmassSize 32
#define tectonicPlates (mapSizeX+mapSizeY)/(landmassSize*8)
    
// Window dimensions
static const int width = mapSizeX;
static const int height = mapSizeY;

int map[mapSizeX][mapSizeY] = {0};
int biomeMap[mapSizeX/biomeSize][mapSizeY/biomeSize];
int landmassMap[mapSizeX/landmassSize][mapSizeY/landmassSize];
char textMode = 0;

// enums
typedef enum {
	emptylandmass,
	ocean,
	islands,
	archipelago,
	continent,
	mainland,
	numlandmass,
} landmass;

typedef enum {
	emptybiome,
	desert,
	beach,
	grasslands,
	forest,
	mountains,
	numBiomes,
} biome;

typedef enum {
	emptyTile,
	treeTile,
	bushTile,
	grassTile,
	oceanTile,
	sandTile,
	riverTile,
	stoneTile,
	errorTile,
	numTiles,
} tilename;

// Get a random Tile ID
int getRandomTileID() {
	return rand() % numTiles;
}

// Get a random Biome ID
int getRandomBiomeID() {
	return rand() % numBiomes+1;
}

// Get a random landmass ID
int getRandomlandmassID() {
	return rand() % numlandmass;
}

//" "Empty can be anything 
// T Trees can be next to bushes and Trees
// m Bushes can be next to grass and Trees
// , Grass can be next to bushes and Grass
// # Sand can be next to Grass
// ~ Water can be next to Sand and Water

int x,y = 0;
int i = 0;
int id = emptyTile;
int surrounding[4] = {emptyTile,emptyTile,emptyTile,emptyTile};
int invalidTile = emptyTile;
int previousRandomValues[20] = { 0 };
SDL_Renderer *renderer;

int updateProgressBar(int progress, int mode) {
	// Draw outlined
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_Rect outlineRect = {mapSizeX/2-53,mapSizeY/2-13,106,26};
	SDL_RenderFillRect(renderer, &outlineRect);
	// Draw bg
	SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
	SDL_Rect bgRect = {mapSizeX/2-50,mapSizeY/2-10,100,20};
	SDL_RenderFillRect(renderer, &bgRect);
	// Get color
	switch(mode) {
		case 0:
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			break;
		case 1:
			SDL_SetRenderDrawColor(renderer, 255, 192, 0, 255);
			break;
		case 2:
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
			break;
		default:
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
			break;
	}
	// Render Progress
	SDL_Rect barRect = {mapSizeX/2-50,mapSizeY/2-10,progress,20};
	SDL_RenderFillRect(renderer, &barRect);
	SDL_RenderPresent(renderer);
	return 0;
}

// Render a tile
int printTile(int x, int y) {
	if (textMode) {
		switch(map[x][y]) {
			case emptyTile: // Empty
				printf("\x1b[0m");
				printf(" ");
				break;
			case treeTile: // Tree
				printf("\x1b[92;42m");
				printf("T");
				break;
			case bushTile: // Bush
				printf("\x1b[32;102m");
				printf("m");
				break;
			case grassTile: // Grass
				printf("\x1b[32;102m");
				printf(",");
				break;
			case sandTile: // Sand
				printf("\x1b[33;103m");
				printf("#");
				break;
			case riverTile: // River Water
				printf("\x1b[37;46m");
				printf("~");
				break;
			case oceanTile: // Ocean Water
				printf("\x1b[37;104m");
				printf("~");
				break;
			case stoneTile: // Stone
				printf("\x1b[30;100m");
				printf("^");
				break;
			case errorTile: // Error
				printf("\x1b[35;40m");
				printf("#");
				break;
		};
		printf("\x1b[0m");
	} else {
		switch(map[x][y]) {
			case emptyTile: // Empty
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				break;
			case treeTile: // Tree
				SDL_SetRenderDrawColor(renderer, 17,95,66, 255);
				break;
			case bushTile: // Bush
				SDL_SetRenderDrawColor(renderer, 94,187,32, 255);
				break;
			case grassTile: // Grass
				SDL_SetRenderDrawColor(renderer, 182,236,101, 255);
				break;
			case sandTile: // Sand
				SDL_SetRenderDrawColor(renderer, 250, 222, 168, 255);
				break;
			case riverTile: // River Water
				SDL_SetRenderDrawColor(renderer, 160, 199, 244, 255);
				break;
			case oceanTile: // Ocean Water
				SDL_SetRenderDrawColor(renderer, 47, 112, 196, 255);
				break;
			case stoneTile: // Stone
				SDL_SetRenderDrawColor(renderer, 114, 121, 130, 255);
				break;
			default: // Error
				SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
				break;
		};
        SDL_RenderDrawPoint(renderer, x, y);
	}
	return 0;
}

// Render the map
int printMap() {
	// Clear screen
	if (textMode) {
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				printTile(x,y);
				printTile(x,y);
			}
			printf("\n");
		}
		printf("\x1b[0m");
	} else {
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				printTile(x,y);
			}
		}
	}
}

// Place a tile on the map
void placeTile(int x, int y, int tile) {
	x = abs(x%mapSizeX);
	y = abs(y%mapSizeY);
	map[x][y] = tile;
}

// Calculate Distance between two points as integer
int getIntDistance(int x1, int y1, int x2, int y2) {
	return (int) sqrt( (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) );  
}

// Random with a maximum
// Max included
int getRandomLimited(int max) {
	max = max+1;
	return rand() % max;
}

// Random with a minimum and maximum
int getRandomLimitedMinMax(int min, int max) {
	max = max+1;
	int result = (rand() % max);
	
	if (result < min) {	result += min; }
	return result;
}

// Smoothed Random Generator with a maximum value
// Used to make random values less... random?
float getSmoothedRandomLimited(int max, int smoothSteps) {
	float smoothedValue = 0.0f;
	for (int i = 1; i < 8; i++) {
		//printf("%d -> %d\n",previousRandomValues[i],previousRandomValues[i+1]);
		previousRandomValues[i] = previousRandomValues[i+1];
	}
	previousRandomValues[8] = rand() % max;
	
	for (int i = 8; i > 8-smoothSteps; i--) {
		smoothedValue+=(float)previousRandomValues[i];
	}
	//printf("%f\n",smoothedValue/(float)smoothSteps);
	
	return smoothedValue/(float)smoothSteps;
}

// Checks if another tile is surrounding the current tile at any other side
int isNeighboring(int tile) {
	if ((surrounding[0] == tile) || (surrounding[1] == tile) || (surrounding[2] == tile) || (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}
}

// Checks if another tile is surrounding the current tile at any other side
int canNeighbor(int tile) {
	if ((surrounding[0] == tile) || (surrounding[1] == tile) || (surrounding[2] == tile) || (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}
}

// Checks to see if a tile is surrounded by another type of tile
int surroundedBy(int tile) {
	if ((surrounding[0] == tile) && (surrounding[1] == tile) && (surrounding[2] == tile) && (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}
}

// The random island
// aka the algorithm I used to shape islands previously
// leaving it here in case I ever find a use for it!
void randomIsland(int x1, int y1, int islandSize) {
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			int x2 = x;
			int y2 = y;
			int distanceToIslandCenter = getIntDistance(x1,y1,x2,y2);
			// Generate Water Circle
			if (distanceToIslandCenter < islandSize*(getRandomLimited(5))) {
				map[x2][y2] = emptyTile;
			}
		}
	}
}

// Line drawing
// Just using Bresenham's line algorithm
void plotLineHigh(int x0, int y0, int x1, int y1, int tile) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;

    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int D = (2 * dx) - dy;
    int x = x0;

    for (int y = y0; y <= y1; y++) {
        placeTile(x,y,tile);

        if (D > 0) {
            x = x + xi;
            D = D + (2 * (dx - dy));
        } else {
            D = D + (2 * dx);
        }
    }
}


void plotLineLow(int x0, int y0, int x1, int y1, int tile) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;

    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }

    int D = (2 * dy) - dx;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        placeTile(x,y,tile);

        if (D > 0) {
            y = y + yi;
            D = D + (2 * (dy - dx));
        } else {
            D = D + (2 * dy);
        }
    }
}


void plotLine(int x0, int y0, int x1, int y1, int tile) {
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            plotLineLow(x1, y1, x0, y0, tile);
        } else {
            plotLineLow(x0, y0, x1, y1, tile);
		}
    } else {
        if (y0 > y1) {
            plotLineHigh(x1, y1, x0, y0, tile);
		} else {
            plotLineHigh(x0, y0, x1, y1, tile);
		}
	}
}
/*
 The Polygon island!
 These create points around a circle,
 then randomly push that out to create islands of various sizes
*/
void polygonIsland(int x1, int y1, int islandSize) {
	// Set number of points along circle
	int numberOfPoints = 20;
	float polygonX[20] = {0};
	float polygonY[20] = {0};

	float angleIncrement = 2 * M_PI / numberOfPoints;  // Calculate the angle increment
	float currentAngle = (float)getRandomLimited(360)/360.0f;  // Starting angle

	// Set random displacements of points
	for (int pointID = 0; pointID < numberOfPoints; pointID++) {
		// Interpolate continent outline between points
		float x2 = cos(currentAngle) * (float)islandSize * getSmoothedRandomLimited(5, 10) + (float)x1;
		float y2 = sin(currentAngle) * (float)islandSize * getSmoothedRandomLimited(5, 10) + (float)y1;
		polygonX[pointID] = x2;
		polygonY[pointID] = y2;
		
		currentAngle += angleIncrement;  // Increment the angle for the next point
	}
	
	// Draw Polygon on map
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < numberOfPoints - 1; ++i) {
        SDL_RenderDrawLine(renderer, polygonX[i], polygonY[i], polygonX[i + 1], polygonY[i + 1]);
    }
	
	// Fill out tiles inside polygon
	for (int mapY = -mapSizeY; mapY < mapSizeY+mapSizeY; mapY++) {
		for (int mapX = -mapSizeX; mapX < mapSizeX+mapSizeX; mapX++) {
			int i, j;
			float pointX = (float)mapX;
			float pointY = (float)mapY;
			char isInside = 0;

			for (i = 0, j = numberOfPoints - 1; i < numberOfPoints; j = i++) {
				if (((polygonY[i] >= pointY && polygonY[j] < pointY) || (polygonY[j] >= pointY && polygonY[i] < pointY)) &&
					(pointX < (polygonX[j] - polygonX[i]) * (pointY - polygonY[i]) / (polygonY[j] - polygonY[i]) + polygonX[i])) {
					isInside = !isInside;
				}
			}

			if (isInside) {
				placeTile(mapX, mapY, emptyTile);
			}
		}
	}
	
	// Generate rivers riverTile
	// Figure out how to draw a nice line
	// FIX RIVERS!!!
	/*
	int numberOfRivers = getRandomLimited(islandSize/2);
	int numberOfRiverBends = 10;
	float lineX[10];
	float lineY[10];
	for (int i = 0; i < numberOfRivers; i++) {
		// Choose two nodes
		int pointA = getRandomLimited(numberOfPoints);
		int pointB = getRandomLimited(numberOfPoints);
		// Get their positions
		float x0 = (float)polygonX[pointA];
		float y0 = (float)polygonY[pointA];
		float x1 = (float)polygonX[pointB];
		float y1 = (float)polygonY[pointB];
		// Calculate the size of each step
		float stepX = (x1-x0)/numberOfRiverBends;
		float stepY = (y1-y0)/numberOfRiverBends;
		// Store the resulting line and randomize it's positions slightly
		float prevStepX = x0;
		float prevStepY = y0;
		for (int i = 0; i < numberOfRiverBends; i++) {
			prevStepX = x0+stepX*i;
			prevStepY = y0+stepY*i;
			plotLine(
				prevStepX,
				prevStepY,
				x0+stepX*(i+1)+(getRandomLimited(3)),
				y0+stepY*(i+1)+(getRandomLimited(3)),
				riverTile);
		}
	}*/
}

// Checks the current Biome Square for a certain tile, usually oceanwater
// Used to prevent deserts from spawning near water
int checkBiomeForTile(int x, int y,int tile) {
	x = abs(x%mapSizeX);
	y = abs(y%mapSizeY);
	x = x*biomeSize;
	y = y*biomeSize;
	int numberOfTiles = 0;
	for (int tileY = y; tileY < y+biomeSize; tileY++) {
		for (int tileX = x; tileX < x+biomeSize; tileX++) {
			if (map[tileX][tileY]==tile) {
				numberOfTiles++;
			}
		}
	}
	return numberOfTiles;
}

// Checks if it's neighboring a biome
int isNeighboringBiome(int x, int y, int biome) {
	x = abs(x%(mapSizeX/biomeSize));
	y = abs(y%(mapSizeY/biomeSize));
	int neighboringOnSides;
	if (biomeMap[x+1][y] == biome) {
		neighboringOnSides++;
	}
	if (biomeMap[x][y+1] == biome) {
		neighboringOnSides++;
	}
	if (biomeMap[x-1][y] == biome) {
		neighboringOnSides++;
	}
	if (biomeMap[x][y-1] == biome) {
		neighboringOnSides++;
	}
	return neighboringOnSides;
}

// Check which landmass the tile belongs to
int checklandmass(int x, int y) {
	return landmassMap[x/landmassSize][y/landmassSize];
}

int getNeighboringBiomes(int x, int y, int direction) {
	switch(direction) {
		case 0:
		return biomeMap[x][y-1]; // North
		case 1:
		return biomeMap[x+1][y]; // East
		case 2:
		return biomeMap[x][y+1]; // South
		case 3:
		return biomeMap[x-1][y]; // West
		default:
		printf("Invalid Direction\n");
	}
}

int renderBiome(int biomeX,int biomeY) {
	switch(biomeMap[biomeX][biomeY]) {
		case desert:
			SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);	
			break;
		case beach:
			SDL_SetRenderDrawColor(renderer, 192, 255, 0, 128);	
			break;
		case grasslands:
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128);	
			break;
		case forest:
			SDL_SetRenderDrawColor(renderer, 0, 128, 0, 128);	
			break;
		case mountains:
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);	
			break;
		default:
			SDL_SetRenderDrawColor(renderer, 255, 0, 255, 128);	
			break;
	}
	SDL_Rect rect = {biomeX*biomeSize,biomeY*biomeSize,biomeSize,biomeSize};
	SDL_RenderFillRect(renderer, &rect);
	//SDL_RenderPresent(renderer);
	return 0;
}

int scaleToValue(int value, int maxValue, int desiredMax) {
	float goal = (float)desiredMax;
	float max = (float)maxValue;
	float scaler = 1.0f;
	if (max < 100) {
		scaler = goal/max;
	} else {
		scaler = max/goal;
	}
	return (int)((float)value*scaler);
}

int WinMain(int argc, char **argv) {
	// SDL2 Prep
	// Seed the map
	int initialSeed = 0;//time(NULL);
	srand(initialSeed);
	float renderScale = 1.0f;
	
	// Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("Hello, SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width*renderScale, height*renderScale, SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
	SDL_RenderSetScale(renderer, renderScale, renderScale);
    CHECK_ERROR(renderer == NULL, SDL_GetError());

    // Initial renderer color
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);	
	SDL_RenderClear(renderer);
	
	// Pregen
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			placeTile(x,y,oceanTile);
		}
	}
	
	for (int i = 0; i < 20; i++) {
		previousRandomValues[i] = getRandomLimited(5);
	}
	
	if (textMode) {
		printf("\x1b[2J"); // Clear Screen
		printf("\x1b[H"); // Set Cursor to Home
	}
	
	// Generation
	
	// Tectonic plates generation
	// To generate more believable landmasses & mountain ranges
	int tectonicPlatesOriginX[tectonicPlates]  = {0.0f};
	int tectonicPlatesOriginY[tectonicPlates]  = {0.0f};
	for (int i = 0; i < tectonicPlates; i++) {
		tectonicPlatesOriginX[i] = (int)getRandomLimited(mapSizeX);
		tectonicPlatesOriginY[i] = (int)getRandomLimitedMinMax(mapSizeY/3,mapSizeY/3*2);
	}
	
	
	int numberOfIslands = 10;
	int islandBaseSize = 3;
	int maxRandomModifer = 16;
	int halfLandmassSize = landmassSize/2;
	// landmass
	for (int landmassY = 0; landmassY < mapSizeY/landmassSize; landmassY++) {
		updateProgressBar(scaleToValue(landmassY,mapSizeY/landmassSize,100)+1,0);
		for (int landmassX = 0; landmassX < mapSizeX/landmassSize; landmassX++) {
			// this is for a single landmass "chunk"
			for (int y = landmassY*landmassSize; y < landmassY*landmassSize+landmassSize; y++) {
				for (int x = landmassX*landmassSize; x < landmassX*landmassSize+landmassSize; x++) {
					int closest = mapSizeX;
					int closestPoint = 0;
					for (int k = 0; k < tectonicPlates; k++) {
						int value = getIntDistance(x,y,tectonicPlatesOriginX[k],tectonicPlatesOriginY[k]);
						if (value < closest) {
							closestPoint = i;
							closest = value;
						}
					}
					if (closest < 255) {
						int r = (32*closest/255);
						int g = (128*closest/255);
						SDL_SetRenderDrawColor(renderer, r, g, closest, 255);
					} else {
						SDL_SetRenderDrawColor(renderer, 32, 128, 255, 255);
					}
					SDL_RenderDrawPoint(renderer, x, y);
				}
			}
			//SDL_RenderPresent(renderer);
			
			// Initial random landmass
			int landmass = 0;
			if (getRandomLimited(100)>95) {
				landmass = getRandomLimitedMinMax(ocean,archipelago);
			}
			int closest = mapSizeX;
			int closestPoint = 0;
			// Then concrete ones based on tectonic plates
			for (int i = 0; i < tectonicPlates; i++) {
				int value = getIntDistance(landmassX*landmassSize,landmassY*landmassSize,tectonicPlatesOriginX[i],tectonicPlatesOriginY[i]);
				if (value < closest) {
					closestPoint = i;
					closest = value;
				}
			}
			if (closest < 20+getRandomLimited(10)) {
				landmass = mainland;
				SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
				SDL_SetRenderDrawColor(renderer, 64, 32, 32, 128);
				SDL_RenderFillRect(renderer, &rect);
			} else if (closest < 60+getRandomLimited(20)) {
				landmass = continent;
				SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
				SDL_SetRenderDrawColor(renderer, 128, 32, 32, 128);
				SDL_RenderFillRect(renderer, &rect);
			} else if (closest < 80) {
				if (getRandomLimited(20)>15) {
					landmass = continent;
				}
				SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
				SDL_SetRenderDrawColor(renderer, 192, 32, 32, 128);
				SDL_RenderFillRect(renderer, &rect);
			}
			if (landmass == emptylandmass) {
				landmass = ocean;
			}
			landmassMap[landmassX][landmassY] = landmass;
		}
	}
	
	// Generate Islands
	for (int landmassY = 0; landmassY < mapSizeY/landmassSize; landmassY++) {
		updateProgressBar(scaleToValue(landmassY,mapSizeY/landmassSize,100)+1,1);
		for (int landmassX = 0; landmassX < mapSizeX/landmassSize; landmassX++) {
			// USE THIS TO MAKE CHUNKING POSSIBLE!!!!
			srand( initialSeed + (landmassX * mapSizeX + landmassY));
			switch(landmassMap[landmassX][landmassY]) {
				case mainland:
					maxRandomModifer = 6;
					islandBaseSize = 10;
					numberOfIslands = 1;
					SDL_SetRenderDrawColor(renderer, 192, 192, 192, 128);	
					printf("^");
					break;
				case continent:
					maxRandomModifer = 10;
					islandBaseSize = 12;
					numberOfIslands = 2;
					SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);	
					printf("#");
					break;
				case archipelago:
					maxRandomModifer = 5;
					islandBaseSize = 2;
					numberOfIslands = 2;
					SDL_SetRenderDrawColor(renderer, 64, 64, 64, 128);	
					printf("o");
					break;
				case islands:
					maxRandomModifer = 3;
					islandBaseSize = 1;
					numberOfIslands = 3;
					SDL_SetRenderDrawColor(renderer, 32, 32, 32, 128);	
					printf(":");
					break;
				default:
				case ocean:
					maxRandomModifer = 0;
					islandBaseSize = 0;
					numberOfIslands = 0;
					SDL_SetRenderDrawColor(renderer, 16, 16, 16, 64);	
					printf("~");
					break;
			}
			SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
			SDL_RenderFillRect(renderer, &rect);
			
				// Generate islands
			for (int i = 1; i <= numberOfIslands; i++) {
				int islandSize = islandBaseSize+getRandomLimited(maxRandomModifer);
				int x1 = landmassX*landmassSize+(int)getRandomLimited(landmassSize);//(mapSizeX/(1+getRandomLimited(16)));
				int y1 = landmassY*landmassSize+(int)getRandomLimited(landmassSize);//(mapSizeY/(1+getRandomLimited(16)));
				
				//randomIsland(x1,y1,islandSize);
				polygonIsland(x1,y1,islandSize);
			}
		}
		printf("\n");
	}
	
	// Biomes based on landmass
	for (int biomeY = 0; biomeY < mapSizeY/biomeSize; biomeY++) {
		updateProgressBar(scaleToValue(biomeY+1,mapSizeY/biomeSize,100),2);
		for (int biomeX = 0; biomeX < mapSizeX/biomeSize; biomeX++) {
			int biome = 0;
			//printf("%d,", checklandmass(biomeX*biomeSize,biomeY*biomeSize));
			switch(checklandmass(biomeX*biomeSize,biomeY*biomeSize)) {
				case mainland:
					biome = mountains;
					if (getRandomLimited(10)>7) {
						biome = mountains;
					} else {
						biome = forest;
					}
					break;
				case continent:
					if (getRandomLimited(10)>9) {
						biome = mountains;
						break;
					}
				default:
					biome = getRandomBiomeID();
					switch(biome) {
						case grasslands:
							if ((biomeY > ((mapSizeY/biomeSize)/5)*2) && (biomeY < ((mapSizeY/biomeSize)/5)*3)) {
								biome = desert;
							} else {
								biome = forest;
							}
							break;
						case beach:
							if (!checkBiomeForTile(biomeX,biomeY,oceanTile)) {
								biome = forest;
							}
							break;
						case mountains:
							if (checkBiomeForTile(biomeX,biomeY,oceanTile)) {
								biome = grasslands;
							}
							break;
						case desert:
							if (checkBiomeForTile(biomeX,biomeY,oceanTile)) {
								// Jungle?
								//biome = beach;
							//} else if (isNeighboringBiome(biomeX,biomeY,forest)) {
								biome = grasslands;
							}
							break;
						case forest:
							if (checkBiomeForTile(biomeX,biomeY,oceanTile)) {
								biome = grasslands;
							}
							break;
					}
					break;
			}
			biomeMap[biomeX][biomeY] = biome;
			renderBiome(biomeX,biomeY);
		}
		//printf("\n");
	}
	
	// Render final map
	for (int mapY = 0; mapY < mapSizeY; mapY++) {
		for (int mapX = 0; mapX < mapSizeX; mapX++) {
			int randomTile;
			if (map[mapX][mapY] == emptyTile) {
				// Get surrounding tiles
				if (mapX-1<0) {
					surrounding[0] = emptyTile;
				} else {
					surrounding[0] = map[mapX-1][mapY];
				}
				if (mapX+1 > mapSizeX) {
					surrounding[1] = emptyTile;
				} else {
					surrounding[1] = map[mapX+1][mapY];
				}
				if (mapY-1 < 0) {
					surrounding[2] = emptyTile;
				} else {
					surrounding[2] = map[mapX][mapY-1];
				}
				if (mapY+1 > mapSizeY) {
					surrounding[3] = emptyTile;
				} else {
					surrounding[3] = map[mapX][mapY+1];
				}
				
				// Actually put down tiles based on biomes
				int biome = biomeMap[mapX/biomeSize][mapY/biomeSize];
				// Find most common adjacent Biome
				int closestDistance = mapSizeX*mapSizeY;
				int closestBiome = biome;
				int biomeXpos = mapX/biomeSize;
				int biomeYpos = mapY/biomeSize;	
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 32);
				// Biome Blending Priority?
				if ((biome == mountains) || (biome == forest)) {
					for (int yBiome = -1; yBiome <= 1; yBiome++) {
						for (int xBiome = -1; xBiome <= 1; xBiome++) {
							if ((xBiome == 0) && (yBiome == 0)) {
								// Own Biome
							} else {
								/*SDL_RenderDrawLine(
									renderer,
									mapX,
									mapY,
									((biomeXpos+xBiome)*biomeSize)+biomeSize/2,
									((biomeYpos+yBiome)*biomeSize)+biomeSize/2
								);*/
								// Get distance to next biome
								int distance = getIntDistance(
									mapX,
									mapY,
									((biomeXpos+xBiome)*biomeSize)+biomeSize/2,
									((biomeYpos+yBiome)*biomeSize)+biomeSize/2
								);
								// Add non-linearity
								if (distance < closestDistance) {
									closestDistance = distance;
									closestBiome = biomeMap[biomeXpos-(xBiome*-1)][biomeYpos-(yBiome*-1)];
								}
								//printf("%d:%d \n", biomeMap[biomeXpos-xBiome][biomeYpos-yBiome], distance);
							}
						}
						//printf("\n");
					}
					//printf("Closest Biome: %d\n", closestBiome);
					if (getIntDistance(mapX,mapY,((biomeXpos)*biomeSize)+biomeSize/2,((biomeYpos)*biomeSize)+biomeSize/2) > 4) {
						if ((closestBiome == mountains) && (biome == forest)) {
							closestBiome = forest;
						}
						biome = closestBiome;
					}	
				}			
				
				// Randomize a bit
				if (getRandomLimited(100)>98) {
					int neighborBiomes[4];
					for (int i = 0; i < 4; i++) {
						neighborBiomes[i] = getNeighboringBiomes(mapX/biomeSize,mapY/biomeSize,i);
					}
					biome = neighborBiomes[getRandomLimited(3)];
				}
				
				// Place tiles based on Biome
				switch(biome) {
					case beach:
						randomTile = getRandomLimitedMinMax(oceanTile,sandTile);
						placeTile(mapX,mapY,randomTile);
						break;
					case desert:
						if (isNeighboring(emptyTile) || isNeighboring(sandTile)) {
							placeTile(mapX,mapY,sandTile);
						} else {
							placeTile(mapX,mapY,grassTile);
						}
						break;
					case grasslands:
						if (isNeighboring(oceanTile)) {
							randomTile = sandTile;
						} else {
							randomTile = getRandomLimitedMinMax(bushTile,grassTile);
						}
						placeTile(mapX,mapY,randomTile);
						break;
					case forest:
						if (isNeighboring(oceanTile)) {
							randomTile = sandTile;
						} else if (isNeighboring(bushTile)) {
							randomTile = treeTile;
						} else {
							randomTile = getRandomLimitedMinMax(treeTile,bushTile);
						}
						placeTile(mapX,mapY,randomTile);
						break;
					case mountains:
						if (isNeighboring(stoneTile) || isNeighboring(grassTile)) {
							randomTile = stoneTile;
						} else {
							randomTile = grassTile;
						}
						placeTile(mapX,mapY,randomTile);
						break;
					default:
						placeTile(mapX,mapY,grasslands);
						break;
				}
			}
		}
		//SDL_RenderPresent(renderer);
	}
	
	// Print Map
    char running = 1;

    // Clear screen
	printMap();
	
    SDL_Event event;
    while(running) {
        // Process events
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = 0;
            } else if(event.type == SDL_KEYDOWN) {
                const char *key = SDL_GetKeyName(event.key.keysym.sym);
                if(strcmp(key, "Q") == 0) {
                    running = 0;
                }                    
            }
        }

        // Draw

        // Show what was drawn
        SDL_RenderPresent(renderer);
    }
	return 0;
}