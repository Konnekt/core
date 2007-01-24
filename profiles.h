#pragma once
#include <Stamina/String.h>
using namespace Stamina;
namespace Konnekt {
	extern String profile;
	extern String profileDir;
	extern String profilesDir;
	extern bool newProfile;

	extern Stamina::MD5Digest passwordDigest;

	void initializeProfileDirectory();

	int setProfile(bool load = true , bool check = false);
	void setProfilesDir(void);
	int loadProfile(void);
	void profilePass ();
};