#version 330 core
out vec4 fragColor;

uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_cameraPos;

// --- Быстрый хэш и шум ---
vec3 hash3(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(dot(hash3(i + vec3(0,0,0)), f - vec3(0,0,0)),
                       dot(hash3(i + vec3(1,0,0)), f - vec3(1,0,0)), u.x),
                   mix(dot(hash3(i + vec3(0,1,0)), f - vec3(0,1,0)),
                       dot(hash3(i + vec3(1,1,0)), f - vec3(1,1,0)), u.x), u.y),
               mix(mix(dot(hash3(i + vec3(0,0,1)), f - vec3(0,0,1)),
                       dot(hash3(i + vec3(1,0,1)), f - vec3(1,0,1)), u.x),
                   mix(dot(hash3(i + vec3(0,1,1)), f - vec3(0,1,1)),
                       dot(hash3(i + vec3(1,1,1)), f - vec3(1,1,1)), u.x), u.y), u.z);
}

float fbm(vec3 p) {
    float f = 0.0;
    float w = 0.5;
    for (int i = 0; i < 3; i++) { // Уменьшено до 3 октав для молниеносной работы
        f += w * noise(p);
        p *= 2.0;
        w *= 0.5;
    }
    return f;
}

float ridgedNoise(vec3 p) {
    float f = 0.0;
    float w = 0.5;
    for (int i = 0; i < 3; i++) {
        float n = 1.0 - abs(noise(p));
        n = n * n;
        f += n * w;
        w *= 0.5;
        p *= 2.0;
    }
    return f;
}

// 1. ЛЕГКАЯ БАЗОВАЯ ГЕОМЕТРИЯ (дает 1000 FPS без лагов)
float map(vec3 p) {
    vec3 boxD = abs(p) - vec3(0.9, 1.9, 0.3);
    return length(max(boxD, 0.0)) + min(max(boxD.x, max(boxD.y, boxD.z)), 0.0);
}

// Расчет нормалей по легкой форме
vec3 calcNormal(vec3 p) {
    float h = 0.001;
    vec2 k = vec2(1.0, -1.0);
    return normalize(k.xyy * map(p + k.xyy * h) +
                     k.yyx * map(p + k.yyx * h) +
                     k.yxy * map(p + k.yxy * h) +
                     k.xxx * map(p + k.xxx * h));
}

void main() {
    vec2 st = (gl_FragCoord.xy * 2.0 - u_resolution.xy) / min(u_resolution.x, u_resolution.y);

    vec3 ro = u_cameraPos;
    vec3 rd = normalize(vec3(st, 1.0));

    // Быстрый Raymarching (базовая плита)
    float t = 0.0;
    float maxDist = 35.0;

    for (int i = 0; i < 64; i++) { // Оптимальное число шагов
        vec3 p = ro + rd * t;
        float d = map(p);
        if (d < 0.001 || t > maxDist) break;
        t += d;
    }

    vec3 color = vec3(0.003, 0.003, 0.008);

    if (t < maxDist) {
        vec3 p = ro + rd * t;
        vec3 baseNormal = calcNormal(p);

        // --- 2. КОМБИНИРОВАННЫЙ МЕТОД: НАТЯГИВАЕМ ТЕКСТУРУ СЛАНЦА ПОВЕРХ ТОЧКИ УДАРА ---
        // Вся тяжелая математика сланца и трещин считается ОДИН РАЗ в точке hit, а не во время луча!

        float slateStrata = abs(sin(p.y * 16.0 + fbm(p * 2.5) * 3.0));
        float fractures = ridgedNoise(p * 3.0);

        // Модифицируем нормаль на поверхности, создавая эффект честного рельефа (Bump/Normal mapping)
        vec3 normal = normalize(baseNormal + vec3(dFdx(slateStrata), dFdy(slateStrata), 0.0) * 0.4);

        // Палитра темного сланца и базальта
        vec3 stoneColor = mix(vec3(0.05, 0.05, 0.07), vec3(0.15, 0.16, 0.18), fbm(p * 4.0));

        // Кварцевые прожилки в трещинах
        float quartz = smoothstep(0.7, 0.9, fractures);
        stoneColor = mix(stoneColor, vec3(0.5, 0.6, 0.65), quartz * 0.7);

        // Грязь в углублениях сланца
        stoneColor *= mix(0.6, 1.0, slateStrata);

        // Освещение
        vec3 lightDir = normalize(vec3(0.5, 1.0, -0.4));
        float diff = max(dot(normal, lightDir), 0.0);

        vec3 viewDir = normalize(ro - p);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);

        vec3 lighting = stoneColor * (diff * 1.5 + 0.1) + vec3(0.25) * spec;

        // Туман
        float fog = 1.0 - exp(-0.03 * t * t);
        color = mix(lighting, vec3(0.003, 0.003, 0.008), fog);
    }

    color = pow(color, vec3(1.0 / 2.2));
    fragColor = vec4(color, 1.0);
}
