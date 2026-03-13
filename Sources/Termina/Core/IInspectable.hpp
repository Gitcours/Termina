#pragma once

namespace Termina {

    class IInspectable
    {
    public:
        virtual ~IInspectable() = default;
        virtual void Inspect() = 0;
        virtual void OnGizmo() {}
    };

}
