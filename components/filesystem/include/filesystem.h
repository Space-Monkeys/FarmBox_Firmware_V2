const char *getFiles(char *filepath);
void filesystem_init();
char *readFile(char *filepath);
int writeFile(char *filepath, char *file_data);
void editFile(char *filepath, char *json_key, char *json_key_data);
