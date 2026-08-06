#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D textureGhost[10];
uniform vec2 uGhostOffset[10];
uniform float uGhostAlpha[10];
uniform float uTime;
uniform float uScanlineIntensity;
uniform float uScanlineCount;
uniform float uScanlineSpeed;
uniform float uCurvature;
uniform float uVignette;
uniform float uNoise;
uniform float uAberration;
uniform float uFlicker;
uniform float uTrackingGlitch;
uniform float uWaveDistortion;
uniform float uWaveSpeed;
uniform float uDropoutLines;
uniform float uJitter;
uniform float uVerticalRoll;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 barrel(vec2 uv, float amount) {
    vec2 cc = uv - 0.5;
    float dist = dot(cc, cc);
    return uv + cc * dist * amount;
}

void main() {
    vec2 uvBase = barrel(fragTexCoord, uCurvature);

    if (uvBase.x < 0.0 || uvBase.x > 1.0 || uvBase.y < 0.0 || uvBase.y > 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec2 uv = uvBase;

    uv.y = fract(uv.y + uTime * uVerticalRoll);

    float trackBandY = fract(uTime * 0.15 + rand(vec2(floor(uTime * 0.37), 0.0)) * 0.7);
    float trackBandH = 0.02 + rand(vec2(floor(uTime * 0.53), 1.0)) * 0.05;
    if (uTrackingGlitch > 0.0 && abs(uv.y - trackBandY) < trackBandH) {
        float shiftN = rand(vec2(floor(uv.y * 60.0), floor(uTime * 24.0))) - 0.5;
        uv.x += shiftN * uTrackingGlitch * 0.2;
    }

    uv.x += sin(uv.y * 12.0 + uTime * uWaveSpeed) * uWaveDistortion * 0.01;
    uv.x += (rand(vec2(floor(uTime * 30.0), 7.0)) - 0.5) * uJitter * 0.01;

    float ab = uAberration * 0.002;
    float r = texture2D(texture0, uv + vec2(ab, 0.0)).r;
    vec4 center = texture2D(texture0, uv);
    float g = center.g;
    float b = texture2D(texture0, uv - vec2(ab, 0.0)).b;
    vec3 color = vec3(r, g, b);

    float scanY = uv.y - uTime * uScanlineSpeed * 0.2;
    float scan = sin(scanY * uScanlineCount * 3.14159) * 0.5 + 0.5;
    color *= mix(1.0, scan, uScanlineIntensity);

    if (uDropoutLines > 0.0) {
        float dropRand = rand(vec2(floor(uv.y * 450.0), floor(uTime * 28.0)));
        if (dropRand > 1.0 - uDropoutLines * 0.06) {
            color = mix(color, vec3(1.0), 0.85);
        }
    }

    color += (rand(uv * uTime) - 0.5) * uNoise;

    float d = distance(uv, vec2(0.5));
    color *= mix(1.0, 1.0 - d, uVignette);

    color += rand(vec2(uTime, 0.0)) * uFlicker * 0.1;

    for (int i = 0; i < 10; i++) {
        vec3 ghostSample = texture2D(textureGhost[i], uv - uGhostOffset[i]).rgb;
        color = mix(color, ghostSample, uGhostAlpha[i]);
    }

    color = clamp(color, 0.0, 1.0);
    gl_FragColor = vec4(color, center.a);
}
