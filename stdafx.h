#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#include <sys/types.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <locale>
#include <stdstring.h>
#include <deque>
#include <vector>
#include <stack>
#include <map>
#include <algorithm>
#include <dos.h>
#include <process.h>
#include <io.h>
#include <ras.h>
#include <stdio.h>
#include <stdexcept>
#include <direct.h>
#include <Wininet.h>
#include <md5.h>
#include <Shlobj.h>
#include <Richedit.h>
#include <tchar.h>
#include <math.h>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <stamina/VersionControl.h>
#include <stamina/ObjectImpl.h>
#include <stamina/Semaphore.h>
#include <stamina/Thread.h>
#include <stamina/ThreadInvoke.h>
#include <stamina/ThreadRun.h>
#include <stamina/RegEx.h>
#include <stamina/Helpers.h>
#include <stamina/WinHelper.h>
#include <Stamina\ButtonX.h>
#include <Stamina\DataTable\DataTable.h>
#include <Stamina\DataTable\FileBin.h>
#include <Stamina\FindFileFiltered.h>
#include <Stamina\FileResource.h>
#include <Stamina\Timer.h>
#include <Stamina\Time64.h>
#include <Stamina\Image.h>


//#include "include\msc_compat.h"
//#include "include\critical_section.h"
//#include "include\messagebox.h"

//#include "include\func.h"
//#include "include\preg.h"
#include "include\callstack.h"       
//#include "include\dbtable.h"
//#include "include\dtablebin.h"
//#include "include\time64.h"
//#include "include\win_registry.h"
//#include "include\richhtml.h"


using namespace std;