#include "stdafx.h"
#include "tools.h"

using namespace Stamina;

int Konnekt::findVersion(string & ver , string fileName , int version) {
    int found = ver.find("\n" + fileName + "=");
    int prevVer = 0;
    if (found!=-1) {
        found = ver.find('=' , found)+1;
        prevVer = chtoint(ver.c_str() + found , 16);
        if (prevVer != version) {
            ver.erase(ver.begin() + found , ver.begin() + ver.find('\n' , found));
            ver.insert(found , inttostr(version , 16));
        }
    } else {
        ver+="\n" + fileName + "=" + inttostr(version , 16);
    }
    return prevVer;
}
