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
#define biomeSize 16
#define landmassSize 32
#define tectonicPlateBit  	1
#define tectonicLandmassBit 2
#define landmassBit 		4
#define polygonIslandsBit 	8
#define biomeBit 			16
#define biomeFinderBit		32

unsigned char** map;
unsigned char** biomeMap;
unsigned char** landmassMap;
char textMode = 0;
char bmpMode = 0;
char visual = 0b11111111;
unsigned char r,g,b = 0;
int tectonicPlates = 0;
int mapSizeX, mapSizeY;
int maximumVerticies;
int initialSeed;

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
	mountains,
	desert,
	beach,
	forest,
	grasslands,
	snowy,
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
	iceTile,
	errorTile,
	numTiles,
} tilename;

// NOTE: THIS ONLY ACCOUNT FOR A SINGLE WRAP AROUND, NOT MULTIPLE!!!!
int getWrappedAround(int location, int maxValue) {
	if (location < 0) {
		location += maxValue;
	} else if (location > maxValue) {
		location = location%maxValue;
	}
    return location;//%maxValue;
}

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
char id = emptyTile;
char surrounding[4] = {emptyTile,emptyTile,emptyTile,emptyTile};
int invalidTile = emptyTile;
int previousRandomValues[20] = { 0 };
SDL_Renderer *renderer;

int updateProgressBar(int progress, int mode) {
	SDL_Event event;
    SDL_PollEvent(&event);
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
		case 0: // Tectonic Plates
			SDL_SetRenderDrawColor(renderer, 255, 16, 1, 255);
			break;
		case 1: // Tectonic Landmass
			SDL_SetRenderDrawColor(renderer, 255, 64, 2, 255);
			break;
		case 2: // Landmass
			SDL_SetRenderDrawColor(renderer, 192, 192, 4, 255);
			break;
		case 3: // Biome
			SDL_SetRenderDrawColor(renderer, 32, 255, 16, 255);
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
	switch(map[x][y]) {
		case emptyTile: // Empty
			r = 0;
			g = 0;
			b = 0;
			break;
		case treeTile: // Tree
			r = 17;
			g = 95;
			b = 66;
			break;
		case bushTile: // Bush
			r = 94;
			g = 187;
			b = 32;
			break;
		case grassTile: // Grass
			r = 182;
			g = 236;
			b = 101;
			break;
		case sandTile: // Sand
			r = 250;
			g = 222;
			b = 168;
			break;
		case riverTile: // River Water
			r = 160;
			g = 199;
			b = 244;
			break;
		case oceanTile: // Ocean Water
			r = 47;
			g = 112;
			b = 196;
			break;
		case stoneTile: // Stone
			r = 114;
			g = 121;
			b = 130;
			break;
		case iceTile: // Snow/Ice
			r = 223;
			g = 248;
			b = 255;
			break;
		default: // Error
			r = 255;
			g = 0;
			b = 255;
			break;
	};
	if (bmpMode) {
		return 0;
	} else {
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
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

int wrapped_distance(int x1, int y1, int x2, int y2, int width, int height) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    // Wrap the distances around the torus
    int wrapped_dx = (dx < width - dx) ? dx : width - dx;
    int wrapped_dy = (dy < height - dy) ? dy : height - dy;

    // Calculate the wrapped distance
    int wrapped_dist = sqrt(wrapped_dx * wrapped_dx + wrapped_dy * wrapped_dy);
    return wrapped_dist;
}

// Random with a maximum
// Max included
int getRandomLimited(int max) {
	max = max+1;
	return rand() % max;
}

// Random with a minimum and maximum
int getRandomLimitedMinMax(int min, int max) {
    max = max + 1; // Include the upper bound (max) in the range
    int result = (rand() % (max - min)) + min;
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

// Both of these are broken!
// Remove surrounding array!
// Checks if another tile is surrounding the current tile at any other side
int isNeighboring(int tile) {
	return 0;
	/*if ((surrounding[0] == tile) || (surrounding[1] == tile) || (surrounding[2] == tile) || (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}*/
}

// Checks to see if a tile is surrounded by another type of tile
int surroundedBy(int tile) {
	return 0;
	/*
	if ((surrounding[0] == tile) && (surrounding[1] == tile) && (surrounding[2] == tile) && (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}*/
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
	float* polygonX = (float*)malloc(maximumVerticies * sizeof(float));;
	float* polygonY = (float*)malloc(maximumVerticies * sizeof(float));;

	float angleIncrement = 2 * M_PI / maximumVerticies;  // Calculate the angle increment
	float currentAngle = (float)getRandomLimited(360)/360.0f;  // Starting angle

	// Set random displacements of points
	for (int pointID = 0; pointID < maximumVerticies; pointID++) {
		// Interpolate continent outline between points
		float x2 = cos(currentAngle) * (float)islandSize * getSmoothedRandomLimited(5, 10) + (float)x1;
		float y2 = sin(currentAngle) * (float)islandSize * getSmoothedRandomLimited(5, 10) + (float)y1;
		polygonX[pointID] = x2;
		polygonY[pointID] = y2;
		
		currentAngle += angleIncrement;  // Increment the angle for the next point
	}
	
	// Draw Polygon on map
    if (visual & polygonIslandsBit) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		for (int i = 0; i < maximumVerticies - 1; ++i) {
			SDL_RenderDrawLine(renderer, polygonX[i], polygonY[i], polygonX[i + 1], polygonY[i + 1]);
		}
	}
	
	// Fill out tiles inside polygon
	// Note: Probably better to do later once all islands have been placed!!!
	for (int mapY = -mapSizeY; mapY < mapSizeY+mapSizeY; mapY++) {
		for (int mapX = -mapSizeX; mapX < mapSizeX+mapSizeX; mapX++) {
			int i, j;
			float pointX = (float)mapX;
			float pointY = (float)mapY;
			char isInside = 0;

			for (i = 0, j = maximumVerticies - 1; i < maximumVerticies; j = i++) {
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
		int pointA = getRandomLimited(maximumVerticies);
		int pointB = getRandomLimited(maximumVerticies);
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
	/*x = getWrappedAround(x,mapSizeX/biomeSize);
	y = getWrappedAround(y,mapSizeY/biomeSize);
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
			return 0;
	}*/
	return 0;
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
		case snowy:
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
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
	float scaler = goal/max;
	return (int)((float)value*scaler);
}

// Function to create and save as BMP file
void saveBMP(const char* filename) {
	/*
	char str0[32];
	char str1[32];
	char str2[64];
	char ending[4] = ".bmp";
	sprintf(str0, "-%d-", mapSizeX);
	sprintf(str1, "%d", mapSizeY);
	sprintf(str2, "_%d_", initialSeed);
	strcat(filename, str0);
	strcat(filename, str1);
	strcat(filename, str2);
	strcat(filename, ending);
	*/
    FILE* file = fopen(filename, "wb");

    if (!file) {
        printf("Error opening file\n");
        return;
    }

    uint32_t imageSize = mapSizeX * mapSizeY * 3;
    uint32_t headerSize = 54;
    uint32_t fileSize = imageSize + headerSize;

    // BMP file header
    uint8_t header[54] = {
        'B', 'M',                 // BMP signature
        (uint8_t)(fileSize),      // File size
        (uint8_t)(fileSize >> 8),
        (uint8_t)(fileSize >> 16),
        (uint8_t)(fileSize >> 24),
        0, 0, 0, 0,              // Reserved
        headerSize, 0, 0, 0,     // Image data offset
        40, 0, 0, 0,             // DIB header size
        (uint8_t)(mapSizeX),        // Image width
        (uint8_t)(mapSizeX >> 8),
        (uint8_t)(mapSizeX >> 16),
        (uint8_t)(mapSizeX >> 24),
        (uint8_t)(mapSizeY),       // Image height
        (uint8_t)(mapSizeY >> 8),
        (uint8_t)(mapSizeY >> 16),
        (uint8_t)(mapSizeY >> 24),
        1, 0,                    // Number of color planes
        24, 0,                   // Bits per pixel (24-bit color)
        0, 0, 0, 0,              // Compression method
        (uint8_t)(imageSize),    // Image size
        (uint8_t)(imageSize >> 8),
        (uint8_t)(imageSize >> 16),
        (uint8_t)(imageSize >> 24),
        0, 0, 0, 0,              // Horizontal resolution (pixels per meter)
        0, 0, 0, 0,              // Vertical resolution (pixels per meter)
        0, 0, 0, 0,              // Number of colors in the color palette
        0, 0, 0, 0,              // Number of important colors used
    };

    // Write the BMP file header
    fwrite(header, sizeof(uint8_t), 54, file);

    // Write image data (BGR format)
	for (int i = 0; i < mapSizeX*mapSizeY; i++) {
		printTile(i%mapSizeX,i/mapSizeY);
        char pixel[3] = {
            (char)(b),              // Blue component (LSB)
            (char)(g >> 8),         // Green component
            (char)(r >> 16)         // Red component (MSB)
        };
        fwrite(pixel, sizeof(char), 3, file);
	}

    fclose(file);
}

int WinMain(int argc, char **argv) {
	// implement reading of parameters!!
	
	mapSizeX = 1024;
	mapSizeY = 512;
	maximumVerticies = 30;
	
	// Initialize maps
	map = (unsigned char**)malloc(mapSizeX * sizeof(unsigned char*));
    for (int i = 0; i < mapSizeX; i++) {
        map[i] = (unsigned char*)malloc(mapSizeY * sizeof(unsigned char));
    }
	
	biomeMap = (unsigned char**)malloc(mapSizeX/biomeSize * sizeof(unsigned char*));
    for (int i = 0; i < mapSizeX/biomeSize; i++) {
        biomeMap[i] = (unsigned char*)malloc(mapSizeY/biomeSize * sizeof(unsigned char));
    }
	
	landmassMap = (unsigned char**)malloc(mapSizeX/landmassSize * sizeof(unsigned char*));
    for (int i = 0; i < mapSizeX/landmassSize; i++) {
        landmassMap[i] = (unsigned char*)malloc(mapSizeY/landmassSize * sizeof(unsigned char));
    }
	
	tectonicPlates = mapSizeX/(landmassSize*8);
	
	// SDL2 Prep
	// Seed the map
	float renderScale = 1.0f;
	
	// Initialize SDL
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());

	// Get Desktop size
	SDL_DisplayMode dm;

	if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
	{
		 SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
		 return 1;
	}

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("Mapgen", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w/3*2, dm.h/3*2, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); 
	//SDL_RenderSetScale(renderer, renderScale, renderScale);
	SDL_RenderSetLogicalSize(renderer, mapSizeX, mapSizeY);   
    CHECK_ERROR(renderer == NULL, SDL_GetError());

    // Initial renderer color
	restart:
	initialSeed = time(NULL);
	printf("%d\n",initialSeed);
	srand(initialSeed);
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);	
	SDL_RenderClear(renderer);
	
	// Pregen
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			placeTile(x,y,oceanTile);
		}
	}
	printf("Pregeneration\n");
	
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
    int* tectonicPlatesOriginX = (int*)malloc(tectonicPlates * sizeof(int));
    int* tectonicPlatesOriginY = (int*)malloc(tectonicPlates * sizeof(int));
    int* tectonicPlatesSize = (int*)malloc(tectonicPlates * sizeof(int));
	for (int i = 0; i < tectonicPlates; i++) {
		tectonicPlatesOriginX[i] = (int)getRandomLimited(mapSizeX);
		tectonicPlatesOriginY[i] = (int)getRandomLimitedMinMax(mapSizeY/5,mapSizeY/5*4);
		tectonicPlatesSize[i] = (int)getRandomLimitedMinMax(5,20);
	}
	printf("Tectonic Plate Points\n");
	
	// Tectonic Plate Visualzation
	if (visual & tectonicPlateBit) {
		for (int y = 0; y < mapSizeY; y++) {
			//updateProgressBar(scaleToValue(y,mapSizeY,100)+1,0);
			for (int x = 0; x < mapSizeX; x++) {
			// this is for a single landmass "chunk"
				int closest = mapSizeX*mapSizeY;
				int closestPoint = 0;
				for (int k = 0; k < tectonicPlates; k++) {
					int value = wrapped_distance(x,y,tectonicPlatesOriginX[k],tectonicPlatesOriginY[k],mapSizeX,mapSizeY);
					if (value < closest) {
						closestPoint = k;
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
	}
	
	
	int numberOfIslands = 10;
	int islandBaseSize = 3;
	int maxRandomModifer = 16;
	int maxRandomIslandModifier = 0;
	// landmass
	
	for (int landmassY = 0; landmassY < mapSizeY/landmassSize; landmassY++) {
		updateProgressBar(scaleToValue(landmassY,mapSizeY/landmassSize,100)+1,1);
		for (int landmassX = 0; landmassX < mapSizeX/landmassSize; landmassX++) {			
			// Initial random landmass
			int landmass = 0;
			if (getRandomLimited(100)>95) {
				landmass = getRandomLimitedMinMax(ocean,archipelago);
			}
			int closest = mapSizeX;
			int closestPoint = 0;
			// Then concrete ones based on tectonic plates
			for (int i = 0; i < tectonicPlates; i++) {
				int value = wrapped_distance(landmassX*landmassSize,landmassY*landmassSize,tectonicPlatesOriginX[i],tectonicPlatesOriginY[i],mapSizeX,mapSizeY);
				if (value < closest) {
					closestPoint = i;
					closest = value;
				}
			}
			if (closest < (2*tectonicPlatesSize[closestPoint])+getRandomLimited(10)) {
				landmass = mainland;
				if (visual & tectonicLandmassBit) {
					SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
					SDL_SetRenderDrawColor(renderer, 64, 32, 32, 128);
					SDL_RenderFillRect(renderer, &rect);
				}
			} else if (closest < (6*tectonicPlatesSize[closestPoint])+getRandomLimited(20)) {
				landmass = continent;
				if (visual & tectonicLandmassBit) {
					SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
					SDL_SetRenderDrawColor(renderer, 128, 32, 32, 128);
					SDL_RenderFillRect(renderer, &rect);
				}
			} else if (closest < (12*tectonicPlatesSize[closestPoint])) {
				if (getRandomLimited(20)>15) {
					landmass = continent;
				}
				if (visual & tectonicLandmassBit) {
					SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
					SDL_SetRenderDrawColor(renderer, 192, 32, 32, 128);
					SDL_RenderFillRect(renderer, &rect);
				}
			}
			if (landmass == emptylandmass) {
				landmass = ocean;
			}
			landmassMap[landmassX][landmassY] = landmass;
		}
	}
	printf("Landmass Generation\n");
	
	// Generate Islands
	for (int landmassY = 0; landmassY < mapSizeY/landmassSize; landmassY++) {
		//updateProgressBar(scaleToValue(landmassY,mapSizeY/landmassSize,100)+1,2);
		for (int landmassX = 0; landmassX < mapSizeX/landmassSize; landmassX++) {
			updateProgressBar(scaleToValue(landmassY*(mapSizeX/landmassSize)+landmassX,(mapSizeY/landmassSize)*(mapSizeX/landmassSize),100)+1,1);
			// USE THIS TO MAKE CHUNKING POSSIBLE!!!!
			switch(landmassMap[landmassX][landmassY]) {
				case mainland:
					maxRandomModifer = 6;
					islandBaseSize = 12;
					numberOfIslands = 1;
					maxRandomIslandModifier = 0;
					SDL_SetRenderDrawColor(renderer, 192, 192, 192, 128);	
					printf("^");
					break;
				case continent:
					maxRandomModifer = 10;
					islandBaseSize = 10;
					numberOfIslands = 1;
					maxRandomIslandModifier = 1;
					SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);	
					printf("#");
					break;
				case archipelago:
					maxRandomModifer = 5;
					islandBaseSize = 2;
					numberOfIslands = 2;
					maxRandomIslandModifier = 1;
					SDL_SetRenderDrawColor(renderer, 64, 64, 64, 128);	
					printf("o");
					break;
				case islands:
					maxRandomModifer = 3;
					islandBaseSize = 1;
					numberOfIslands = 2;
					maxRandomIslandModifier = 2;
					SDL_SetRenderDrawColor(renderer, 32, 32, 32, 128);	
					printf(":");
					break;
				default:
				case ocean:
					maxRandomModifer = 0;
					islandBaseSize = 0;
					numberOfIslands = 0;
					maxRandomIslandModifier = 0;
					SDL_SetRenderDrawColor(renderer, 16, 16, 16, 64);	
					printf("~");
					break;
			}
			
			if (visual & landmassBit) {
				SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
				SDL_RenderFillRect(renderer, &rect);
			}
			
			// Generate islands
			int maximumNumberOfIslands = numberOfIslands + getRandomLimited(maxRandomIslandModifier);
			for (int i = 1; i <= maximumNumberOfIslands; i++) {
				int islandSize = islandBaseSize+getRandomLimited(maxRandomModifer);
				int x1 = landmassX*landmassSize+(int)getRandomLimited(landmassSize);//(mapSizeX/(1+getRandomLimited(16)));
				int y1 = landmassY*landmassSize+(int)getRandomLimited(landmassSize);//(mapSizeY/(1+getRandomLimited(16)));
				
				//randomIsland(x1,y1,islandSize);
				polygonIsland(x1,y1,islandSize);
			}
		}
		printf("\n");
	}
	printf("Island Generation\n");
	
	// Biomes based on landmass
	for (int biomeY = 0; biomeY < mapSizeY/biomeSize; biomeY++) {
		updateProgressBar(scaleToValue(biomeY+1,mapSizeY/biomeSize,100),3);
		for (int biomeX = 0; biomeX < mapSizeX/biomeSize; biomeX++) {
			int biome = emptybiome;
			//printf("%d,", checklandmass(biomeX*biomeSize,biomeY*biomeSize));	
			// Basic Post-processing
			if ((getRandomLimited(1) + getIntDistance(biomeX,0,biomeX,biomeY)) < ((mapSizeY/biomeSize)/5)) {
				biome = snowy;
			}
			if ((getRandomLimited(1) + getIntDistance(biomeX,mapSizeY/biomeSize,biomeX,biomeY)) < (((mapSizeY/biomeSize)/5))) {
				biome = snowy;
			}
			
			if ((biomeY + getRandomLimited(2) > ((mapSizeY/biomeSize)/5)*2)
				&& (biomeY - getRandomLimited(2) < ((mapSizeY/biomeSize)/5)*3)) {
				biome = desert;
			}
			
			// Checks to ensure adjance Biomes don't fuck up
			if (biome == emptybiome) {
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
						}
						break;
					default:
						biome = getRandomBiomeID();
						switch(biome) {
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
								if (checkBiomeForTile(biomeX,biomeY,oceanTile) ||
									getNeighboringBiomes(biomeX,biomeY,0) == desert ||
									getNeighboringBiomes(biomeX,biomeY,1) == desert ||
									getNeighboringBiomes(biomeX,biomeY,2) == desert ||
									getNeighboringBiomes(biomeX,biomeY,3) == desert) {
									biome = grasslands;
								} else {
									biome = forest;
								}
								break;
							default:
								biome = grasslands;
								break;
						}
						break;
				}
			}
			biomeMap[biomeX][biomeY] = biome;
			if (visual & biomeBit) {
				renderBiome(biomeX,biomeY);
			}
		}
		//printf("\n");
	}
	SDL_RenderPresent(renderer);
	printf("Biome Generation\n");
	
	// Prepare
	for (int mapY = 0; mapY < mapSizeY; mapY++) {
		for (int mapX = 0; mapX < mapSizeX; mapX++) {
			//printf("Started: %d,%d\n", mapX, mapY);
			int randomTile;
			if (map[mapX][mapY] == emptyTile) {
				// Get surrounding tiles
				// Unused right now
				int mapXminus = getWrappedAround(mapX-1,mapSizeX);
				int mapXplus = getWrappedAround(mapX+1,mapSizeX);
				int mapYminus = getWrappedAround(mapY-1,mapSizeY);
				int mapYplus = getWrappedAround(mapY+1,mapSizeY);
				/*surrounding[0] = map[mapXminus][mapY];
				surrounding[1] = map[mapXplus][mapY];
				surrounding[2] = map[mapX][mapYminus];
				surrounding[3] = map[mapX][mapYplus];*/
				
				// Actually put down tiles based on biomes
				int biome = biomeMap[mapX/biomeSize][mapY/biomeSize];
				// Find most common adjacent Biome
				int closestDistance = mapSizeX*mapSizeY;
				int closestBiome = biome;
				int biomeXpos = mapX/biomeSize;
				int biomeYpos = mapY/biomeSize;	
				// Biome Blending Priority
				/*
				Check surrounding tiles and smooth into closest one with higher priority
				ooo
				o.o
				ooo
				
				*/
				for (int yBiome = -1; yBiome <= 1; yBiome++) {
					for (int xBiome = -1; xBiome <= 1; xBiome++) {
						if ((xBiome == 0) && (yBiome == 0)) {
							// Own Biome
						} else {
							/*if (visual & biomeFinderBit) {
								SDL_SetRenderDrawColor(renderer, 255, 0, 0, 32);
								SDL_RenderDrawLine(
									renderer,
									mapX,
									mapY,
									((biomeXpos+xBiome)*biomeSize)+biomeSize/2,
									((biomeYpos+yBiome)*biomeSize)+biomeSize/2
								);
							}*/
							
							// Get distance to next biome
							
							int distance = getIntDistance(
								mapX,
								mapY,
								(getWrappedAround(biomeXpos+xBiome,mapSizeX/biomeSize)*biomeSize)+biomeSize/2,
								(getWrappedAround(biomeYpos+yBiome,mapSizeY/biomeSize)*biomeSize)+biomeSize/2
							);
							// Check closest distance
							if (distance < closestDistance) {
								closestDistance = distance;
								int closestBiomeX = getWrappedAround(biomeXpos-(xBiome*-1),mapSizeX/biomeSize);
								int closestBiomeY = getWrappedAround(biomeYpos-(yBiome*-1),mapSizeY/biomeSize);
								closestBiome = biomeMap[closestBiomeX][closestBiomeY];
							}
							//printf("%d:%d \n", biomeMap[biomeXpos-xBiome][biomeYpos-yBiome], distance);
						}
					}
				}
				//printf("Closest Biome: %d\n", closestBiome);
				if (wrapped_distance(
					mapX,
					mapY,
					((biomeXpos)*biomeSize)+biomeSize/3,
					((biomeYpos)*biomeSize)+biomeSize/3,
					mapSizeX,
					mapSizeY
				) > 4) {
					if (biome < closestBiome) {
						biome = closestBiome;
					}
				}
				
				// Randomize a bit
				// TODO: Check how close one is to the biome border
				/*if (getRandomLimited(100)>95) {
					int neighborBiomes[4];
					for (int i = 0; i < 4; i++) {
						neighborBiomes[i] = getNeighboringBiomes(mapX/biomeSize,mapY/biomeSize,i);
					}
					biome = neighborBiomes[getRandomLimited(3)];
				}*/
				
				// Place tiles based on Biome
				switch(biome) {
					case beach:
						randomTile = getRandomLimitedMinMax(oceanTile,sandTile);
						placeTile(mapX,mapY,randomTile);
						break;
					case desert:
						placeTile(mapX,mapY,sandTile);
						break;
					default:
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
						} else if (isNeighboring(grassTile)) {
							randomTile = bushTile;
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
					case snowy:
						if (isNeighboring(sandTile)) {
							randomTile = grassTile;
						} else {
							randomTile = iceTile;
						}
						placeTile(mapX,mapY,randomTile);
						break;
				}
			}
		}
	}
	printf("Tile Placement\n");
	
	// Print Map
    char running = 1;
	//printMap();
	
    // Clear screen	
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
                if(strcmp(key, "W") == 0) {
					bmpMode = 1;
					saveBMP("map");
					bmpMode = 0;
                }     
                if(strcmp(key, "E") == 0) {
                    printMap();
                }     
                if(strcmp(key, "R") == 0) {
                    goto restart;
                }                    				
            }
        }

        // Draw

        // Show what was drawn
        SDL_RenderPresent(renderer);
    }
	return 0;
}