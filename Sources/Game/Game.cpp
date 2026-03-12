#include <Termina/Scripting/API/ScriptingAPI.hpp>
#include <Termina/World/ComponentRegistry.hpp>

#include "MySimpleComponent.hpp"

COMPONENT_MODULE_BEGIN()
    REGISTER_COMPONENT(MySimpleComponent, "MySimpleComponent")
COMPONENT_MODULE_END()
