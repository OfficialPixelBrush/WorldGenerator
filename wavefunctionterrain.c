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
// 5 Water
int numTiles = 5;
#define mapSizeX 1024
#define mapSizeY 512
int map[mapSizeX][mapSizeY] = {0};
int map2[mapSizeX][mapSizeY] = {0};

typedef enum {
	emptyTile,
	treeTile,
	bushTile,
	grassTile,
	sandTile,
	waterTile,
	errorTile
} tilename;

int getRandomTilePos() {
	//return rand() % mapSize;
}

int getRandomTileID() {
	return rand() % numTiles +1;
}

//" "Empty can be anything 
// T Trees can be next to bushes and Trees
// m Bushes can be next to grass and Trees
// , Grass can be next to bushes and Grass
// # Sand can be next to Grass
// ~ Water can be next to Sand and Water

int x,y = 0;
int i = 0;
int iterations = 5;
int id = emptyTile;
int surrounding[4] = {emptyTile,emptyTile,emptyTile,emptyTile};
int invalidTile = emptyTile;

int printTile() {
	switch(map2[x][y]) {
		case 0: // Empty
			printf("\x1b[0m");
			printf(" ");
			break;
		case 1: // Tree
			printf("\x1b[92;42m");
			printf("T");
			break;
		case 2: // Bush
			printf("\x1b[32;102m");
			printf("m");
			break;
		case 3: // Grass
			printf("\x1b[32;102m");
			printf(",");
			break;
		case 4: // Sand
			printf("\x1b[33;103m");
			printf("#");
			break;
		case 5: // Water
			printf("\x1b[37;104m");
			printf("~");
			break;
		case 6: // Error
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
		}
		printf("\n");
	}
	printf("\x1b[0m");
}

int getIntDistance(int x1, int y1, int x2, int y2) {
	return (int) sqrt( (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) );  
}

int getRandomLimited(int max) {
	return rand() % max;
}

int main() {
	for (y = 0; y < mapSizeY; y++) {
		for (x = 0; x < mapSizeX; x++) {
			map2[x][y] = waterTile;
		}
	}
	
	// Seed the map
	srand ( time(NULL) );
	
	printf("\x1b[2J"); // Clear Screen
	printf("\x1b[H"); // Set Cursor to Home
	
	// Pregen
	int numberOfIslands = 1000;
	for (int i = 1; i <= numberOfIslands; i++) {
		int islandSize = 3+getRandomLimited(16);
		int x1 = (int)getRandomLimited(mapSizeX);//(mapSizeX/(1+getRandomLimited(16)));
		int y1 = (int)getRandomLimited(mapSizeY);//(mapSizeY/(1+getRandomLimited(16)));
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				int x2 = x;
				int y2 = y;
				// Generate Water Circle
				if (getIntDistance(x1,y1,x2,y2) < islandSize+(getRandomLimited(2))) {
					map2[x2][y2] = emptyTile;
				}
				// Generate Tree Circle
				if (getIntDistance(x1,y1,x2,y2) < (int)(islandSize/(1+getRandomLimited(5)))) {
					map2[x2][y2] = treeTile;
				}
			}
		}
	}
	printMap();
	
	// Initial Generate Map
	for (i = 1; i <= iterations; i++) {
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				printf("\x1b[%u;%uH",y+1,x+1);
				if (map[x][y] == 0) {
					if (x-1<0) {
						surrounding[0] = emptyTile;
					} else {
						surrounding[0] = map[x-1][y];
					}
					if (x+1 > mapSizeX) {
						surrounding[1] = emptyTile;
					} else {
						surrounding[1] = map[x+1][y];
					}
					if (y-1 < 0) {
						surrounding[2] = emptyTile;
					} else {
						surrounding[2] = map[x][y-1];
					}
					if (y+1 > mapSizeY) {
						surrounding[3] = emptyTile;
					} else {
						surrounding[3] = map[x][y+1];
					}
					
					// TODO: SHOULD BE RANDOM WITH IDs IT KNOWS IT CAN PLACE, NOT JUST ANY RANDOM NUMBERS
					id = getRandomTileID();
					
					retry:
					invalidTile = emptyTile;
					if (i != iterations) {
						switch (id) {
							case treeTile: // Tree
								if (((surrounding[0] == emptyTile) || (surrounding[0] == treeTile) || (surrounding[0] == bushTile)) &&
									((surrounding[1] == emptyTile) || (surrounding[1] == treeTile) || (surrounding[1] == bushTile)) &&
									((surrounding[2] == emptyTile) || (surrounding[2] == treeTile) || (surrounding[2] == bushTile)) &&
									((surrounding[3] == emptyTile) || (surrounding[3] == treeTile) || (surrounding[3] == bushTile))) {
									map2[x][y] = treeTile; // Place Tree
								} else {
									id = treeTile;
									invalidTile = treeTile;
								}
								break;
							case bushTile: // Bushes
								if (((surrounding[0] == emptyTile) || (surrounding[0] == bushTile) || (surrounding[0] == treeTile) || (surrounding[0] == grassTile)) &&
									((surrounding[1] == emptyTile) || (surrounding[1] == bushTile) || (surrounding[1] == treeTile) || (surrounding[1] == grassTile)) &&
									((surrounding[2] == emptyTile) || (surrounding[2] == bushTile) || (surrounding[2] == treeTile) || (surrounding[2] == grassTile)) &&
									((surrounding[3] == emptyTile) || (surrounding[3] == bushTile) || (surrounding[3] == treeTile) || (surrounding[3] == grassTile))) {
									map2[x][y] = bushTile; // Place Bush
								} else {
									id = bushTile;
									invalidTile = bushTile;
								}
								break;
							case grassTile: // Grass
								if (((surrounding[0] == emptyTile) || (surrounding[0] == bushTile) || (surrounding[0] == grassTile) || (surrounding[0] == sandTile)) &&
									((surrounding[1] == emptyTile) || (surrounding[1] == bushTile) || (surrounding[1] == grassTile) || (surrounding[1] == sandTile)) &&
									((surrounding[2] == emptyTile) || (surrounding[2] == bushTile) || (surrounding[2] == grassTile) || (surrounding[2] == sandTile)) &&
									((surrounding[3] == emptyTile) || (surrounding[3] == bushTile) || (surrounding[3] == grassTile) || (surrounding[3] == sandTile))) {
									map2[x][y] = grassTile; // Place Grass
								} else {
									id = grassTile;
									invalidTile = grassTile;
								}
								break;
							case sandTile: // Sand
								if (((surrounding[0] == emptyTile) || (surrounding[0] == grassTile) || (surrounding[0] == sandTile) || (surrounding[0] == waterTile)) &&
									((surrounding[1] == emptyTile) || (surrounding[1] == grassTile) || (surrounding[0] == sandTile) || (surrounding[0] == waterTile)) &&
									((surrounding[2] == emptyTile) || (surrounding[2] == grassTile) || (surrounding[0] == sandTile) || (surrounding[0] == waterTile)) &&
									((surrounding[3] == emptyTile) || (surrounding[3] == grassTile) || (surrounding[0] == sandTile) || (surrounding[0] == waterTile))) {
									map2[x][y] = sandTile; // Place Sand
								} else {
									id = sandTile;
									invalidTile = sandTile;
								}
								break;
							case waterTile: // Water
								if (((surrounding[0] == emptyTile) || (surrounding[0] == sandTile) || (surrounding[0] == waterTile)) &&
									((surrounding[1] == emptyTile) || (surrounding[1] == sandTile) || (surrounding[1] == waterTile)) &&
									((surrounding[2] == emptyTile) || (surrounding[2] == sandTile) || (surrounding[2] == waterTile)) &&
									((surrounding[3] == emptyTile) || (surrounding[3] == sandTile) || (surrounding[3] == waterTile))) {
									map2[x][y] = waterTile; // Place Water
								} else {
									id = waterTile;
									invalidTile = waterTile;
								}
								break;
							default: // Invalid Tile
								map2[x][y] = errorTile; // Place Error
								break;
						};
					
						if (invalidTile != emptyTile) {
							id++;
							if (id > numTiles) {
								id = emptyTile;
								goto skip;
							}
							goto retry;
						}
					} else {
						map2[x][y] = id;
					}
					
					skip:
					invalidTile = emptyTile;
					memcpy(map, map2, sizeof(map));
				}
				printTile();
			}
			//printf("\n");
		}
	}
	
	// Print Map
	//printMap();
	return 0;
}