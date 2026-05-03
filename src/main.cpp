#include "Application.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        engine::Application application;
        application.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
