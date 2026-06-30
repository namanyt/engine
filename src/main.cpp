#include "core/Log.h"

#include "runtime/EngineRuntime.h"
#include "runtime/RuntimeFactory.h"
#include "runtime/ExplorationRuntime.h"
#include "runtime/MenuRuntime.h"
#include "runtime/VNRuntime.h"

#include <cstdlib>
#include <exception>
#include <sstream>

namespace
{
void registerRuntimes()
{
    engine::RuntimeFactory::registerRuntime(engine::RuntimeId::Menu, []() {
        return std::make_unique<engine::MenuRuntime>();
    });
    
    engine::RuntimeFactory::registerRuntime(engine::RuntimeId::FoggyTestWorld, []() {
        return std::make_unique<engine::ExplorationRuntime>(engine::RuntimeId::FoggyTestWorld);
    });
    
    engine::RuntimeFactory::registerRuntime(engine::RuntimeId::DaylightSandbox, []() {
        return std::make_unique<engine::ExplorationRuntime>(engine::RuntimeId::DaylightSandbox);
    });
    
    engine::RuntimeFactory::registerRuntime(engine::RuntimeId::VNPrototype, []() {
        return std::make_unique<engine::VNRuntime>(
            std::filesystem::path{"scripts/test.vnscript"},
            engine::RuntimeId::DaylightSandbox);
    });
}
}

int main()
{
    registerRuntimes();
    
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
