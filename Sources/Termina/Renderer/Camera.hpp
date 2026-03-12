#pragma once

#include <GLM/glm.hpp>

#include <Termina/Core/Common.hpp>

namespace Termina {
    struct Camera
    {
        glm::mat4 Projection = glm::mat4(1.0f);
        glm::mat4 View = glm::mat4(1.0f);
        glm::mat4 ViewProjection = glm::mat4(1.0f);
        glm::mat4 InverseProjection = glm::mat4(1.0f);
        glm::mat4 InverseView = glm::mat4(1.0f);
        glm::mat4 InverseViewProjection = glm::mat4(1.0f);
        glm::vec3 Position = glm::vec3(0.0f);
        glm::vec3 Direction = glm::vec3(0.0f);
        glm::vec2 Jitter = glm::vec2(0.0f);

        glm::mat4 PreviousProjection = glm::mat4(1.0f);
        glm::mat4 PreviousView = glm::mat4(1.0f);
        glm::mat4 PreviousViewProjection = glm::mat4(1.0f);
        glm::mat4 PreviousInverseProjection = glm::mat4(1.0f);
        glm::mat4 PreviousInverseView = glm::mat4(1.0f);
        glm::mat4 PreviousInverseViewProjection = glm::mat4(1.0f);
        glm::vec3 PreviousPosition = glm::vec3(0.0f);
        glm::vec3 PreviousDirection = glm::vec3(0.0f);
        glm::vec2 PreviousJitter = glm::vec2(0.0f);

        float32 Near = 0.01f;
        float32 Far = 1000.0f;

        void UpdatePrevious()
        {
            PreviousProjection = Projection;
            PreviousView = View;
            PreviousViewProjection = ViewProjection;
            PreviousInverseProjection = InverseProjection;
            PreviousInverseView = InverseView;
            PreviousInverseViewProjection = InverseViewProjection;
            PreviousPosition = Position;
            PreviousDirection = Direction;
            PreviousJitter = Jitter;
        }
    };
}
