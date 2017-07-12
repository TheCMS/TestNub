#include "AutoTestHelperFuncs.h"

#include <string> 
#include <string.h>
#include <time.h>
#include <math.h>
#include "sys/time.h"

const char* ws = " \t\n\r\f\v";

#ifdef WIN32



int getline(char** line, size_t* len, FILE* f)
{
    (void)line;
    (void)len;
    (void)f;
    return -1;
}

#endif

// trim from end of string (right)
std::string& rtrim(std::string& s, const char* t)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
 std::string& ltrim(std::string& s, const char* t)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (left & right)
 std::string& trim(std::string& s, const char* t )
{
    return ltrim(rtrim(s, t), t);
}

 std::string& removeSequence(std::string& s, const char* sequence)
{
    bool bFinished=false;

    while (!bFinished)
    {
        std::string::size_type foundIdx = s.find(sequence,0);
        if (foundIdx!=std::string::npos)
        {
            s.erase(foundIdx,strlen(sequence));
        } else {
            bFinished=true;
        }
    }
    return s;

}

 std::string folderForFilePath(std::string& path)
{
    std::string::size_type folderIdx = path.find_last_of("/");

    std::string folder="./";
    if (folderIdx!=std::string::npos)
    {
        // keeping the "/";
        folder=path.substr(0,folderIdx+1);
    }
    return folder;
}


 void splitString(std::string& str, std::string separator, std::vector<std::string>& out)
{
    std::string::size_type splitIdx = str.find(separator);

    std::string temp = str;
    while (splitIdx!=std::string::npos)
    {
        std::string rez = temp.substr(0,splitIdx);
        if (rez.length()>0)
        {
            out.push_back(rez);
        }
        temp = temp.substr(splitIdx+separator.length());
        splitIdx = temp.find(separator);
    }
    out.push_back(temp);
}

 void splitOnce(std::string& str, std::string delimiter, std::vector<std::string>& out)
{

    std::string::size_type splitIdx = str.find(delimiter);

    if (splitIdx!=std::string::npos)
    {
        out.push_back(str.substr(0,splitIdx));
        out.push_back(str.substr(splitIdx+1));
    } else {
        out.push_back(str);
    }

}

 long getCurrentTimeMillis()
{

    long            ms; // Milliseconds

    struct timeval tm;

    gettimeofday(&tm,NULL);
    ms = tm.tv_sec*1000 + tm.tv_usec / 1000;
    return ms;
}
