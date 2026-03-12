#include "LaunchProcess.hpp"
#include "Core/Logger.hpp"

#if !defined(TRMN_WINDOWS)

namespace Termina {
    void LaunchProcess::Launch(const std::string& executable, const std::vector<std::string>& arguments)
    {
        // Build the command line
        std::string command = executable;
        for (const auto& arg : arguments) {
            command += " " + arg;
        }

        // Use system() to launch the process
        int result = system(command.c_str());
        if (result != 0) {
            TN_ERROR("Failed to launch process: %s", command.c_str());
        }
    }
}

#endif
