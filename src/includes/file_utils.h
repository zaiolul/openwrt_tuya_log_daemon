#ifndef FILE_UTILS_H
#define FILE_UTILS_H


int check_file_ending(char *filename, char *expected_ending);
int get_string_parts(const char *str, char *sep, char ***arr);
int free_string_array(char ***arr, int count);
#endif