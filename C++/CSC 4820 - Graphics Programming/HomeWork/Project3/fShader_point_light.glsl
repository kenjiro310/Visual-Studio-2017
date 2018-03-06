#version 130

in vec3 N; // interpolated normal for the pixel
in vec3 v; // interpolated position for the pixel 

// Uniform block for the light source properties
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

// Uniform block for surface material properties
layout (std140) uniform materialProp {
	uniform vec4 Kambient;
	uniform vec4 Kdiffuse;
	uniform vec4 Kspecular;
	uniform float shininess;
};

out vec4 color;

// This fragment shader is an example of per-pixel lighting.
void main() {

    // Now calculate the parameters for the lighting equation:
    // color = Ka * Lag + (Ka * La) + attenuation * ((Kd * (N dot L) * Ld) + (Ks * ((N dot HV) ^ shininess) * Ls))
    // Ka, Kd, Ks: surface material properties
    // Lag: global ambient light (not used in this example)
    // La, Ld, Ls: ambient, diffuse, and specular components of the light source
    // N: normal
    // L: light vector
    // HV: half vector
    // shininess
    // attenuation: light intensity attenuation over distance and spotlight angle
    
    vec3 lightVector;
    float attenuation = 1.0; 
    float spotEffect;

    // point light source
    lightVector = normalize(lightSourcePosition.xyz - v);

    // calculate light attenuation 
    float distance = length(lightSourcePosition.xyz - v);
    attenuation = 1.0 / (constantAttenuation + (linearAttenuation * distance) 
		+(quadraticAttenuation * distance * distance));

   //calculate Diffuse Color  
   float NdotL = max(dot(N,lightVector), 0.0);

   vec4 diffuseColor = Kdiffuse * diffuseLightIntensity * NdotL;

   // calculate Specular color. Here we use the original Phong illumination model. 
   vec3 E = normalize(-v); // Eye vector. We are in Eye Coordinates, so EyePos is (0,0,0)  
   
   vec3 R = normalize(-reflect(lightVector,N)); // light reflection vector

   float RdotE = max(dot(R,E),0.0);

   vec4 specularColor = Kspecular * specularLightIntensity * pow(RdotE,shininess);

   // ambient color
   vec4 ambientColor = Kambient * ambientLightIntensity;

   color = ambientColor + attenuation * (diffuseColor + specularColor);   
}

