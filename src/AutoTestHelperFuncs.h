#ifndef __AUTO_TEST_HELPER_FUNCS_H__
#define __AUTO_TEST_HELPER_FUNCS_H__

#include <string> 
#include <vector>

extern const char* ws ;

#ifdef WIN32
#define realpath(x,y) x

int getline(char** line, size_t* len, FILE* f);


#endif

// trim from end of string (right)
std::string& rtrim(std::string& s, const char* t = ws);

// trim from beginning of string (left)
std::string& ltrim(std::string& s, const char* t = ws);

// trim from both ends of string (left & right)
std::string& trim(std::string& s, const char* t = ws);

std::string& removeSequence(std::string& s, const char* sequence);

std::string folderForFilePath(std::string& path);

void splitString(std::string& str, std::string separator, std::vector<std::string>& out);

void splitOnce(std::string& str, std::string delimiter, std::vector<std::string>& out);

long getCurrentTimeMillis();

#endif
