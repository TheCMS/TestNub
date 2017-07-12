#ifndef __TEST_LOADER_H__
#define __TEST_LOADER_H__

#include <vector>
#include <stdio.h>

#include "AutoTestLogging.h"
#include "AutoTestTypes.h"



// parses test files , makes the test units and identifies the main tests
// validates includes, test imbrication and

class TestLoader
{
public:

        TestLoader();
        ~TestLoader();

        void    setLoggingCb(autoTestLoggingCb cb);
        int     loadTestsFromFile(const char* filePath);

        std::vector<test_unit>& getLoadedUnits();
        test& getStartingTest();

        bool    loadedTestsAreValid();

        void printFileReferenceToOutput(int idx, autoTestLoggingCb out=NULL);

private:

        int   parseFile(FILE* f, const char* folderPath,const char* filePath,int curIdx,bool bMain=false);

        void  printFileReference(int idx,bool bFirst=true);

        std::vector<test_unit> m_loadedUnits;
        std::vector<std::string> m_loadedFilesPath;
        std::vector<int> m_fileReferences;
        std::vector<std::string> m_filesQueue;

        autoTestLoggingCb m_pLog;

        bool m_bMainTestFound;
        bool m_bParseSuccesfull;

        test m_entryPoint;

};

#endif
