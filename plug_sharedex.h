#ifndef PLUGSHAREDEX_H
#define PLUGSHAREDEX_H

#pragma pack(push, 1)

namespace Konnekt {


     struct sPlugInfo {
        char file [101];
        char version [51];
        char company [101];
        char name [201];
        char info [256];
        char url  [256];
        int load;
        bool isNew;
     };

     // Dodatkowe , specjalne f-cje sterujace .. Adres tej struktury podawany jest
     // tylko do ui.dll
     class cCtrlEx : public cCtrl {
        public:
        virtual void b0(void){};
        virtual void b1(void){};
        virtual void b2(void){};
        virtual void b3(void){};
        virtual void b4(void){};
        virtual void b5(void){};
        virtual void b6(void){};
        virtual void b7(void){};
        virtual void b8(void){};
        virtual void b9(void){};
        virtual void bA(void){};
        virtual void bB(void){};
        //virtual void bC(void){};
        //virtual void bD(void){};
        //virtual void bE(void){};
        virtual void __stdcall PlugOut(unsigned int id , const char * reason , bool restart = 1){};
              ///< Wy³¹cza wtyczkê (i program , jeœli restart==1 , pyta czy restartowac)
     };


struct exception_plug {
    const char * msg;
    tPluginId id;
    unsigned int severity;
    exception_plug(const char * msg , tPluginId id , unsigned int severity):msg(msg),id(id),severity(severity) {}
};
#ifdef _STRING_
struct exception_plug2 {
    std::string msg;
    tPluginId id;
    unsigned int severity;
    exception_plug2(std::string msg , tPluginId id , unsigned int severity):msg(msg),id(id),severity(severity) {}
};
#endif


#define CNT_INTERNAL_adding 1

}

#pragma pack(pop)

#endif