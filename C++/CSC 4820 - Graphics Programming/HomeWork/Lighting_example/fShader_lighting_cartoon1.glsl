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

const vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
const vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
	
// Output color
out vec4 color;

// This fragment shader is an example of cartoonish lighting.
void main() {

    vec3 lightVector = normalize(lightSourcePosition.xyz - v);

    vec3 eyeVector = normalize(-v); // Eye vector. We are in Eye Coordinates, so EyePos is (0,0,0) 

	// This is the cosine of the angle between the normal vector and the light vector
	// for this pixel. If this value is small, it means the angle between the normal
	// vector and the light vector is big. 
    float NdotL = max(dot(N,lightVector), 0.0);

	// Ignore NdotL that is below 0. 
    float Kd = max(NdotL, 0.0);

	// This is a simple cartoonish lighting equation. Mix red and yellow color with Kd.
	// This means if the light comes in a small angle with the normal vector, then the 
	// pixel will look more red. If the light comes in a larger angle with the normal
	// vector, then the pixel will look more yellow. 
    //color = mix(red, yellow, Kd);
	
	// An alternative lighting equation. The color change will be more abrupt, more like 
	// hand painting. 
	color = Kd > 0.6 ? yellow : red;

	// If the angle between the light vector and the normal is too big, paint the pixel
	// black. This pixel is likely on the silhouette. 
    if (abs(dot(eyeVector, N)) < 0.15) {
       color = black;
    }
}

