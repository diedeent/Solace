#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aBoneWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

layout(std430, binding = 0) readonly buffer BoneBuffer {
    mat4 u_bones[];
};

void main() {
    float totalWeight = aBoneWeights[0] + aBoneWeights[1] + aBoneWeights[2] + aBoneWeights[3];
    vec4 localPos = vec4(aPos, 1.0);
    vec3 localNormal = aNormal;
    
    if (totalWeight > 0.001) {
        vec4 skinnedPos = vec4(0.0);
        vec3 skinnedNormal = vec3(0.0);
        for (int i = 0; i < 4; ++i) {
            int boneID = aBoneIDs[i];
            float weight = aBoneWeights[i];
            if (weight > 0.0 && boneID >= 0) {
                skinnedPos += weight * (u_bones[boneID] * vec4(aPos, 1.0));
                skinnedNormal += weight * (mat3(u_bones[boneID]) * aNormal);
            }
        }
        localPos = skinnedPos;
        localNormal = skinnedNormal;
    }
    
    FragPos = vec3(u_model * localPos);
    Normal = mat3(transpose(inverse(u_model))) * localNormal;
    TexCoords = aTexCoords;
    gl_Position = u_proj * u_view * vec4(FragPos, 1.0);
} 