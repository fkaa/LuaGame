#include "MapFormat.h"

#include <windows.h>
//#include <filesystem>

void LoadMaps(std::vector<MapEntry> &entries)
{
	WIN32_FIND_DATAA search_data;
	memset(&search_data, 0, sizeof(WIN32_FIND_DATAA));

	HANDLE handle = FindFirstFileA("Assets\\Maps\\*", &search_data);

	while (handle != INVALID_HANDLE_VALUE) {
		auto path = std::string("Assets\\Maps\\") + search_data.cFileName;
		FILE *f = fopen(path.c_str(), "r");
		if (f) {
			Map m = {};
			fread(&m, sizeof(Map), 1, f);
			fclose(f);

			entries.push_back({ std::string(search_data.cFileName), m });
		}

		if (FindNextFileA(handle, &search_data) == FALSE)
			break;
	}

	FindClose(handle);
}

void SerializeMap(Map *map, std::string name)
{
	auto ext = "Assets\\Maps\\" + name;
	FILE *f = fopen(ext.c_str(), "w+");
	if (f) {
		fwrite(map, sizeof(Map), 1, f);
		fclose(f);
	}
}
