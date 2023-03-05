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
#define mapSizeX 128
#define mapSizeY 64
int map[mapSizeX][mapSizeY] = {0};
int map2[mapSizeX][mapSizeY] = {0};

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
int id = 0;
int surrounding[4] = {0,0,0,0};
int invalidTile = 0;

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
			map2[x][y] = 5;
		}
	}
	
	// Seed the map
	srand ( time(NULL) );
	
	printf("\x1b[2J"); // Clear Screen
	printf("\x1b[H"); // Set Cursor to Home
	
	// Pregen
	int numberOfIslands = 20;
	for (int i = 1; i <= numberOfIslands; i++) {
		int islandSize = 3+getRandomLimited(16);
		int x1 = (int)(mapSizeX/(1+getRandomLimited(16)));
		int y1 = (int)(mapSizeY/(1+getRandomLimited(16)));
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				int x2 = x;
				int y2 = y;
				// Generate Water Circle
				if (getIntDistance(x1,y1,x2,y2) < islandSize+(getRandomLimited(5))) {
					map2[x2][y2] = 0;
				}
				// Generate Tree Circle
				if (getIntDistance(x1,y1,x2,y2) < (int)(islandSize/(1+getRandomLimited(5)))) {
					map2[x2][y2] = 1;
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
						surrounding[0] = 5;
					} else {
						surrounding[0] = map[x-1][y];
					}
					if (x+1 > mapSizeX) {
						surrounding[1] = 5;
					} else {
						surrounding[1] = map[x+1][y];
					}
					if (y-1 < 0) {
						surrounding[2] = 5;
					} else {
						surrounding[2] = map[x][y-1];
					}
					if (y+1 > mapSizeY) {
						surrounding[3] = 5;
					} else {
						surrounding[3] = map[x][y+1];
					}
					
					// TODO: SHOULD BE RANDOM WITH IDs IT KNOWS IT CAN PLACE, NOT JUST ANY RANDOM NUMBERS
					id = getRandomTileID();
					
					retry:
					invalidTile = 0;
					if (i != iterations) {
						switch (id) {
							case 1: // Tree
								if (((surrounding[0] == 0) || (surrounding[0] == 1) || (surrounding[0] == 2)) &&
									((surrounding[1] == 0) || (surrounding[1] == 1) || (surrounding[1] == 2)) &&
									((surrounding[2] == 0) || (surrounding[2] == 1) || (surrounding[2] == 2)) &&
									((surrounding[3] == 0) || (surrounding[3] == 1) || (surrounding[3] == 2))) {
									map2[x][y] = 1; // Place Tree
								} else {
									id = 1;
									invalidTile = 1;
								}
								break;
							case 2: // Bushes
								if (((surrounding[0] == 0) || (surrounding[0] == 1) || (surrounding[0] == 3)) &&
									((surrounding[1] == 0) || (surrounding[1] == 1) || (surrounding[1] == 3)) &&
									((surrounding[2] == 0) || (surrounding[2] == 1) || (surrounding[2] == 3)) &&
									((surrounding[3] == 0) || (surrounding[3] == 1) || (surrounding[3] == 3))) {
									map2[x][y] = 2; // Place Bush
								} else {
									id = 2;
									invalidTile = 2;
								}
								break;
							case 3: // Grass
								if (((surrounding[0] == 0) || (surrounding[0] == 2) || (surrounding[0] == 3) || (surrounding[0] == 4)) &&
									((surrounding[1] == 0) || (surrounding[1] == 2) || (surrounding[1] == 3) || (surrounding[1] == 4)) &&
									((surrounding[2] == 0) || (surrounding[2] == 2) || (surrounding[2] == 3) || (surrounding[2] == 4)) &&
									((surrounding[3] == 0) || (surrounding[3] == 2) || (surrounding[3] == 3) || (surrounding[3] == 4))) {
									map2[x][y] = 3; // Place Grass
								} else {
									id = 3;
									invalidTile = 3;
								}
								break;
							case 4: // Sand
								if (((surrounding[0] == 0) || (surrounding[0] == 3) || (surrounding[0] == 4) || (surrounding[0] == 5)) &&
									((surrounding[1] == 0) || (surrounding[1] == 3) || (surrounding[0] == 4) || (surrounding[0] == 5)) &&
									((surrounding[2] == 0) || (surrounding[2] == 3) || (surrounding[0] == 4) || (surrounding[0] == 5)) &&
									((surrounding[3] == 0) || (surrounding[3] == 3) || (surrounding[0] == 4) || (surrounding[0] == 5))) {
									map2[x][y] = 4; // Place Sand
								} else {
									id = 4;
									invalidTile = 4;
								}
								break;
							case 5: // Water
								if (((surrounding[0] == 0) || (surrounding[0] == 4) || (surrounding[0] == 5)) &&
									((surrounding[1] == 0) || (surrounding[1] == 4) || (surrounding[1] == 5)) &&
									((surrounding[2] == 0) || (surrounding[2] == 4) || (surrounding[2] == 5)) &&
									((surrounding[3] == 0) || (surrounding[3] == 4) || (surrounding[3] == 5))) {
									map2[x][y] = 5; // Place Water
								} else {
									id = 5;
									invalidTile = 5;
								}
								break;
							default: // Invalid Tile
								map2[x][y] = 6; // Place Error
								break;
						};
					
						if (invalidTile != 0) {
							id++;
							if (id > numTiles) {
								id = 0;
								goto skip;
							}
							goto retry;
						}
					} else {
						map2[x][y] = id;
					}
					
					skip:
					invalidTile = 0;
					memcpy(map, map2, sizeof(map));
				}
				printTile();
			}
			//printf("\n");
		}
	}
	
	// Print Map
	return 0;
}