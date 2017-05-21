#pragma once

#include <vector>

struct Map {
	struct Spawn {
		char x;
		char y;
	} spawn[2];

	char tiles[16][9];


};

struct MapEntry {
	std::string name;
	Map map;
};

void LoadMaps(std::vector<MapEntry> &entries);

void SerializeMap(Map *map, std::string name);