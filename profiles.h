#pragma once
namespace Konnekt {
	extern CStdString profile;
	extern CStdString profileDir;
	extern CStdString profilesDir;
	extern bool newProfile;

	extern Stamina::MD5Digest passwordDigest;

	void initializeProfileDirectory();

	int setProfile(bool load = true , bool check = false);
	void setProfilesDir(void);
	int loadProfile(void);
	void profilePass ();
};