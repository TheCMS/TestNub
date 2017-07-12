#include "TestLoader.h"
#include "stdlib.h"
#include "AutoTestHelperFuncs.h"


#define MAX_PATH 1024

TestLoader::TestLoader():
    m_bMainTestFound(false),
    m_bParseSuccesfull(false)
{

}

TestLoader::~TestLoader()
{

}

void TestLoader::setLoggingCb(autoTestLoggingCb cb)
{
    m_pLog=cb;
}

int TestLoader::loadTestsFromFile(const char* filePath)
{



#ifdef WIN32
    const char* rootPath;
#else
    char rootPath[MAX_PATH];
#endif
    // cms bad, to be reconsidered

    char* resolvedPath = realpath(filePath,rootPath);


    // get folder
    std::string fPath =std::string(resolvedPath);
    std::string folder(folderForFilePath(fPath));

    FILE* f =fopen(rootPath,"rb");

    unsigned int queueHead=0;
    if (f)
    {
        m_pLog(ALL_INFO,"TSTL","Parsing main test file %s.",filePath);
        m_filesQueue.push_back(resolvedPath);
        int rez = parseFile(f,folder.c_str(),resolvedPath,0,true);

        fclose(f);
        f=NULL;

        if (rez!=0)
        {
            m_pLog(ALL_ERROR,"TSTL","Error parsing file %s.Aborting.",filePath);
            return -2;
        }

        if (!m_bMainTestFound)
        {
            m_pLog(ALL_WARNING,"TSTL","No main test defined in main test file %s. Compilation and running tests won't be possible !",filePath);
        }
        queueHead++;

        while (m_filesQueue.size()>queueHead)
        {
            std::string curFile = m_filesQueue[queueHead];
            f = fopen(curFile.c_str(),"rb");
            if (!f)
            {
                m_pLog(ALL_ERROR,"TSTL","Error opening referenced test file %s.",curFile.c_str());
                printFileReference(queueHead);
                return -2;
            } else {

                m_pLog(ALL_INFO,"TSTL","Parsing file %s.",curFile.c_str());
                folder = folderForFilePath(curFile);

                int err = parseFile(f,folder.c_str(),curFile.c_str(),queueHead);
                if (err !=0)
                {
                    m_pLog(ALL_ERROR,"TSTL","Error parsing file.",curFile.c_str());
                    printFileReference(queueHead);
                    return -2;
                } else {
                    m_pLog(ALL_INFO,"TSTL","File parsed succesfully.");
                }
                fclose(f);
            }
            queueHead++;
        }

    } else {
        m_pLog(ALL_ERROR,"TSTL","Failed to open main test file %s.",filePath);
        return -1;
    }
    m_bParseSuccesfull=true;

    return 0;

}

void TestLoader::printFileReferenceToOutput(int idx, autoTestLoggingCb out)
{
    autoTestLoggingCb temp= m_pLog;
    if (out!=NULL)
    {
        m_pLog=out;
    }
    printFileReference(idx,true);
    if (out!=NULL)
    {
        m_pLog = temp;
    }
}

void TestLoader::printFileReference(int idx, bool bFirst)
{
    if (bFirst)
    {
        if (idx >= 0)
        {
            int refIdx = m_fileReferences[idx];
            if (refIdx >=0)
            {
                m_pLog(ALL_INFO,"TSTL","\n\nFile:\n %s",m_loadedFilesPath[idx].c_str());
                m_pLog(ALL_INFO,"TSTL","Included by\n%s",m_loadedFilesPath[refIdx].c_str());
                refIdx = m_fileReferences[refIdx];
                printFileReference(refIdx,false);
            }
        }
        m_pLog(ALL_INFO,"TSTL","\n");
    } else {
       if (idx >=0 )
       {
           m_pLog(ALL_INFO,"TSTL","Included by\n%s",m_loadedFilesPath[idx].c_str());
           printFileReference(m_fileReferences[idx],false);
       }
    }
}

int TestLoader::parseFile(FILE* f,const char* folderPath,const char* filePath,int curIdx, bool bMain)
{

    m_pLog(ALL_VERBOSE,"TSTL","received file realpath [%s] ",filePath);
    char* line=NULL;
    size_t len=0;
    ssize_t read;

    bool bParsingTest=false;
    bool bMainTest=false;

    test_unit cur;
    cur.fileName=std::string(filePath);
    if (bMain)
    {

        cur.refIdx=-1;
        m_fileReferences.push_back(-1);
        m_loadedFilesPath.push_back(filePath);
    } else {
        cur.refIdx=m_fileReferences[curIdx];
    }

    int lineCnt=0;

    test curTest;

    int ret=0;

    m_pLog(ALL_VERBOSE,"TSTL","Begin Parse ");
    while ((read = getline(&line,&len,f)) != -1 )
    {
        lineCnt++;
        std::string curLine(line);

        m_pLog(ALL_VERBOSE,"TSTL","Parsing line %s ",curLine.c_str());
        // trim line
        trim(curLine);

        if (curLine.length()<1) continue;

        if (curLine[0]=='#') continue;

        if (curLine.find("begin_main_test")!=std::string::npos)
        {
            if (!bMain)
            {
                m_pLog(ALL_INFO,"TSTL","Main test declaration from file %s will be ignored.",filePath);
            } else {
                bParsingTest=true;
                bMainTest=true;
            }
            curTest.resetInfo();
            curTest.test_name="main_test";
            curTest.package=cur.package_name;
            curTest.src_file=filePath;
            curTest.testLine = lineCnt;

        } else if (curLine.find("begin_test")!=std::string::npos)
        {
            std::string str = curLine;
            removeSequence(str,"begin_test");
            removeSequence(str,"(");
            removeSequence(str,")");
            removeSequence(str,"\"");


            if (bParsingTest)
            {
                m_pLog(ALL_ERROR,"TSTL","Malformed Test Unit. Test %s declared at line %d is inside test %d declared at line %d. Aborting.",str.c_str(),curLine.c_str(),
                       curTest.test_name.c_str(),curTest.testLine);
                free(line);
                return -1;
            }
            bParsingTest=true;

            curTest.resetInfo();
            curTest.test_name=str;
            curTest.package=cur.package_name;
            curTest.src_file=filePath;
            curTest.testLine = lineCnt;

        } else if (curLine.find("end_test")!=std::string::npos)
        {
            if (bMainTest)
            {
                m_entryPoint = curTest;
                m_bMainTestFound=true;
            } else {
                cur.tests.push_back(curTest);
            }
            bParsingTest=false;
            bMainTest=false;
        } else if (curLine.find("include")!=std::string::npos)
        {

            std::string str = curLine;
            removeSequence(str,"include");
            removeSequence(str,"(");
            removeSequence(str,")");
            removeSequence(str,"\"");

            if (str[0]=='/')
            {
                // absolute
                // do nothing

            } else {
                // relative
                str = std::string(folderPath).append(str);

#ifdef WIN32
                const char* buffer;
#else
                char buffer[1024];
#endif
                char* resolved = realpath(str.c_str(),buffer);
                if (resolved==NULL)
                {
                    // don't change str
                } else {
                    str = std::string(resolved);
                }
            }

            // validate if this file was ever added
            bool bFound=false;
            for (unsigned int i=0;i<m_loadedFilesPath.size();i++)
            {
                if (m_loadedFilesPath[i]==str)
                {
                    m_pLog(ALL_INFO,"TSTL","Included file %s skipped since it was already loaded due to previous reference."
                           ,str.c_str());
                    unsigned int ref =m_fileReferences[i];
                    if (ref < m_loadedFilesPath.size())
                    {
                        m_pLog(ALL_INFO,"TSTL","Place of previous reference: %s",m_loadedFilesPath[ref].c_str());
                    } else {
                        m_pLog(ALL_ERROR,"TSTL","Invalid reference or circular reference detected to %s!\nFile %s, at line %d:\n%s",
                               str.c_str(),cur.fileName.c_str(),lineCnt,curLine.c_str());
                        printFileReference(curIdx,true);
                    }

                    bFound=true;
                    break;
                }
            }
            if (!bFound)
            {
                m_loadedFilesPath.push_back(str);
                m_filesQueue.push_back(str);
                m_fileReferences.push_back(curIdx);
            }

        } else if (curLine.find("use_package")!=std::string::npos)
        {

            std::string str = curLine;
            removeSequence(str,"use_package");
            removeSequence(str,"(");
            removeSequence(str,")");
            removeSequence(str,"\"");

            cur.used_packages.push_back(str);

        } else if (curLine.find("package_name")!=std::string::npos)
        {
            std::string str = curLine;
            removeSequence(str,"package_name");
            removeSequence(str,"(");
            removeSequence(str,")");
            removeSequence(str,"\"");

            cur.package_name=str;

        } else if (curLine.find("use_plugin_grammar")!=std::string::npos)
        {
            std::string str = curLine;
            removeSequence(str,"use_plugin_grammar");
            removeSequence(str,"(");
            removeSequence(str,")");
            removeSequence(str,"\"");

            cur.used_grammars.push_back(str);
        } else {
            // start adding stuff inside
            if (curLine[0]=='#')
            {
                // comment
                // m_pLog(ALL_VERBOSE,"TSTL","Ignoring comment at line %d",lineCnt);
            } else {

                if (bParsingTest)
                {
                    curTest.actions.push_back(curLine);
                    curTest.action_fileLine.push_back(lineCnt);
                } else {
                    m_pLog(ALL_ERROR,"TSTL","Non comment declaration outside a test: %s found in file %s at line %d:\n%s\nAborting file parsing."
                           ,line,filePath,lineCnt,curLine.c_str());
                    ret =  -1;
                }

            }

        }
        free(line);
        line=NULL;
    }

    if (bParsingTest)
    {
        m_pLog(ALL_ERROR,"TSTL","Unfinished test declaration in test unit %s for test %s (starts at %d). ",filePath,curTest.test_name.c_str(),curTest.testLine);
        return -1;
    } else {

        if (ret == 0)
        {
            m_loadedUnits.push_back(cur);
        }

        if (bMain && !m_bMainTestFound)
        {
            m_pLog(ALL_ERROR,"TSTL","Main test entry point declaration wasn't found in test unit %s although it was declared main. Aborting test loading.",
                   filePath);
        }
        return ret;
    }


}

std::vector<test_unit>& TestLoader::getLoadedUnits()
{
    return m_loadedUnits;
}

test& TestLoader::getStartingTest()
{
    return m_entryPoint;
}

bool TestLoader::loadedTestsAreValid()
{
    return m_bMainTestFound && m_bParseSuccesfull;
}
