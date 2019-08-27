#define		MAX_STRING_LENGTH	100
#define		FILE_OPEN_ERROR		0
#define		OK					1

int read_map_file();
int write_map_file();
int fgetline(FILE* infile, char* buffer);
int proccess_ini_file();
int file_exist(const char *file);
