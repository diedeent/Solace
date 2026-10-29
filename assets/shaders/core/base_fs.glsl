#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 u_sunDir;
uniform vec3 u_sunColor;
uniform vec3 u_skyColor;
uniform vec3 u_groundColor;
uniform vec3 u_baseColor;   
uniform vec3 u_viewPos;     
uniform vec3 u_rimColor;    

uniform float u_toonMidStep;    
uniform float u_toonShadowStep; 
uniform float u_rimPower;       

uniform sampler2D u_textureDiffuse;
uniform bool u_hasTexture; // <-- ФЛАГ: есть ли текстура у модели

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_sunDir);
    vec3 viewDir = normalize(u_viewPos - FragPos);

    // Cel-shading и освещение...
    float NdotL = dot(norm, lightDir);
    float shadowStep = smoothstep(0.0, u_toonShadowStep, NdotL);
    float midStep = smoothstep(u_toonMidStep, u_toonMidStep + 0.05, NdotL);
    vec3 toonDiffuse = (shadowStep * 0.6 + midStep * 0.4) * u_sunColor;

    float ndotup = norm.y * 0.5 + 0.5; 
    vec3 ambient = mix(u_groundColor, u_skyColor, ndotup);

    // ПРОВЕРКА: цветная модель или текстурная?
    vec3 objectColor = u_baseColor;
    float alpha = 1.0f;

    if (u_hasTexture) {
        vec4 texColor = texture(u_textureDiffuse, TexCoords);
        if (texColor.a < 0.1f) discard; 
        objectColor = texColor.rgb * u_baseColor; // Текстура с возможностью легкого тонирования цветом
        alpha = texColor.a;
    }

    vec3 lighting = ambient + toonDiffuse;
    vec3 finalColor = objectColor * lighting;

    // Rim light...
    float rim = 1.0 - max(dot(norm, viewDir), 0.0);
    rim = smoothstep(u_rimPower, u_rimPower + 0.1, rim); 
    finalColor += u_rimColor * rim * objectColor; 

    FragColor = vec4(finalColor, alpha);
}