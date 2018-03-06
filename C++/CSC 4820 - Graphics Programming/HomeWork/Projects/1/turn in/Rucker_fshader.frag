#version 330

layout (std140) uniform Material {
uniform	vec4 diffuse;
uniform	vec4 ambient;
uniform	vec4 specular;
uniform	vec4 emissive;
uniform	float shininess;
uniform	int texCount;
};

uniform	sampler2D texUnit;

in vec3 Normal;
in vec2 TexCoord;
out vec4 out_color;

void main()
{
	vec4 color;
	vec4 amb;
	float intensity;
	vec3 lightDir;
	vec3 n;
	
	lightDir = normalize(vec3(1.0,1.0,1.0));
	n = normalize(Normal);	
	intensity = max(dot(lightDir,n),0.0);
	
	if (texCount == 0) {
		color = diffuse;
		amb = ambient;
	}
	else {
		color = texture(texUnit, TexCoord);
		amb = color * 0.33;
	}
	out_color = (color * intensity) + amb;
	//out_color = vec4(texCount,0.0,0.0,1.0);

}
