#include "core/Log.h"

#include "runtime/EngineRuntime.h"

#include <cstdlib>
#include <exception>
#include <sstream>

int main()
{
    try
    {
        engine::EngineRuntime runtime;
        return runtime.run();
    }
    catch (const std::exception& exception)
    {
        std::ostringstream stream;
        stream << "Fatal error: " << exception.what();
        engine::Log::error("Main", stream.str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
