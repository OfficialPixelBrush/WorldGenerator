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
int numTiles = 5;
#define mapSizeX 64
#define mapSizeY 32
int map[mapSizeX][mapSizeY] = {0};
int map2[mapSizeX][mapSizeY] = {0};

typedef enum {
	emptyTile,
	treeTile,
	bushTile,
	grassTile,
	sandTile,
	lakeTile,
	waterTile,
	stoneTile,
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
		case lakeTile: // Lake
			printf("\x1b[37;46m");
			printf("~");
			break;
		case waterTile: // Water
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
	int numberOfIslands = 5;
	for (int i = 1; i <= numberOfIslands; i++) {
		int islandSize = 3+getRandomLimited(16);
		int x1 = (int)getRandomLimited(mapSizeX);//(mapSizeX/(1+getRandomLimited(16)));
		int y1 = (int)getRandomLimited(mapSizeY);//(mapSizeY/(1+getRandomLimited(16)));
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				int x2 = x;
				int y2 = y;
				int distanceToIslandCenter = getIntDistance(x1,y1,x2,y2);
				// Generate Water Circle
				if (distanceToIslandCenter < islandSize+(getRandomLimited(5))) {
					map2[x2][y2] = emptyTile;
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
								if (surroundedBy(emptyTile) || surroundedBy(treeTile) || surroundedBy(bushTile)) {
									map2[x][y] = treeTile; // Place Tree
								} else {
									id = treeTile;
									invalidTile = treeTile;
								}
								break;
							case bushTile: // Bushes
								if (surroundedBy(emptyTile) || surroundedBy(bushTile) || surroundedBy(treeTile) || surroundedBy(grassTile)) {
									map2[x][y] = bushTile; // Place Bush
								} else {
									id = bushTile;
									invalidTile = bushTile;
								}
								break;
							case grassTile: // Grass
								if (surroundedBy(emptyTile) || surroundedBy(grassTile) || surroundedBy(bushTile) || surroundedBy(sandTile) || surroundedBy(lakeTile))  {
									map2[x][y] = grassTile; // Place Grass
								} else {
									id = grassTile;
									invalidTile = grassTile;
								}
								break;
							case sandTile: // Sand
								if (surroundedBy(emptyTile) || surroundedBy(grassTile) || surroundedBy(sandTile) || surroundedBy(waterTile)) {
									map2[x][y] = sandTile; // Place Sand
								} else {
									id = sandTile;
									invalidTile = sandTile;
								}
								break;
							case waterTile: // Water
								goto skip;
								break;
							case lakeTile: // Lake
								if (surroundedBy(emptyTile) || surroundedBy(waterTile) || surroundedBy(grassTile) || surroundedBy(lakeTile)) {
									map2[x][y] = lakeTile; // Place Lake
								} else {
									id = lakeTile;
									invalidTile = lakeTile;
								}
								break;
							default: // Invalid Tile
								map2[x][y] = errorTile; // Place Error
								break;
						};
					
						if (invalidTile != emptyTile) {
							id++;
							if (id > numTiles-1) {
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