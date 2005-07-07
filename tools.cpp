#include "stdafx.h"
#include "tools.h"

int Konnekt::findVersion(string & ver , string fileName , int version) {
    int found = ver.find("\n" + fileName + "=");
    int prevVer = 0;
    if (found!=-1) {
        found = ver.find('=' , found)+1;
        prevVer = chtoint(ver.c_str() + found , 16);
        if (prevVer != version) {
            ver.erase(ver.begin() + found , ver.begin() + ver.find('\n' , found));
            ver.insert(found , inttoch(version , 16));
        }
    } else {
        ver+="\n" + fileName + "=" + string(inttoch(version , 16));
    }
    return prevVer;
}
