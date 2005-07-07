#pragma once
namespace Konnekt {
	extern CStdString profile;
	extern CStdString profileDir;
	extern CStdString profilesDir;
	extern bool newProfile;

	extern unsigned char md5digest [16];

	int setProfile(bool load = true , bool check = false);
	void setProfilesDir(void);
	int loadProfile(void);
	void profilePass ();
};