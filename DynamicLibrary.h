/**
 * @file DynamicLibrary.h
 * @brief This class implements a C++ wrapper to load shared libraries (.so)
 */

#pragma once

#include <exception>
#include <string>

class DynamicLibrary
{
public:
    DynamicLibrary(const char *fileName);
    ~DynamicLibrary();

    /**
     * @brief getFunctionPtr Resolve the symbol name
     * @param name The symbol name to resolve
     * @return The resolved function
     */
    void *getFunctionPtr(const char *name) const;

    DynamicLibrary(const DynamicLibrary &) = delete;
    DynamicLibrary & operator = (const DynamicLibrary &other) = delete;

private:
    void *libHandle;
};

class OpenLibFailedException: public std::exception
{
    virtual const char* what() const throw()
    {
        return "Failed to open Library";
    }
};

