#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>

// Wave Function Collapse

// 0 Empty
// 1 Trees
// 2 Bushes
// 3 Grass
// 4 Sand
// 5 Water
unsigned int numTiles = 5;
unsigned int mapSizeX = 64;
unsigned int mapSizeY = 32;
int map[64][32];
int map2[64][32];

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
int iterations = 1;
int id = 0;
int surrounding[4] = {0,0,0,0};
int invalidTile = 0;

int printTile() {
	switch(map[x][y]) {
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

int main() {
	// Seed the map
	srand ( time(NULL) );
	// Initial Generate Map
	printf("\x1b[2J"); // Clear Screen
	for (i = 0; i < iterations; i++) {
		for (y = 0; y < mapSizeY; y++) {
			for (x = 0; x < mapSizeX; x++) {
				printf("\x1b[%u;%uH",y+1,x+1);
				if (map[x][y] == 0) {
					surrounding[0] = 0;
					surrounding[1] = 0;
					surrounding[2] = 0;
					surrounding[3] = 0;
					if (map[x-1][y] != NULL) {
						surrounding[0] = map[x-1][y];
					}
					if (map[x+1][y] != NULL) {
						surrounding[1] = map[x+1][y];
					}
					if (map[x][y-1] != NULL) {
						surrounding[2] = map[x][y-1];
					}
					if (map[x][y+1] != NULL) {
						surrounding[3] = map[x][y+1];
					}
					
					// TODO: SHOULD BE RANDOM WITH IDs IT KNOWS IT CAN PLACE, NOT JUST ANY RANDOM NUMBERS
					id = getRandomTileID();
					
					retry:
					switch (id) {
						case 1: // Tree
							if (((surrounding[0] == 0) || (surrounding[0] == 1) || (surrounding[0] == 2)) &&
								((surrounding[1] == 0) || (surrounding[1] == 1) || (surrounding[1] == 2)) &&
								((surrounding[2] == 0) || (surrounding[2] == 1) || (surrounding[2] == 2)) &&
								((surrounding[3] == 0) || (surrounding[3] == 1) || (surrounding[3] == 2))) {
								map2[x][y] = 1; // Place Tree
							} else {
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
								invalidTile = 2;
							}
							break;
						case 3: // Grass
							if (((surrounding[0] == 0) || (surrounding[0] == 2) || (surrounding[0] == 3)) &&
								((surrounding[1] == 0) || (surrounding[1] == 2) || (surrounding[1] == 3)) &&
								((surrounding[2] == 0) || (surrounding[2] == 2) || (surrounding[2] == 3)) &&
								((surrounding[3] == 0) || (surrounding[3] == 2) || (surrounding[3] == 3))) {
								map2[x][y] = 3; // Place Grass
							} else {
								invalidTile = 3;
							}
							break;
						case 4: // Sand
							if (((surrounding[0] == 0) || (surrounding[0] == 3) || (surrounding[0] == 5)) &&
								((surrounding[1] == 0) || (surrounding[1] == 3) || (surrounding[1] == 5)) &&
								((surrounding[2] == 0) || (surrounding[2] == 3) || (surrounding[2] == 5)) &&
								((surrounding[3] == 0) || (surrounding[3] == 3) || (surrounding[3] == 5))) {
								map2[x][y] = 4; // Place Sand
							} else {
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
								invalidTile = 5;
							}
							break;
						default: // Invalid Tile
							map2[x][y] = 3; // Place idk
							invalidTile = 0;
							break;
					};
					
					if (invalidTile != 0) {
						if (id-1 < 0) {
							id = numTiles;
						} else {
							id--;
						}
						goto retry;
					}
					
					invalidTile = 0;
					memcpy(map, map2, sizeof(map));
					printTile();
				}
			}
			printf("\n");
		}
	}
	
	// Print Map
	return 0;
}