#pragma once

#include <vector>
#include <string>

namespace Termina {
    class LaunchProcess
    {
    public:
        static void Launch(const std::string& executable, const std::vector<std::string>& arguments);
    };
}
