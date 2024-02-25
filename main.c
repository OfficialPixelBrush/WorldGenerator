#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <pthread.h> 

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
#define landmassSize 16
#define PI 3.14159265359

#define tectonicPlateBit  	1
#define tectonicLandmassBit 2
#define landmassBit 		4
#define polygonIslandsBit 	8
#define biomeBit 			16
#define biomeFinderBit		32
#define finalMapRender		64
#define nothingYet 			128

#define numberOfOldRandomValues 20

struct biomeStruct {
	unsigned char biomeID;
	unsigned char biomeCenterOffsetX;
	unsigned char biomeCenterOffsetY;
};
typedef struct biomeStruct biomeInfo;

struct color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};
typedef struct color color;

unsigned char** map;
int** heightMap;
biomeInfo** biomeMap;
unsigned char** landmassMap;
color** finalMap;
char textMode = 0;
char visual = tectonicPlateBit | tectonicLandmassBit | polygonIslandsBit |biomeBit | finalMapRender;
unsigned char r,g,b = 0;
int tectonicPlates = 0;
int mapSizeX, mapSizeY;
int maximumVerticies;
int initialSeed;
char animate = 0;
int heighestHeight = 0;
int deepestDepth = -255;

double lightAngle = 3.5;

// Export Options
char bmpMode = 0;
char heightmapExport = 0;
char albedoExport = 0;

float biomeMapSizeX, biomeMapSizeY = 0;

// Threading
int numberOfThreads = 16;
pthread_t *tids = NULL;
int *threadProgress = NULL;
int finishedThreads = 0;

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

// Biomes
typedef enum {
	emptybiome,
	mountains,
	desert,
	beach,
	forest,
	grasslands,
	tundra,
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
	snowTile,
	iceTile,
	errorTile,
	numTiles,
} tilename;

// NOTE: THIS ONLY ACCOUNT FOR A SINGLE WRAP AROUND, NOT MULTIPLE!!!!
int getWrappedAround(int location, int maxValue) {
	if (location < 0) {
		location += maxValue;
	} else if (location >= maxValue) {
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
	return rand() % numBiomes;
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
int previousRandomValues[numberOfOldRandomValues] = { 0 };
SDL_Renderer *renderer;

int updateProgressBar(int progress, int mode) {
	// Adjust
	progress+=1;
	// Fake event to keep SDL from freezing
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
			SDL_SetRenderDrawColor(renderer, 255, 66, 66, 255);
			break;
		case 1: // Tectonic Landmass
			SDL_SetRenderDrawColor(renderer, 255, 141, 83, 255);
			break;
		case 2: // Landmass
			SDL_SetRenderDrawColor(renderer, 255, 216, 100, 255);
			break;
		case 3: // Biome
			SDL_SetRenderDrawColor(renderer, 211, 236, 90, 255);
			break;
		case 4: // Blended Biomes
			SDL_SetRenderDrawColor(renderer, 167, 255, 80, 255);
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

int find_max_along_line(int** arr, int m, int n, int x1, int y1, int x2, int y2) {
  int dx = x2 - x1, dy = y2 - y1;
  int max_value = INT_MIN;
  for (float t = 0; t <= 1; t += 0.01) { // Adjust step size based on desired precision
    int x = getWrappedAround((int)round(x1 + t * dx),mapSizeX);
    int y = getWrappedAround((int)round(y1 + t * dy),mapSizeY);
    // Check if x and y are within array bounds and update max_value if necessary
    if (x >= 0 && x < m && y >= 0 && y < n && arr[x][y] > max_value) {
      max_value = arr[x][y];
    }
  }
  return max_value;
}

int getCoordinatesRelativeToCenter(double angle, double distance, double center_x, double center_y, char xOrY) {
  double x_double = center_x + distance * cos(angle);
  double y_double = center_y + distance * sin(angle);

  // Round coordinates to nearest integer
  if (xOrY) {
  	return (int)round(x_double);
  } else {
  	return (int)round(y_double);
  }
}

// Render a tile
int printTile(int x, int y) {
	int currentTile = map[x][y];
	switch(currentTile) {
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
		case snowTile: // Snow
			r = 223;
			g = 248;
			b = 255;
			break;
		case iceTile: // Ice
			r = 179;
			g = 214;
			b = 241;
			break;
		default: // Error
			r = 255;
			g = 0;
			b = 255;
			break;
	};
	// Shade
	if (!albedoExport) {
		int heightAtCoordinate = heightMap[x][y];
		int distantX = getCoordinatesRelativeToCenter(lightAngle,2.0,(double)x,(double)y, 1);
		int distantY = getCoordinatesRelativeToCenter(lightAngle,2.0,(double)x,(double)y, 0);
		float shade = 0.0f;
		int maxAlongLine = find_max_along_line(heightMap, mapSizeX, mapSizeY, x, y, distantX, distantY);
		
		if ((maxAlongLine > heightAtCoordinate)) {
			float untilPeak = ((float)heightAtCoordinate)/((float)255.0f);
			float heightDifference = ((float)heightAtCoordinate)/((float)maxAlongLine);
			shade = (1.0f-untilPeak)*heightDifference;
		} else {
			shade = 1.0f;
		}

		if (currentTile == oceanTile) {
			if (heightAtCoordinate < 128) {
				shade *= ((float)heightAtCoordinate)/255.0f;
			}
		}
		//float height = (((float)heightMap[x][y])/255.0f);
		float finalR = (float)r/255.0f;
		float finalG = (float)g/255.0f;
		float finalB = (float)b/255.0f;
		finalR *= shade;
		finalG *= shade;
		finalB *= shade;
		r = ((int)(finalR*255.0f))&0xFF;
		g = ((int)(finalG*255.0f))&0xFF;
		b = ((int)(finalB*255.0f))&0xFF;
	}
	if (heightmapExport) {
		r = heightMap[x][y];
		g = heightMap[x][y];
		b = heightMap[x][y];
	}

	if (!bmpMode) {
		finalMap[x][y].red = r;
		finalMap[x][y].green = g;
		finalMap[x][y].blue = b;
	}	
	return 0;
}

int normalizeHeightmap() {
	for (int y = 0; y < mapSizeY; y++) {
		for (int x = 0; x < mapSizeX; x++) {
			int height = heightMap[x][y];
			float relativeHeight = 0;
			if (height >= 0) {
				relativeHeight = (float)height/(float)heighestHeight;
			} else {
				relativeHeight = ((float)height/(float)deepestDepth)*-1;
			}
			relativeHeight = (relativeHeight/2)+0.5f;
			heightMap[x][y] = (int)(relativeHeight*255.0f);
			SDL_SetRenderDrawColor(renderer, heightMap[x][y],heightMap[x][y],heightMap[x][y],255);
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}
	SDL_RenderPresent(renderer);
}

int addNoiseToHeightmap(int noiseIntensity) {
	for (int y = 0; y < mapSizeY; y++) {
		for (int x = 0; x < mapSizeX; x++) {
			heightMap[x][y] += (rand()%noiseIntensity)-(noiseIntensity/2);
			if (heightMap[x][y] > heighestHeight) {
				heighestHeight = heightMap[x][y];
			}
			if (heightMap[x][y] < deepestDepth) {
				deepestDepth = heightMap[x][y];
			}
		}
	}
}

// This'll be a lazy Box blur
int blurHeightmap(int blurRadius) {
	int** blurredHeightMap;
	blurredHeightMap = (int**)malloc(mapSizeX * sizeof(int*));
    for (int i = 0; i < mapSizeX; i++) {
        blurredHeightMap[i] = (int*)malloc(mapSizeY * sizeof(int));
    }

	// Iterate over every pixel
	for (int y = 0; y < mapSizeY; y++) {
		for (int x = 0; x < mapSizeX; x++) {
			// Internal iteration
			int total = 0;
			int i = 0;
			for (int a = blurRadius*-1; a <= blurRadius; a++) {
				for (int b = blurRadius*-1; b <= blurRadius; b++) {
					int internalX = getWrappedAround(x+a,mapSizeX);
					int internalY = getWrappedAround(y+b,mapSizeY);
					//printf("%d,%d\n", internalX, internalY);
					total += heightMap[internalX][internalY];
					//printf("%d: %d,%d: %d\n", i, a,b,total);
					i++;
				}
			}
			total = total/i;
			/*
			if (total > heighestHeight) {
				total = heighestHeight;
			}
			if (total < deepestDepth) {
				total = deepestDepth;
			}*/
			blurredHeightMap[x][y] = total;
			SDL_SetRenderDrawColor(renderer, total,total,total,255);
			SDL_RenderDrawPoint(renderer, x, y);
		}
		//if (y%biomeSize) {
		//}
	}
	SDL_RenderPresent(renderer);
	
	// Copy to main heightmap
	for (int y = 0; y < mapSizeY; y++) {
		for (int x = 0; x < mapSizeX; x++) {
			heightMap[x][y] = blurredHeightMap[x][y];
		}
	}
	
	free(blurredHeightMap);
	return 0;
}

void *printSectionOfMap(void *vargp) {
    int index = (int)vargp;
	int minSegment = mapSizeX/numberOfThreads;
	int start = index*minSegment;
	int end   = index*minSegment+minSegment-1;
	//printf("%d: %d -> %d\n", index, start, end);
	for (x = start; x < end; x++) {
		for (y = 0; y < mapSizeY; y++) {
			printTile(x,y);
			//printf("%d: %d,%d\n", index, x,y);
		}
	}
	finishedThreads++;
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
		for (x = 0; x < mapSizeX; x++) {
			for (y = 0; y < mapSizeY; y++) {
				printTile(x,y);
				//printf("%d: %d,%d\n", index, x,y);
			}
		}
		/*
		tids = (pthread_t *)malloc(numberOfThreads * sizeof(pthread_t));
		
		if (tids == NULL) {
			// handle memory allocation failure
			printf("Could not allocate memory to threads!\n");
			return 1;
		}
		
		// Create Threads
		finishedThreads = 0;
		for (int i = 0; i < numberOfThreads; i++) {
			int ret = pthread_create(&tids[i], NULL, printSectionOfMap, (void *)i); 
			if (ret != 0) {
				printf("Thread %d failed to create! Error code %d\n", i, ret);
				return 1;
			}
		}

		// Supervise
		//while (finishedThreads < numberOfThreads) {
			//printf("%d/%d\n", finishedThreads, numberOfThreads);
		//}
		
		// Rejoin them
		for (int i = 0; i < numberOfThreads; i++) {
			pthread_join(tids[i], NULL);
		}
		free(tids);*/
	}
}

// Place a tile on the map
void placeTile(int x, int y, int tile) {
	x = getWrappedAround(x,mapSizeX);
	y = getWrappedAround(y,mapSizeY);
	map[x][y] = tile;
}

void zeroHeightMapIfLessThanZero(int x, int y) {
	x = getWrappedAround(x,mapSizeX);
	y = getWrappedAround(y,mapSizeY);
	if (heightMap[x][y] < 0) {
		heightMap[x][y] = 0;
	}
}

void increaseHeightMap(int x, int y) {
	x = getWrappedAround(x,mapSizeX);
	y = getWrappedAround(y,mapSizeY);
	heightMap[x][y]+=1;
	if (heightMap[x][y] > heighestHeight) {
		heighestHeight = heightMap[x][y];
	}
}

// Place a tile on the map
int getTile(int x, int y) {
	x = getWrappedAround(x,mapSizeX);
	y = getWrappedAround(y,mapSizeY);
	return map[x][y];
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
	for (int i = 1; i < numberOfOldRandomValues-1; i++) {
		//printf("%d -> %d\n",previousRandomValues[i],previousRandomValues[i+1]);
		previousRandomValues[i] = previousRandomValues[i+1];
	}
	previousRandomValues[numberOfOldRandomValues] = rand() % max;
	
	for (int i = numberOfOldRandomValues; i > numberOfOldRandomValues-smoothSteps; i--) {
		smoothedValue+=(float)previousRandomValues[i];
	}
	//printf("%f\n",smoothedValue/(float)smoothSteps);
	
	return smoothedValue/(float)smoothSteps;
}

// Both of these are broken!
// Remove surrounding array!
// Checks if another tile is surrounding the current tile at any other side
int isNeighboring(int tile) {
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
void randomIsland(int x1, int y1, float islandSize) {
	islandSize *= (float)(getRandomLimited(10))/7.0f;
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			int x2 = x;
			int y2 = y;
			int distanceToIslandCenter = getIntDistance(x1,y1,x2,y2);
			// Generate Water Circle
			if (distanceToIslandCenter < islandSize*(getRandomLimited(5))) {
				map[x2][y2] = emptyTile;
				SDL_RenderDrawPoint(renderer, x, y);
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
void polygonIsland(int x1, int y1, float islandSize, float smoothness) {
	// Set number of points along circle
	float* polygonX = (float*)malloc(maximumVerticies * sizeof(float));;
	float* polygonY = (float*)malloc(maximumVerticies * sizeof(float));;

	float angleIncrement = 2 * M_PI / maximumVerticies;  // Calculate the angle increment
	float currentAngle = (float)getRandomLimited(360)/360.0f;  // Starting angle

	// Set random displacements of points
	for (int pointID = 0; pointID < maximumVerticies; pointID++) {
		//printf("%d/%d\n", pointID, maximumVerticies);
		// Interpolate continent outline between points
		float x2 = cos(currentAngle) * islandSize * getSmoothedRandomLimited(5, smoothness) + (float)x1;
		float y2 = sin(currentAngle) * islandSize * getSmoothedRandomLimited(5, smoothness) + (float)y1;
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
				zeroHeightMapIfLessThanZero(mapX, mapY);
				increaseHeightMap(mapX, mapY);
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
	x = getWrappedAround(x*biomeSize,mapSizeX);
	y = getWrappedAround(y*biomeSize,mapSizeY);
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
	x = getWrappedAround(x,((int)biomeMapSizeX));
	y = getWrappedAround(y,((int)biomeMapSizeY));
	int neighboringOnSides;
	if (biomeMap[x+1][y].biomeID == biome) {
		neighboringOnSides++;
	}
	if (biomeMap[x][y+1].biomeID == biome) {
		neighboringOnSides++;
	}
	if (biomeMap[x-1][y].biomeID == biome) {
		neighboringOnSides++;
	}
	if (biomeMap[x][y-1].biomeID == biome) {
		neighboringOnSides++;
	}
	return neighboringOnSides;
}

// Check which landmass the tile belongs to
int checklandmass(int x, int y) {
	x = x/landmassSize;
	y = y/landmassSize;
	x = getWrappedAround(x,mapSizeX/landmassSize);
	y = getWrappedAround(y,mapSizeY/landmassSize);
	return landmassMap[x][y];
}

// This segfaults a lot...
int getNeighboringBiomes(int x, int y, int direction) {
	//printf("x: %d, y: %d, d: %d\n", x, y, direction);
	switch(direction) {
		case 0:
			// North
			y-=1;
			break;
		case 1:
			// East
			x+=1;
			break;
		case 2:
			// South
			y+=1;
			break;
		case 3:
			// West
			x-=1;
			break;
		default:
			printf("Invalid Direction\n");
			break;
	}
	x = getWrappedAround(x,mapSizeX/biomeSize);
	y = getWrappedAround(y,mapSizeY/biomeSize);
	return biomeMap[x][y].biomeID;
}

int renderBiome(int biomeX,int biomeY) {
	switch(biomeMap[biomeX][biomeY].biomeID) {
		case emptybiome:
			SDL_SetRenderDrawColor(renderer, 25, 25, 25, 128);	
			break;
		case mountains:
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);	
			break;
		case desert:
			SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);	
			break;
		case beach:
			SDL_SetRenderDrawColor(renderer, 192, 255, 0, 128);	
			break;
		case forest:
			SDL_SetRenderDrawColor(renderer, 0, 128, 0, 128);	
			break;
		case grasslands:
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128);	
			break;
		case tundra:
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

// NOTE: Breaks with non-square images
// Function to create and save as BMP file
void saveBMP() {
    char outputFilename[256];
    char str0[32];
    char str1[32];
    char str2[64];
    char str3[64];
    sprintf(str0, "map_%d-", mapSizeX);
    sprintf(str1, "%d_", mapSizeY);
    sprintf(str2, "%d_", initialSeed);
    sprintf(str3, "%d", time(NULL));

    strcpy(outputFilename, str0);
    strcat(outputFilename, str1);
    strcat(outputFilename, str2);
    strcat(outputFilename, str3);
	if (heightmapExport) {
    	strcat(outputFilename, "_height");
	}
	if (albedoExport) {
    	strcat(outputFilename, "_height");
	}
    strcat(outputFilename, ".bmp");

    FILE* file = fopen(outputFilename, "wb");

    if (!file) {
        printf("Error opening file\n");
        return;
    }

    uint32_t imageSize = mapSizeX * mapSizeY * 3;
    uint32_t headerSize = 54;
    uint32_t fileSize = imageSize + headerSize;+
	printf("Filesize: %d", fileSize);

    // BMP file header
    uint8_t header[54] = {
        'B', 'M',                 // BMP signature
        (uint8_t)(fileSize & 0xFF),      // File size
        (uint8_t)(fileSize >> 8 & 0xFF),
        (uint8_t)(fileSize >> 16 & 0xFF),
        (uint8_t)(fileSize >> 24 & 0xFF),
        0, 0, 0, 0,              // Reserved
        headerSize, 0, 0, 0,     // Image data offset ; 14 bytes up to this point
        40, 0, 0, 0,             // DIB header size
        (uint8_t)(mapSizeX & 0xFF),        // Image width
        (uint8_t)(mapSizeX >> 8 & 0xFF),
        (uint8_t)(mapSizeX >> 16 & 0xFF),
        (uint8_t)(mapSizeX >> 24 & 0xFF),
        (uint8_t)(mapSizeY & 0xFF),       // Image height
        (uint8_t)(mapSizeY >> 8 & 0xFF),
        (uint8_t)(mapSizeY >> 16 & 0xFF),
        (uint8_t)(mapSizeY >> 24 & 0xFF),
        1, 0,                    // Number of color planes
        24, 0,                   // Bits per pixel (24-bit color)
        0, 0, 0, 0,              // Compression method
        //0, 0, 0, 0,              // Image size, a dummy 0 can be given for BI_RGB bitmaps.
        (uint8_t)(imageSize & 0xFF),    // Image size
        (uint8_t)(imageSize >> 8 & 0xFF),
        (uint8_t)(imageSize >> 16 & 0xFF),
        (uint8_t)(imageSize >> 24 & 0xFF),
        0x13, 0x0B, 0, 0,              // Horizontal resolution (pixels per meter)
        0x13, 0x0B, 0, 0,              // Vertical resolution (pixels per meter)
        0, 0, 0, 0,              // Number of colors in the color palette
        0, 0, 0, 0,              // Number of important colors used
    };

    // Write the BMP file header
    fwrite(header, sizeof(uint8_t), headerSize, file);

    // Write image data (BGR format)
	// Also write from bottom to top!!
	for (int y = mapSizeY; y > 0; y--) {
		for (int x = 0; x < mapSizeX; x++) {
			// Assuming r, g, and b represent the red, green, and blue components respectively for each pixel.
			printTile(x,y);
			char pixel[3] = {b,g,r};
			fwrite(pixel, sizeof(char), 3, file);
		}
		// Padding bytes
		for (int i = 0; i < mapSizeX*3%4; i++) {
			fputc(0,file);
		}
	}

    fclose(file);
}

// TODO: Right now, depending on the number of threads, the island generation can change
void *generateIslands(void *vargp) {
    int index = (int)vargp;
	int minSegment = (mapSizeX/landmassSize)/numberOfThreads;
	int startLandmass = index*minSegment;
	int endLandmass   = index*minSegment+minSegment;
	
	// Landmass globals
	int numberOfIslands = 10;
	int islandBaseSize = 3;
	int maxRandomModifer = 10;
	int maxRandomIslandModifier = 0;
	int smoothness = 5;

	// Generate Islands
	for (int landmassX = startLandmass; landmassX < endLandmass; landmassX++) {
		threadProgress[index] = landmassX-startLandmass;
		for (int landmassY = 0; landmassY < mapSizeY/landmassSize; landmassY++) {
			// USE THIS TO MAKE CHUNKING POSSIBLE!!!!
			switch(landmassMap[landmassX][landmassY]) {
				case mainland:
					maxRandomModifer = 7;
					islandBaseSize = 13;
					numberOfIslands = 1;
					maxRandomIslandModifier = 0;
					smoothness = 6;
					SDL_SetRenderDrawColor(renderer, 192, 192, 192, 128);	
					//printf("^");
					break;
				case continent:
					maxRandomModifer = 4;
					islandBaseSize = 6;
					numberOfIslands = 1;
					maxRandomIslandModifier = 1;
					smoothness = 4;
					SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);	
					//printf("#");
					break;
				case archipelago:
					maxRandomModifer = 2;
					islandBaseSize = 3;
					numberOfIslands = 2;
					maxRandomIslandModifier = 1;
					smoothness = 3;
					SDL_SetRenderDrawColor(renderer, 96, 96, 96, 128);	
					//printf("o");
					break;
				case islands:
					maxRandomModifer = 1;
					islandBaseSize = 2;
					numberOfIslands = 2;
					maxRandomIslandModifier = 2;
					smoothness = 2;
					SDL_SetRenderDrawColor(renderer, 64, 64, 64, 128);	
					//printf(":");
					break;
				default:
				case ocean:
					maxRandomModifer = 0;
					islandBaseSize = 0;
					numberOfIslands = 0;
					maxRandomIslandModifier = 0;
					smoothness = 0;
					SDL_SetRenderDrawColor(renderer, 16, 16, 16, 128);	
					//printf("~");
					break;
			}
			
			if (visual & landmassBit) {
				SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
				SDL_RenderFillRect(renderer, &rect);
			}
			
			// Generate islands
			int maximumNumberOfIslands = numberOfIslands + getRandomLimited(maxRandomIslandModifier);
			//int smoothness = smoothness+(getRandomLimited(2)-1);
			for (int i = 1; i <= maximumNumberOfIslands; i++) {
				float islandSize = islandBaseSize + (getRandomLimited(maxRandomModifer)-(maxRandomModifer/2));
				int x1 = landmassX*landmassSize+(int)getRandomLimited(landmassSize);//(mapSizeX/(1+getRandomLimited(16)));
				int y1 = landmassY*landmassSize+(int)getRandomLimited(landmassSize);//(mapSizeY/(1+getRandomLimited(16)));
				
				if (rand()%4) {
					// Polygon Island
					//printf("Poly\n");
					polygonIsland(x1,y1,islandSize,smoothness);
					//printf("Island");
				} else {
					// Random Island
					//printf("Random\n");
					//randomIsland(x1,y1,islandSize/2);
				}
			}
		}
		//printf("\n");
	}
}

void drawFinalMap() {
	for (int y = 0; y < mapSizeY; y++) {
		for (int x = 0; x < mapSizeX; x++) {
			color finalCol = finalMap[x][y];
			SDL_SetRenderDrawColor(renderer,finalCol.red,finalCol.green,finalCol.blue,255);
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}
}

void changeHeightmapOverBiomeArea(int biomeX, int biomeY, int change) {
	for (int x = biomeX*biomeSize; x < biomeX*biomeSize+biomeSize; x++) {	
		for (int y = biomeY*biomeSize; y < biomeY*biomeSize+biomeSize; y++) {
			heightMap[x][y] += change+getRandomLimited(1);
			if (heightMap[x][y] < 0) {
				heightMap[x][y] = 0;
			}
			if (heightMap[x][y] > heighestHeight) {
				heightMap[x][y] = heighestHeight;
			}
		}
	}
} 

int WinMain(int argc, char **argv) {
    //for (int i = 0; i < argc; i++) printf("argv[%d] = %s\n", i, argv[i]);
	//if (argc > 0) {
		//numberOfThreads = atoi(argv[0]);
		//printf("Number of Threads: %s\n", argv[0]);
	//}

	
	// implement reading of parameters!!
	if (numberOfThreads % 2) {
		printf("Number of threads MUST be multiple of two!\n");
		return 1;
	}
	
	mapSizeX = 1024;
	mapSizeY = 512;
	maximumVerticies = 25;
	heighestHeight = 1;
	
	// Initialize maps
	finalMap = (color**)malloc(mapSizeX * sizeof(color*));
    for (int i = 0; i < mapSizeX; i++) {
        finalMap[i] = (color*)malloc(mapSizeY * sizeof(color));
    }

	map = (unsigned char**)malloc(mapSizeX * sizeof(unsigned char*));
    for (int i = 0; i < mapSizeX; i++) {
        map[i] = (unsigned char*)malloc(mapSizeY * sizeof(unsigned char));
    }
	
	// Heightmaps
	heightMap = (int**)malloc(mapSizeX * sizeof(int*));
    for (int i = 0; i < mapSizeX; i++) {
        heightMap[i] = (int*)malloc(mapSizeY * sizeof(int));
    }
	
	// Biome Map
	biomeMapSizeX = (((float)mapSizeX)/((float)biomeSize))+0.5;
	biomeMapSizeY = (((float)mapSizeY)/((float)biomeSize))+0.5;
	
	biomeMap = (biomeInfo**)malloc((int)biomeMapSizeX * sizeof(biomeInfo*));
    for (int i = 0; i < mapSizeX/biomeSize; i++) {
        biomeMap[i] = (biomeInfo*)malloc((int)biomeMapSizeY * sizeof(biomeInfo));
    }
	
	landmassMap = (unsigned char**)malloc(mapSizeX/landmassSize * sizeof(unsigned char*));
    for (int i = 0; i < mapSizeX/landmassSize; i++) {
        landmassMap[i] = (unsigned char*)malloc(mapSizeY/landmassSize * sizeof(unsigned char));
    }
	
	tectonicPlates = 20; //mapSizeX/(landmassSize*(2+(getRandomLimited(10))));
	
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
    SDL_Window *window = SDL_CreateWindow("World Generator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w/3*2, dm.h/3*2, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); 
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	//SDL_RenderSetScale(renderer, renderScale, renderScale);
	SDL_RenderSetLogicalSize(renderer, mapSizeX, mapSizeY);   
    CHECK_ERROR(renderer == NULL, SDL_GetError());

    // Initial renderer color
	restart:
	//if (!animate) {
	initialSeed = time(NULL); // 1708518104 //1690361670; // time(NULL); // 1690274433;
	//}
	printf("%d\n",initialSeed);
	srand(initialSeed);
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);	
	SDL_RenderClear(renderer);
	
	// Pregen
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			placeTile(x,y,oceanTile);
			heightMap[x][y] = 0.0f;
		}
	}
	printf("Pregeneration\n");
	
	for (int i = 0; i < numberOfOldRandomValues; i++) {
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
		tectonicPlatesSize[i] = (int)getRandomLimitedMinMax(1,15);
	}
	printf("Tectonic Plate Points\n");
	
	// Tectonic Plate Visualzation
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
				closest = closest*-1;
				SDL_SetRenderDrawColor(renderer, closest, closest, closest, 255);
				heightMap[x][y] = closest;
			} else {
				SDL_SetRenderDrawColor(renderer, 32, 128, 255, 255);
				heightMap[x][y] = deepestDepth;
			}
			//printf("%d,%d: %d\n",x,y,255-heightMap[x][y]);
			if (visual & tectonicPlateBit) {
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}
	
	/*
	int numberOfIslands = 10;
	int islandBaseSize = 3;
	int maxRandomModifer = 10;
	int maxRandomIslandModifier = 0;
	*/
	// landmass
	
	for (int landmassY = 0; landmassY < mapSizeY/landmassSize; landmassY++) {
		updateProgressBar(scaleToValue(landmassY,mapSizeY/landmassSize,100),1);
		for (int landmassX = 0; landmassX < mapSizeX/landmassSize; landmassX++) {			
			// Initial random landmass
			int landmass = 0;
			int closest = mapSizeX;
			int closestPoint = 0;
			// Then concrete ones based on tectonic plates
			for (int i = 0; i < tectonicPlates; i++) {
				int value = wrapped_distance(landmassX*landmassSize,landmassY*landmassSize,tectonicPlatesOriginX[i],tectonicPlatesOriginY[i],mapSizeX,mapSizeY);
				value += getRandomLimited(20);
				if (value < closest) {
					closestPoint = i;
					closest = value;
				}
			}
			// If it's near the center, it's mainland
			if (closest < (2*tectonicPlatesSize[closestPoint])+getRandomLimited(10)) {
				landmass = mainland;
				if (visual & tectonicLandmassBit) {
					SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
					SDL_SetRenderDrawColor(renderer, 64, 32, 32, 128);
					SDL_RenderFillRect(renderer, &rect);
				}
			// A little farther out it's still part of the main continent
			} else if (closest < (6*tectonicPlatesSize[closestPoint])+getRandomLimited(20)) {
				landmass = continent;
				if (visual & tectonicLandmassBit) {
					SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
					SDL_SetRenderDrawColor(renderer, 128, 32, 32, 128);
					SDL_RenderFillRect(renderer, &rect);
				}
			// After this, landmasses become a bit scarcer and fade into the ocean
			} else if (closest < (12*tectonicPlatesSize[closestPoint])) {
				if (getRandomLimited(20)>15) {
					landmass = continent;
				}
				if (visual & tectonicLandmassBit) {
					SDL_Rect rect = {landmassX*landmassSize,landmassY*landmassSize,landmassSize,landmassSize};
					SDL_SetRenderDrawColor(renderer, 192, 32, 32, 128);
					SDL_RenderFillRect(renderer, &rect);
				}
			// Everything past this point has a VERY high chance of just being ocean
			} else if (closest < 12*+getRandomLimited(20)) {
				if (getRandomLimited(100)>95) {
					landmass = getRandomLimitedMinMax(ocean,archipelago);
				}
			} else {
				landmass = ocean;
			}
			
			// Finish up any empty tiles
			if (landmass == emptylandmass) {
				landmass = ocean;
			}
			landmassMap[landmassX][landmassY] = landmass;
		}
	}
	printf("Landmass Generation\n");
	tids = (pthread_t *)malloc(numberOfThreads * sizeof(pthread_t));
	threadProgress = (int *)malloc(numberOfThreads * sizeof(int));
	
	if (tids == NULL) {
		// handle memory allocation failure
		printf("Could not allocate memory to threads!\n");
		return 1;
	}
	
	// Create Threads
	for (int i = 0; i < numberOfThreads; i++) {
		//generateIslands(i);
		int ret = pthread_create(&tids[i], NULL, generateIslands, (void *)i); 
		if (ret != 0) {
			printf("Thread %d failed to create! Error code %d\n", i, ret);
			return 1;
		}
	}
	
	// Supervise
	int finishedLandmasses = 0;
	while (!finishedLandmasses) {
		int totalThreadProgress = 0;
		for (int i = 0; i < numberOfThreads; i++) {
			totalThreadProgress += threadProgress[i];
		}
		//printf("%d/%d\n", totalThreadProgress,(mapSizeX/landmassSize)*minSegment);
		
		updateProgressBar(scaleToValue(totalThreadProgress+1,mapSizeX/landmassSize,100),2);
		if (totalThreadProgress==mapSizeX/landmassSize-(numberOfThreads)) {
			finishedLandmasses = 1;
		}
	}
	
	// Rejoin them
	for (int i = 0; i < numberOfThreads; i++) {
		pthread_join(tids[i], NULL);
	}
	free(tids);
	printf("Island Generation\n");
	
	// Biomes based on landmass
	for (int biomeY = 0; biomeY < mapSizeY/biomeSize; biomeY++) {
		updateProgressBar(scaleToValue(biomeY,mapSizeY/biomeSize,100),3);
		//printf("%d\n",biomeY);
		for (int biomeX = 0; biomeX < mapSizeX/biomeSize; biomeX++) {
			int biome = emptybiome;
			int biomeXOffset = biomeSize/3 + getRandomLimited(biomeSize/2);
			int biomeYOffset = biomeSize/3 + getRandomLimited(biomeSize/2);
			//printf("%d,", checklandmass(biomeX*biomeSize,biomeY*biomeSize));	
			// Basic Post-processing
			if ((getRandomLimited(4) + getIntDistance(biomeX,0,biomeX,biomeY)) < ((mapSizeY/biomeSize)/6)) {
				biome = tundra;
			}
			if ((getRandomLimited(4) + getIntDistance(biomeX,mapSizeY/biomeSize,biomeX,biomeY)) < (((mapSizeY/biomeSize)/6))) {
				biome = tundra;
			}
			
			if ((biomeY + getRandomLimited(2) > ((mapSizeY/biomeSize)/7)*4)
				&& (biomeY - getRandomLimited(2) < ((mapSizeY/biomeSize)/7)*5)) {
				biome = desert;
				//changeHeightmapOverBiomeArea(biomeX,biomeY,-3);
			}
			
			// Checks to ensure adjance Biomes don't fuck up
			if (biome == emptybiome) {
				switch(checklandmass(biomeX*biomeSize,biomeY*biomeSize)) {
					case mainland:
						biome = grasslands;
						if (getRandomLimited(10)>7) {
							biome = mountains;
						} else {
							biome = forest;
						}
						break;
					case continent:
						biome = grasslands;
						if (heightMap[biomeX*biomeSize][biomeY*biomeSize] > heighestHeight/3) {
							//if (getRandomLimited(2)>1) {
								biome = mountains;
							//}
						}
						/* else {
						if (getRandomLimited(10)>9) {
							biome = mountains;
						}
						}*/
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
			biomeMap[biomeX][biomeY].biomeID = biome;
			biomeMap[biomeX][biomeY].biomeCenterOffsetX = biomeXOffset;
			biomeMap[biomeX][biomeY].biomeCenterOffsetY = biomeYOffset;
			if (visual & biomeBit) {
				renderBiome(biomeX,biomeY);
			}
		}
		//printf("\n");
	}
	SDL_RenderPresent(renderer);
	printf("Biome Generation\n");
	
	// Heightmap Processing
	normalizeHeightmap();
	addNoiseToHeightmap(20);
	blurHeightmap(3);
	addNoiseToHeightmap(3);
	blurHeightmap(1);
	addNoiseToHeightmap(1);
	printf("Heightmap Processing\n");
	
	// Prepare
	for (int mapY = 0; mapY < mapSizeY; mapY++) {
		if (mapY%10 == 0) {
			updateProgressBar(scaleToValue(mapY,mapSizeY,100),4);
		}
		for (int mapX = 0; mapX < mapSizeX; mapX++) {
			//printf("Started: %d,%d\n", mapX, mapY);
			int randomTile;
			// Get surrounding tiles
			// Unused right now
			int mapXminus = getWrappedAround(mapX-1,mapSizeX);
			int mapXplus = getWrappedAround(mapX+1,mapSizeX);
			int mapYminus = getWrappedAround(mapY-1,mapSizeY);
			int mapYplus = getWrappedAround(mapY+1,mapSizeY);
			surrounding[0] = map[mapXminus][mapY];
			surrounding[1] = map[mapXplus][mapY];
			surrounding[2] = map[mapX][mapYminus];
			surrounding[3] = map[mapX][mapYplus];
			
			// Actually put down tiles based on biomes
			// lower resolution biome selection
			//int vagueBiomeX = (int)((float)mapX*sin((float)mapY/(float)mapSizeY*PI));
			//int biome = biomeMap[vagueBiomeX/biomeSize][mapY/biomeSize];
			int biome = biomeMap[mapX/biomeSize][mapY/biomeSize].biomeID;
			
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
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 4);
			for (int yBiome = -1; yBiome <= 1; yBiome++) {
				for (int xBiome = -1; xBiome <= 1; xBiome++) {
					if ((xBiome == 0) && (yBiome == 0)) {
						// Own Biome
					} else {
						if (visual & biomeFinderBit) {
							SDL_RenderDrawLine(
								renderer,
								mapX,
								mapY,
								((biomeXpos+xBiome)*biomeSize)+biomeMap[mapX/biomeSize][mapY/biomeSize].biomeCenterOffsetX,
								((biomeYpos+yBiome)*biomeSize)+biomeMap[mapX/biomeSize][mapY/biomeSize].biomeCenterOffsetY
							);
						}
						
						// Get distance to next biome
						
						int distance = getIntDistance(
							mapX,
							mapY,
							(getWrappedAround(biomeXpos+xBiome,mapSizeX/biomeSize)*biomeSize)+biomeMap[mapX/biomeSize][mapY/biomeSize].biomeCenterOffsetX,
							(getWrappedAround(biomeYpos+yBiome,mapSizeY/biomeSize)*biomeSize)+biomeMap[mapX/biomeSize][mapY/biomeSize].biomeCenterOffsetY
						);
						distance+= getRandomLimited(biomeSize/2);
						// Check closest distance
						if (distance < closestDistance) {
							closestDistance = distance;
							int closestBiomeX = getWrappedAround(biomeXpos-(xBiome*-1),mapSizeX/biomeSize);
							int closestBiomeY = getWrappedAround(biomeYpos-(yBiome*-1),mapSizeY/biomeSize);
							closestBiome = biomeMap[closestBiomeX][closestBiomeY].biomeID;
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
			) > 3) {
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
			// This works because emptyTile == 0
			if (getTile(mapX,mapY)==emptyTile)	{
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
					case tundra:
						if (isNeighboring(sandTile)) {
							randomTile = grassTile;
						} else {
							randomTile = snowTile;
						}
						placeTile(mapX,mapY,randomTile);
						break;
				}
			} else {
				switch(biome) {
					case tundra:
						if (getTile(mapX,mapY) == oceanTile) {
							placeTile(mapX,mapY,iceTile);
						}
						break;
					default:
						placeTile(mapX,mapY,oceanTile);
						break;
				}
			}
		}
	}
	printf("Tile Placement\n");
	
	// Print Map
    char running = 1;
	if (visual & finalMapRender) {
		printMap();
		drawFinalMap();
	}
	
    SDL_RenderPresent(renderer);
    // Clear screen	
    SDL_Event event;
    while(running) {
        // Process events
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = 0;
            } else if(event.type == SDL_KEYDOWN) {
                const char *key = SDL_GetKeyName(event.key.keysym.sym);
				// Exit
                if(strcmp(key, "Q") == 0) {
                    running = 0;
                }    
				// Export Image
                if(strcmp(key, "W") == 0) {
					bmpMode = 1;
					saveBMP();
					bmpMode = 0;
                }     
				// Export Albedo
                if(strcmp(key, "A") == 0) {
					bmpMode = 1;
					albedoExport = 1;
					saveBMP();
					bmpMode = 0;
					albedoExport = 0;
                }     
				// Export Heightmap
                if(strcmp(key, "S") == 0) {
					bmpMode = 1;
					heightmapExport = 1;
					saveBMP();
					bmpMode = 0;
					heightmapExport = 0;
                }     
				// Rerender image
                if(strcmp(key, "E") == 0) {
                    printMap();
					drawFinalMap();
       				SDL_RenderPresent(renderer);
                }     
				// Render new image
                if(strcmp(key, "R") == 0) {
                    goto restart;
                }      
				// Rotate light
                if(strcmp(key, "T") == 0) {
					animate = 1;
					//bmpMode = 1;
					//saveBMP();
					//initialSeed+=1;
                    //goto restart;
                }                      				
            }
        }
		
		if (animate) {
			lightAngle += 0.1f;
			printMap();
			drawFinalMap();
			printf("Angle: %f\n", lightAngle);
        	SDL_RenderPresent(renderer);
			//SDL_Delay(100);
		}

        // Draw

        // Show what was drawn
    }
    pthread_exit(NULL); 
	return 0;
}