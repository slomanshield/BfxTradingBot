#include "logger.h"

#if USING_LOGGER

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>

Logger* Logger::m_Instance = NULL;

static int scanpath(const char* path, const unsigned int maxNumFile);

Logger::Logger(std::string* logPath)
{
   //mkdir(logPath->c_str(),0777);


   m_LogLevel = DEFAULT_LOG_LEVEL;
   m_LogType  = DEFAULT_LOG_TYPE;

   m_LogPath = logPath->c_str();

   CreateLogDir();

   // scan path and delete oldest files if more than 10
   scanpath(logPath->c_str(), MAX_NUM_LOGFILE);
}

Logger::~Logger()
{
  
}

Logger* Logger::getInstance(std::string* logPath)
{
   if (NULL == m_Instance)
   {
      m_Instance = new Logger(logPath);
   }
   return m_Instance;
}

// DONOT pass in NULL  pointer for strLevel and logText
void Logger::log(const char* strLevel, const LogLevel level, const char* logText)
{
    if(m_LogLevel <= level)
    {
    	if(TYPE_FILE == m_LogType || TYPE_BOTH_FILECONSOLE == m_LogType)
    	{
			time_t now = time(0);
			struct tm* ptm = localtime(&now);
	#if 0 //enable this to test
			char buf[] = "YYYYMMDDHHMMSS.log";
			snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d.log", ptm->tm_year + 1900, ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min);
	#else
			char buf[] = "YYYYMMDD.log";
			snprintf(buf, sizeof(buf), "%04d%02d%02d.log", ptm->tm_year + 1900, ptm->tm_mon+1, ptm->tm_mday);
	#endif

			if(this->m_LogFileName.compare(buf) != 0)
			{
				this->m_LogFileName = buf;
			}

			// open and write to file
			m_Mutex.lock();
		    m_File.open(std::string(m_LogPath + "/" + this->m_LogFileName).c_str(), std::ios::out | std::ios::app);
            if(m_File.is_open())
            {
    		     m_File << getCurrentTime() << "  " << strLevel << logText << std::endl;
			     m_File.close();
            }

            // scan path and delete oldest files if more than 10
            scanpath(m_LogPath.c_str(), MAX_NUM_LOGFILE);
			m_Mutex.unlock();
		}

    	if(m_LogType == TYPE_CONSOLE || TYPE_BOTH_FILECONSOLE == m_LogType)
    	{
    		std::cout << getCurrentTime() << "  " << strLevel << logText << std::endl;
    	}
    }
}

// scan path and keep only maxNumFile(10) newest files
// return: number of files in the directory
static int scanpath(const char* path, const unsigned int maxNumFile)
{
   // scan path and delete files if more than 10
   	struct dirent ** namelist = NULL;
   	int numFiles = scandir(path, &namelist, NULL, alphasort);
	int n = numFiles;
   	int numFile2Delete = numFiles - maxNumFile;
   	if(numFile2Delete >= 0)
   	{
   		// delete the oldest files
   		for(int i = 0; (i < numFiles) && (0 != numFile2Delete); i++)
   		{
   			if(strcmp(namelist[i]->d_name, "." ) != 0 &&
   			   strcmp(namelist[i]->d_name, "..") != 0)
   			{
   				// delete the oldes file
   				std::string tmp(std::string(path) + "/" + namelist[i]->d_name);
   				std::remove(tmp.c_str());
   				numFile2Delete--;
   			}
   		}
   	}

	while (n--) {
		free(namelist[n]);
	}
	free(namelist);

   	return numFiles - numFile2Delete;
}

void Logger::CreateLogDir()
{
	std::string currentDir = "";
	int currIndex = m_LogPath.find("/");

	while (currIndex != m_LogPath.length() - 1)
	{
		currentDir = m_LogPath.substr(0, currIndex + 1);
		mkdir(currentDir.c_str(), 0777);
		currIndex++;
		currIndex = m_LogPath.find_first_of('/', currIndex);
		if (currIndex == -1)
		{
			currIndex = m_LogPath.length() - 1;
			mkdir(m_LogPath.c_str(), 0777);
		}
			
	}
	return;
}
#endif //USING_LOGGER
