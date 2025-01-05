#pragma once

#include <string>
#include <stdexcept>
#include <filesystem>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

class ResourcePath
{
public:
    static std::string getResourcesDirectory()
    {
#ifdef __APPLE__
        return getMacOSResourcePath();
#elif _WIN32
        return getWindowsResourcePath();
#else
        return getLinuxResourcePath();
#endif
    }

private:
#ifdef __APPLE__
    static std::string getMacOSResourcePath()
    {
        char path[PATH_MAX];
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (!mainBundle)
        {
            throw std::runtime_error("Failed to get main bundle.");
        }

        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        if (!CFURLGetFileSystemRepresentation(resourcesURL, true, reinterpret_cast<UInt8 *>(path), PATH_MAX))
        {
            CFRelease(resourcesURL);
            throw std::runtime_error("Failed to resolve resource path.");
        }
        CFRelease(resourcesURL);
        return std::string(path);
    }
#endif

#ifdef _WIN32
    static std::string getWindowsResourcePath()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string exePath = std::string(path);
        size_t lastSlash = exePath.find_last_of("\\/");
        return exePath.substr(0, lastSlash) + "\\Resources";
    }
#endif

#ifdef __linux__
    static std::string getLinuxResourcePath()
    {
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        if (count == -1)
        {
            throw std::runtime_error("Failed to resolve executable path.");
        }
        std::string exePath = std::string(path, count);
        size_t lastSlash = exePath.find_last_of("/");
        return exePath.substr(0, lastSlash) + "/Resources";
    }
#endif
};