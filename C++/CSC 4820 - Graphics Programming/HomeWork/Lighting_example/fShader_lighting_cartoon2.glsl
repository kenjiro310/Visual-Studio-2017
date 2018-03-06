#version 330

in vec3 N; // interpolated normal for the pixel
in vec3 v; // interpolated position for the pixel 

// Preserve the uniform block so we can use the same C++ code for different light shaders. 
layout (std140) uniform LightSourceProp {
	// Light source position in eye space (i.e. eye is at (0, 0, 0))
	uniform vec4 lightSourcePosition;
	
	uniform vec4 diffuseLightIntensity;
	uniform vec4 specularLightIntensity;
	uniform vec4 ambientLightIntensity;
	
	// for calculating the light attenuation 
	uniform float constantAttenuation;
	uniform float linearAttenuation;
	uniform float quadraticAttenuation;
	
	// Spotlight direction
	uniform vec3 spotDirection;

	// Spotlight cutoff angle
	uniform float spotCutoff;
};

layout (std140) uniform materialProp {
	uniform vec4 Kambient;
	uniform vec4 Kdiffuse;
	uniform vec4 Kspecular;
	uniform float shininess;
};

vec4 ambient = {0.2, 0.2, 0.2, 1.0};

out vec4 color;

// This fragment shader is an example of cartoonish lighting.
void main() {

    vec3 lightVector = normalize(lightSourcePosition.xyz - v);

    float NdotL = max(dot(N,lightVector), 0.0);

    float Kd = max(NdotL, 0.0);

	// Paint the pixel is different colors based on the angle of incoming light. 
    if (Kd > 0.95) {
        color = vec4(1.0, 0.5, 0.5, 1.0);
    } else if (Kd > 0.5) {
        color = vec4(0.6, 0.3, 0.3, 1.0);
    } else if (Kd > 0.25) {
        color = vec4(0.4, 0.2, 0.2, 1.0);
    } else {
        color = vec4(0.1, 0.1, 0.1, 1.0);
    }

    color = color + ambient;
}

