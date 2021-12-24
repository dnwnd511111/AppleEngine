#ifndef APPLEENGINE_VERSION_DEFINED
#define APPLEENGINE_VERSION_DEFINED

namespace ap::version
{
	
	int GetMajor();
	// minor features, major bug fixes
	int GetMinor();
	// minor bug fixes, alterations
	int GetRevision();
	const char* GetVersionString();
}

#endif // APPLEENGINE_VERSION_DEFINED
