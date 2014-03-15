/****************************************************************************
Copyright (c) 2013 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "Runtime.h"

#include "SimpleAudioEngine.h"
#include "jsb_cocos2dx_auto.hpp"
#include "jsb_cocos2dx_extension_auto.hpp"
#include "jsb_cocos2dx_builder_auto.hpp"
#include "extension/jsb_cocos2dx_extension_manual.h"
#include "cocosbuilder/js_bindings_ccbreader.h"
#include "localstorage/js_bindings_system_registration.h"
#include "chipmunk/js_bindings_chipmunk_registration.h"
#include "jsb_opengl_registration.h"
#include "cocos2d.h"

#ifdef _WIN32
#define realpath(dir,fuldir) _fullpath(fuldir,dir,_MAX_PATH_)
#include <direct.h>
#else
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#endif

#include <vector>
#include <string>

using namespace std;
using namespace cocos2d;

extern string getDotWaitFilePath();
extern string getProjSearchPath();
extern string getIPAddress();
extern vector<string> getSearchPath();
extern bool browseDir(const char *dir,const char *filespec,vector<string> &filterArray,vector<std::string> &fileList);


/*@brief   use "|" splite string  */
vector<string> splitFilter(const char *str)
{
	vector<string> filterArray;
	if (str)
	{
		char *token=NULL;
		char szFilterFile[_MAX_PATH_]={0};
		strcpy(szFilterFile,str);
		token = strtok(szFilterFile, "|" );		
		while( token != NULL )
		{
			filterArray.push_back(token);
			token = strtok( NULL, "|");
		}
	}
	return filterArray;
}

/*@brief wildcard funciton*/
bool wildcardMatches(const char *wildcard, const char *str) 
{
	while (1) {
		if (*wildcard == '\0')
		{
			return *str == '\0';
		}
		if (*wildcard == '?') 
		{
			++wildcard; ++str;
		} 
		else if (*wildcard == '*') 
		{
			for (++wildcard; *str; ++str)
			{
				if (wildcardMatches(wildcard, str))
				{
					return true;
				}
			}
			return *wildcard == '\0';
		} 
		else 
		{
			if (*wildcard != *str)
			{
				return false;
			}
			++wildcard; ++str;
		}
	}
}


#ifndef _WIN32
/*
*@brief iterator directory and process file.
*/
bool browseDir(const char *dir,const char *filespec,vector<string> &filterArray,vector<std::string> &fileList)
{
	DIR *dp=NULL;
	struct dirent *entry=NULL;
	struct stat statbuf;
	if((dp = opendir(dir)) == NULL) 
	{
		return false;
	}

	if (chdir(dir) != 0)
	{
		return false;
	}

	while((entry = readdir(dp)) != NULL) 
	{
		lstat(entry->d_name,&statbuf);
		if(S_ISDIR(statbuf.st_mode)) 
		{
			if(strcmp(".",entry->d_name) == 0 ||strcmp("..",entry->d_name) == 0)
			{
				continue;
			}

			if (find(filterArray.begin(),filterArray.end(),entry->d_name) != filterArray.end())
			{
				continue;
			}

			char subdir[_MAX_PATH_]={0};
			sprintf(subdir,"%s%s/",dir,entry->d_name);
			if (!browseDir(subdir,filespec,filterArray,fileList))
			{
				closedir(dp);
				return false;
			}
		} 
		else 
		{

			if (!wildcardMatches(filespec,entry->d_name))
			{
				continue;
			}

			char *pszexten=strrchr(entry->d_name,'.');
			char szextension[_MAX_PATH_]={0};
			if (pszexten)
			{
				strcpy(szextension,"*");
				strcat(szextension,pszexten);
				if (find(filterArray.begin(),filterArray.end(),szextension) != filterArray.end())
				{
					continue;
				}
			}

			strcpy(szextension,entry->d_name);
			if (find(filterArray.begin(),filterArray.end(),szextension) != filterArray.end())
			{
				continue;
			}

			char fullFileName[_MAX_PATH_] ={0};
			sprintf(fullFileName,"%s%s",dir,entry->d_name);
			fileList.push_back(fullFileName);
		}
	}
	chdir("..");
	closedir(dp);
	return true;
}
#endif


/************************
* Get file list from specified directory. 
*
*@param dir		    search directory
*@param filespec    search specified type file
*@param filterfile  filter file or folder
*
*Like this:
*    searchFileList("/home","*.*",".svn|.jpg|");  
*********************************/
vector<std::string> searchFileList(string &dir,const char *filespec="*.*",const char *filterfile=NULL)
{
	char fulldir[_MAX_PATH_]={0};
	vector<string> _filterArray;
	vector<std::string> _lfileList;
	_filterArray = splitFilter(filterfile);

	if (realpath(dir.c_str(), fulldir)== NULL)
	{
		return _lfileList;
	}

	int len=strlen(fulldir);
	if (fulldir[len-1] != '/')
	{
		strcat(fulldir,"/");
	}

	browseDir(fulldir,filespec,_filterArray,_lfileList);
	dir =fulldir;
	return _lfileList;
}

string getRuntimeVersion()
{
    return "0.0.1";
}

void startScript()
{
	ScriptEngineProtocol *engine = ScriptingCore::getInstance();
	ScriptEngineManager::getInstance()->setScriptEngine(engine);
	ScriptingCore::getInstance()->runScript("main.js");
}

void reloadScript(const string& file)
{
	string modulefile = file;
	if (modulefile.empty())
	{
		modulefile = "main.js";
	}
    auto core = ScriptingCore::getInstance();
    core->reset();
    core->runScript(modulefile.c_str());
}


class VisibleRect
{
public:
    static Rect getVisibleRect();
    
    static Point left();
    static Point right();
    static Point top();
    static Point bottom();
    static Point center();
    static Point leftTop();
    static Point rightTop();
    static Point leftBottom();
    static Point rightBottom();
private:
    static void lazyInit();
    static Rect s_visibleRect;
};

Rect VisibleRect::s_visibleRect;

void VisibleRect::lazyInit()
{
    // no lazy init
    // Useful if we change the resolution in runtime
    s_visibleRect = Director::getInstance()->getOpenGLView()->getVisibleRect();
}

Rect VisibleRect::getVisibleRect()
{
    lazyInit();
    return s_visibleRect;
}

Point VisibleRect::left()
{
    lazyInit();
    return Point(s_visibleRect.origin.x, s_visibleRect.origin.y+s_visibleRect.size.height/2);
}

Point VisibleRect::right()
{
    lazyInit();
    return Point(s_visibleRect.origin.x+s_visibleRect.size.width, s_visibleRect.origin.y+s_visibleRect.size.height/2);
}

Point VisibleRect::top()
{
    lazyInit();
    return Point(s_visibleRect.origin.x+s_visibleRect.size.width/2, s_visibleRect.origin.y+s_visibleRect.size.height);
}

Point VisibleRect::bottom()
{
    lazyInit();
    return Point(s_visibleRect.origin.x+s_visibleRect.size.width/2, s_visibleRect.origin.y);
}

Point VisibleRect::center()
{
    lazyInit();
    return Point(s_visibleRect.origin.x+s_visibleRect.size.width/2, s_visibleRect.origin.y+s_visibleRect.size.height/2);
}

Point VisibleRect::leftTop()
{
    lazyInit();
    return Point(s_visibleRect.origin.x, s_visibleRect.origin.y+s_visibleRect.size.height);
}

Point VisibleRect::rightTop()
{
    lazyInit();
    return Point(s_visibleRect.origin.x+s_visibleRect.size.width, s_visibleRect.origin.y+s_visibleRect.size.height);
}

Point VisibleRect::leftBottom()
{
    lazyInit();
    return s_visibleRect.origin;
}

Point VisibleRect::rightBottom()
{
    lazyInit();
    return Point(s_visibleRect.origin.x+s_visibleRect.size.width, s_visibleRect.origin.y);
}

class ConnectWaitLayer: public Layer
{
public:

	ConnectWaitLayer()
	{
        string strip = getIPAddress();
        char szIPAddress[512]={0};
        sprintf(szIPAddress, "LocalIP: %s",strip.c_str());
        auto label = LabelTTF::create(szIPAddress, "Arial", 24);
        addChild(label, 9999);
        label->setPosition( Point(VisibleRect::center().x, VisibleRect::top().y - 30) );
        
        auto labelwait = LabelTTF::create("wait transfer files ...", "Arial", 22);
        addChild(labelwait, 10000);
        labelwait->setPosition( Point(VisibleRect::center().x, VisibleRect::center().y) );
        
        
        auto labelPlay = LabelTTF::create("play", "Arial", 20);
        auto menuItem = MenuItemLabel::create(labelPlay, CC_CALLBACK_1(ConnectWaitLayer::playerCallback, this));
        auto menu = Menu::create(menuItem, NULL);
        
        menu->setPosition( Point::ZERO );
        menuItem->setPosition( Point( VisibleRect::right().x - 50, VisibleRect::bottom().y + 25) );
        addChild(menu, 1);
		//_scheduler = CCDirector::sharedDirector()->getScheduler();
        //scheduleUpdate();
	}
    
    void playerCallback(Object* sender)
    {
        startScript();
    }

};


#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include <WS2tcpip.h>

#define bzero(a, b) memset(a, 0, b);

#else
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

class  FileServer
{
public:
    
    FileServer()
    {
        _listenfd = -1;
        _running = false;
        _endThread = false;
		_writepath = FileUtils::getInstance()->getWritablePath();
    }
    
	bool listenOnTCP(int port);
    void stop();
    
private:
    bool recv_file(int fd);
    void addClient();
    void loop();
 
    // file descriptor: socket, console, etc.
    int _listenfd;
    int _maxfd;
    std::vector<int> _fds;
    std::thread _thread;
    fd_set _read_set;
    bool _running;
    bool _endThread;
	std::string _writepath;
};

bool FileServer::listenOnTCP(int port)
{
	int listenfd, n;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;
	char serv[30];

	snprintf(serv, sizeof(serv)-1, "%d", port );
	serv[sizeof(serv)-1]=0;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET; // AF_UNSPEC: Do we need IPv6 ?
	hints.ai_socktype = SOCK_STREAM;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	WSADATA wsaData;
	n = WSAStartup(MAKEWORD(2, 2),&wsaData);
#endif

	if ( (n = getaddrinfo(NULL, serv, &hints, &res)) != 0) {
		fprintf(stderr,"net_listen error for %s: %s", serv, gai_strerror(n));
		return false;
	}

	ressave = res;
	do {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;       /* error, try next one */
		
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
		if (::bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;          /* success */
		
		close(listenfd);    /* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
		perror("net_listen:");
		freeaddrinfo(ressave);
		return false;
	}

	listen(listenfd, 1);

	if (res->ai_family == AF_INET) 
	{
		char buf[INET_ADDRSTRLEN] = "";
		struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
		if( inet_ntop(res->ai_family, &sin->sin_addr, buf, sizeof(buf)) != NULL )
			cocos2d::log("Console: listening on  %s : %d", buf, ntohs(sin->sin_port));
		else
			perror("inet_ntop");
	} else if (res->ai_family == AF_INET6) 
	{
		char buf[INET6_ADDRSTRLEN] = "";
		struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
		if( inet_ntop(res->ai_family, &sin->sin6_addr, buf, sizeof(buf)) != NULL )
			cocos2d::log("Console: listening on  %s : %d", buf, ntohs(sin->sin6_port));
		else
			perror("inet_ntop");
	}
	freeaddrinfo(ressave);
	_listenfd = listenfd;
	_thread = std::thread( std::bind( &FileServer::loop, this) );
	return true;	
}
	
void FileServer::stop()
{
	if( _running ) {
		_endThread = true;
		_thread.join();
	}
}

string& replace_all(string& str,const string& old_value,const string& new_value)
{
	while(true)
	{
		int pos=0;
		if((pos=str.find(old_value,0))!=string::npos)
			str.replace(pos,old_value.length(),new_value);
		else break;
	}
	return str;
}

bool CreateDir(const char *sPathName)
{
	char   DirName[256]={0};
	strcpy(DirName,   sPathName);
	int   i,len   =   strlen(DirName);
	if(DirName[len-1]!='/')
		strcat(DirName,   "/");
    
	len   =   strlen(DirName);
	for(i=1;   i<len;   i++)
	{
		if(DirName[i]=='/')
		{
			DirName[i]   =   0;
			if(access(DirName,   NULL)!=0   )
			{
#ifdef _WIN32
				if(mkdir(DirName/*,   0755*/)==-1)
#else
                if(mkdir(DirName,   0755)==-1)
#endif
                {
                    perror("mkdir   error");
                    return  false;
                }
			}  
			DirName[i]   =   '/';  
		}  
	}  
    
	return   true;  
}

bool FileServer::recv_file(int fd)
{
	char buffer[1024]={0};
    char namelen[4]={0};
	if (recv(fd, namelen, 4,0)<=0) {
		return  false;
	}
    
	if (recv(fd, buffer, atoi(namelen),0)<=0) {
		return  false;
	}
    
    char fullfilename[1024]={0};
	sprintf(fullfilename,"%s%s",_writepath.c_str(),buffer);
    string file(fullfilename);
	file=replace_all(file,"\\","/");
	sprintf(fullfilename, "%s", file.c_str());
	cocos2d::log("recv fullfilename = %s",fullfilename);
    CreateDir(file.substr(0,file.find_last_of("/")).c_str());
	FILE *fp =fopen(fullfilename, "wb");
	
	int length =0;
	while ((length=recv(fd, fullfilename, sizeof(fullfilename),0)) > 0) {
		fwrite(fullfilename, sizeof(char), length,fp);
	}
	fclose(fp);
	return true;
}
    
void FileServer::addClient()
{
	struct sockaddr client;
	socklen_t client_len;

	/* new client */
	client_len = sizeof( client );
	int fd = accept(_listenfd, (struct sockaddr *)&client, &client_len );

	// add fd to list of FD
	if( fd != -1 ) {
		FD_SET(fd, &_read_set);
		_fds.push_back(fd);
		_maxfd = std::max(_maxfd,fd);
	}
}

void FileServer::loop()
{
	fd_set copy_set;
	struct timeval timeout, timeout_copy;

	_running = true;

	FD_ZERO(&_read_set);
	FD_SET(_listenfd, &_read_set);
	_maxfd = _listenfd;

	timeout.tv_sec = 0;

	/* 0.016 seconds. Wake up once per frame at 60PFS */
	timeout.tv_usec = 16000;

	while(!_endThread) {

		copy_set = _read_set;
		timeout_copy = timeout;
		int nready = select(_maxfd+1, &copy_set, NULL, NULL, &timeout_copy);

		if( nready == -1 )
		{
			/* error */
			if(errno != EINTR)
				log("Abnormal error in select()\n");
			continue;
		}
		else if( nready == 0 )
		{
			/* timeout. do somethig ? */
		}
		else
		{
			/* new client */
			if(FD_ISSET(_listenfd, &copy_set)) {
				addClient();
				if(--nready <= 0)
					continue;
			}

			/* data from client */
			std::vector<int> to_remove;
			for(const auto &fd: _fds) {
				if(FD_ISSET(fd,&copy_set)) {
					if( ! recv_file(fd) ) {
						to_remove.push_back(fd);
					}
					if(--nready <= 0)
						break;
				}
			}

			/* remove closed conections */
			for(int fd: to_remove) {
				FD_CLR(fd, &_read_set);
				_fds.erase(std::remove(_fds.begin(), _fds.end(), fd), _fds.end());
			}
		}
	}

	// clean up: ignore stdin, stdout and stderr
	for(const auto &fd: _fds )
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		closesocket(fd);
#else
		close(fd);
#endif
	}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	closesocket(_listenfd);
	WSACleanup();
#else
	close(_listenfd);
#endif

	_running = false;
}


class ConsoleCustomCommand
{
public:
    ConsoleCustomCommand():_fileserver(nullptr)
    {
        _writepath = FileUtils::getInstance()->getWritablePath();
        cocos2d::Console *_console = Director::getInstance()->getConsole();
        static struct Console::Command commands[] = {
            {"shutdownapp","exit runtime app",std::bind(&ConsoleCustomCommand::onShutDownApp, this, std::placeholders::_1, std::placeholders::_2)},
            {"precompile","",std::bind(&ConsoleCustomCommand::onPreCompile, this, std::placeholders::_1, std::placeholders::_2)},
            {"start-logic","run game logic script",std::bind(&ConsoleCustomCommand::onRunLogicScript, this, std::placeholders::_1, std::placeholders::_2)},
            {"reload","reload script.Args:[filepath]",std::bind(&ConsoleCustomCommand::onReloadScriptFile, this, std::placeholders::_1, std::placeholders::_2)},
            {"getversion","get runtime version.",std::bind(&ConsoleCustomCommand::onRuntimeVersion, this, std::placeholders::_1, std::placeholders::_2)},
        };
        for (int i=0;i< sizeof(commands)/sizeof(Console::Command);i++) {
            _console->addCommand(commands[i]);
        }
        _console->listenOnTCP(6050);
        
        _fileserver=new FileServer();
        _fileserver->listenOnTCP(6060);
    }
    ~ConsoleCustomCommand()
    {
		Director::getInstance()->getConsole()->stop();
        _fileserver->stop();
        if (_fileserver) {
            delete _fileserver;
            _fileserver = nullptr;
        }
    }
    
    void onPreCompile(int fd, const std::string &args)
    {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([=](){
            vector<std::string> fileInfoList = searchFileList(_writepath,"*.js","runtime|frameworks|");
            for (unsigned i = 0; i < fileInfoList.size(); i++)
            {
                ScriptingCore::getInstance()->compileScript(fileInfoList[i].substr(_writepath.length(),-1).c_str());
            }
        });
    }
    
    void onRunLogicScript(int fd, const std::string &args)
    {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([](){
            startScript();
        });
    }
    
    void onReloadScriptFile(int fd,const std::string &args)
    {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([=](){
             reloadScript(args.c_str());
        });
    }
    
	void onRuntimeVersion(int fd, const std::string &args)
    {
        string runtimeVer=getRuntimeVersion();
		runtimeVer += "\n";
        send(fd, runtimeVer.c_str(), runtimeVer.size(),0);
    }
	
    void onShutDownApp(int fd, const std::string &args)
    {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([](){
            exit(0);
        });
    }
private:
    FileServer* _fileserver;
    string _writepath;
};

void startRuntime()
{
    static ConsoleCustomCommand s_customCommand;
	vector<string> searchPathArray;
    searchPathArray=FileUtils::getInstance()->getSearchPaths();
    vector<string> writePathArray;
    writePathArray.push_back(FileUtils::getInstance()->getWritablePath());
    FileUtils::getInstance()->setSearchPaths(writePathArray);
	for (unsigned i = 0; i < searchPathArray.size(); i++)
	{
		FileUtils::getInstance()->addSearchPath(searchPathArray[i]);
	}
    ScriptingCore::getInstance()->start();
	ScriptingCore::getInstance()->enableDebugger();
	ScriptEngineProtocol *engine = ScriptingCore::getInstance();
	ScriptEngineManager::getInstance()->setScriptEngine(engine);

    auto scene = Scene::create();
    auto layer = new ConnectWaitLayer();
    layer->autorelease();
    auto director = Director::getInstance();
    scene->addChild(layer);
    director->runWithScene(scene);

}


// SimulatorConfig
SimulatorConfig *SimulatorConfig::s_sharedInstance = NULL;
SimulatorConfig *SimulatorConfig::getInstance(void)
{
	if (!s_sharedInstance)
	{
		s_sharedInstance = new SimulatorConfig();
	}
	return s_sharedInstance;
}

SimulatorConfig::SimulatorConfig(void)
{
	m_screenSizeArray.push_back(SimulatorScreenSize("iPhone 3Gs (480x320)", 480, 320));
	m_screenSizeArray.push_back(SimulatorScreenSize("iPhone 4 (960x640)", 960, 640));
	m_screenSizeArray.push_back(SimulatorScreenSize("iPhone 5 (1136x640)", 1136, 640));
	m_screenSizeArray.push_back(SimulatorScreenSize("iPad (1024x768)", 1024, 768));
	m_screenSizeArray.push_back(SimulatorScreenSize("iPad Retina (2048x1536)", 2048, 1536));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (800x480)", 800, 480));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (854x480)", 854, 480));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (960x540)", 960, 540));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (1024x600)", 1024, 600));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (1280x720)", 1280, 720));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (1280x800)", 1280, 800));
	m_screenSizeArray.push_back(SimulatorScreenSize("Android (1920x1080)", 1920, 1080));
}

int SimulatorConfig::getScreenSizeCount(void)
{
	return (int)m_screenSizeArray.size();
}

const SimulatorScreenSize SimulatorConfig::getScreenSize(int index)
{
	return m_screenSizeArray.at(index);
}