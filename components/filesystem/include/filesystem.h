const char *getFiles(char *filepath);
void filesystem_init();
const char *readFile(char *filepath);
void writeFile(char *filepath, char *file_data);
void editFile(char *filepath, char *json_key, char *json_key_data);
