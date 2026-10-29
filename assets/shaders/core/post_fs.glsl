#version 330 core
in  vec2 v_uv;
out vec4 outColor;

uniform sampler2D u_scene;   
uniform vec2      u_lowRes;  
uniform float     u_paletteBits;
uniform float     u_dither;
uniform float     u_bloomStrength;
uniform float     u_bloomThreshold;

float bayer2(vec2 a) { a = floor(a); return fract(a.x * 0.5 + a.y * a.y * 0.75); }
float bayer4(vec2 a) { return bayer2(0.5 * a) * 0.25 + bayer2(a); }
float bayer8(vec2 a) { return bayer4(0.5 * a) * 0.25 + bayer2(a); }

vec3 texel(vec2 p) {
    return texture(u_scene, (p + 0.5) / u_lowRes).rgb;
}

void main() {
    vec2 pix = floor(v_uv * u_lowRes);   
    vec3 col = texel(pix);

    // Bloom
    vec3 bloom = vec3(0.0);
    bloom += max(texel(pix + vec2(-1.0, -1.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2( 0.0, -1.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2( 1.0, -1.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2(-1.0,  0.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2( 0.0,  0.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2( 1.0,  0.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2(-1.0,  1.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2( 0.0,  1.0)) - u_bloomThreshold, 0.0);
    bloom += max(texel(pix + vec2( 1.0,  1.0)) - u_bloomThreshold, 0.0);

    col += bloom * (u_bloomStrength / 9.0);

    float safePaletteBits = max(u_paletteBits, 5.0); 
    float levels = pow(2.0, safePaletteBits) - 1.0;
    
    float noise  = bayer8(pix) - 0.5;
    col = floor(col * levels + noise * u_dither + 0.5) / levels;

    outColor = vec4(col, 1.0);
} 
