#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>

// Wave Function Collapse

// 0 Empty
// 1 Trees
// 2 Bushes
// 3 Grass
// 4 Sand
// 5 Lake
// 6 Water
// 7 Stone
#define mapSizeX 128
#define mapSizeY 128
#define biomeSize 8
#define macroBiomeSize 32
int map[mapSizeX][mapSizeY] = {0};
int biomeMap[mapSizeX/biomeSize][mapSizeY/biomeSize];
int macrobiomeMap[mapSizeX/macroBiomeSize][mapSizeY/macroBiomeSize];

typedef enum {
	emptymacrobiome,
	continent,
	ocean,
	islands,
	archipelago,
	numMacroBiomes,
} macrobiome;

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

int getRandomTilePos() {
	//return rand() % mapSize;
}

int getRandomTileID() {
	return rand() % numTiles;
}

int getRandomBiomeID() {
	return rand() % numBiomes;
}

int getRandomMacroBiomeID() {
	return rand() % numMacroBiomes;
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

int printTile() {
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
	return 0;
}

int printMap() {
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			printTile();
			printTile();
		}
		printf("\n");
	}
	printf("\x1b[0m");
}

void placeTile(int x, int y, int tile) {
	x = abs(x%mapSizeX);
	y = abs(y%mapSizeY);
	map[x][y] = tile;
}

int getIntDistance(int x1, int y1, int x2, int y2) {
	return (int) sqrt( (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) );  
}

int getRandomLimited(int max) {
	max = max+1;
	return rand() % max;
}

int getRandomLimitedMinMax(int min, int max) {
	max = max+1;
	int result = (rand() % max);
	
	if (result < min) {	result += min; }
	return result;
}

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

int isNeighboring(int tile) {
	if ((surrounding[0] == tile) || (surrounding[1] == tile) || (surrounding[2] == tile) || (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}
}

int canNeighbor(int tile) {
	if ((surrounding[0] == tile) || (surrounding[1] == tile) || (surrounding[2] == tile) || (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}
}

int surroundedBy(int tile) {
	if ((surrounding[0] == tile) && (surrounding[1] == tile) && (surrounding[2] == tile) && (surrounding[3] == tile)) {
		return 1;
	} else {
		return 0;
	}
}

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

void polygonIsland(int x1, int y1, int islandSize) {
	// Set number of points along circle
	int numberOfPoints = 20;
	float polygonX[20] = {0};
	float polygonY[20] = {0};

	float angleIncrement = 2 * M_PI / numberOfPoints;  // Calculate the angle increment
	float currentAngle = 0.0f;  // Starting angle

	// Set random displacements of points
	for (int pointID = 0; pointID < numberOfPoints; pointID++) {
		// Interpolate continent outline between points
		float x2 = cos(currentAngle) * (float)islandSize * getSmoothedRandomLimited(5, 10) + (float)x1;
		float y2 = sin(currentAngle) * (float)islandSize * getSmoothedRandomLimited(5, 10) + (float)y1;
		polygonX[pointID] = x2;
		polygonY[pointID] = y2;
		
		currentAngle += angleIncrement;  // Increment the angle for the next point
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
	}
}

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

int main() {
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			placeTile(x,y,oceanTile);
		}
	}
	// Seed the map
	srand ( time(NULL) );
	
	for (int i = 0; i < 20; i++) {
		previousRandomValues[i] = getRandomLimited(5);
	}
	
	
	printf("\x1b[2J"); // Clear Screen
	printf("\x1b[H"); // Set Cursor to Home
	
	// Pregen
	int numberOfIslands = 10;
	int islandBaseSize = 3;
	int maxRandomModifer = 16;
	// Macrobiomes
	for (int macroBiomeY = 0; macroBiomeY < mapSizeY/macroBiomeSize; macroBiomeY++) {
		for (int macroBiomeX = 0; macroBiomeX < mapSizeX/macroBiomeSize; macroBiomeX++) {
			int macrobiome = getRandomMacroBiomeID();
			switch(macrobiome) {
				case continent:
					printf("#");
					break;
				default:
					macrobiome = ocean;
				case ocean:
					printf("~");
					break;
				case islands:
					printf(":");
					break;
				case archipelago:
					printf("o");
					break;
			}
			macrobiome = macrobiomeMap[macroBiomeX][macroBiomeY];
		}
		printf("\n");
	}
	
	for (int macroBiomeY = 0; macroBiomeY < mapSizeY/macroBiomeSize; macroBiomeY++) {
		for (int macroBiomeX = 0; macroBiomeX < mapSizeX/macroBiomeSize; macroBiomeX++) {
			switch(macrobiomeMap[macroBiomeX][macroBiomeY]) {
				case continent:
					maxRandomModifer = 1;
					islandBaseSize = 1;
					numberOfIslands = 0;
					break;
				case archipelago:
					maxRandomModifer = 1;
					islandBaseSize = 1;
					numberOfIslands = 0;
					break;
				case islands:
					maxRandomModifer = 1;
					islandBaseSize = 1;
					numberOfIslands = 0;
					break;
				case ocean:
					maxRandomModifer = 1;
					islandBaseSize = 1;
					numberOfIslands = 0;
					break;
			}
				// Generate islands
			for (int i = 1; i <= numberOfIslands; i++) {
				int islandSize = islandBaseSize+getRandomLimited(maxRandomModifer);
				int x1 = (int)getRandomLimited(macroBiomeSize);//(mapSizeX/(1+getRandomLimited(16)));
				int y1 = (int)getRandomLimited(macroBiomeSize);//(mapSizeY/(1+getRandomLimited(16)));
				
				//randomIsland(x1,y1,islandSize);
				polygonIsland(x1,y1,islandSize);
				
				/*
				switch (islandtype) {
					case 0:
						if (islandSize > 10)
							randomIsland(x1,y1,islandSize);
						else
							islandtype++;
						break;
					case 1:
						polygonIsland(x1,y1,islandSize);
						break;
				}*/
			}
		}
	}
	
	// Initial Generate Map
	// Biomes based on water?
	// Rough Biome map
	for (int biomeY = 0; biomeY < mapSizeY/biomeSize; biomeY++) {
		for (int biomeX = 0; biomeX < mapSizeX/biomeSize; biomeX++) {
			int biome = getRandomBiomeID();
			switch(biome) {
				case desert:
					if (isNeighboringBiome(biomeX,biomeY,forest)) {
						biome = grasslands;
					}
				case forest:
					if (checkBiomeForTile(biomeX,biomeY,oceanTile)) {
					} else {
						biome = grasslands;
					}
					break;
				case beach:
					if (checkBiomeForTile(biomeX,biomeY,oceanTile)) {
					} else {
						biome = desert;
					}
					break;
			}
			biomeMap[biomeX][biomeY] = biome;
		}
	}
	
	/*
	for (int biomeY = 0; biomeY < mapSizeY/biomeSize; biomeY++) {
		for (int biomeX = 0; biomeX < mapSizeX/biomeSize; biomeX++) {
			switch(biomeMap[biomeX][biomeY]) {
				case desert:
					printf(".");
					break;
				case beach:
					printf("-");
					break;
				case grasslands:
					printf(",");
					break;
				case forest:
					printf("t");
					break;
				case mountains:
					printf("^");
					break;
				default:
					printf("#");
					break;
			}
		}
		printf("\n");
	}*/
	
	
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
				int biome = biomeMap[mapX>>3][mapY>>3];
				// Randomly get tiles from adjacent Biomes
				if (getRandomLimited(2)%2) {
					int neighborBiomes[4];
					for (int i = 0; i < 4; i++) {
						neighborBiomes[i] = getNeighboringBiomes(mapX/8,mapY/8,i);
					}
					biome = neighborBiomes[getRandomLimited(4)];
				}
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
						if (isNeighboring(stoneTile)) {
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
	}
	
	// Print Map
	printMap();
	return 0;
}