#version 330

in vec2 textureCoord;

uniform sampler2D textureMap0;
uniform sampler2D textureMap1;
uniform sampler2D textureMap2;

out vec4 fragColor;

void main() {

// retrieve color from each texture
    vec4 textureColor1 = texture2D(textureMap0, textureCoord);
    vec4 textureColor2 = texture2D(textureMap1, textureCoord);
    vec4 textureColor3 = texture2D(textureMap2, textureCoord);
	
// Combine the two texture colors
// Depending on the texture colors, you may multiply, add,
// or mix the two colors.
#if __VERSION__ >= 130
 if ((textureColor1.r <= 0.01f) && (textureColor1.g <= 0.01f)
  && (textureColor1.b <= 0.01f)) {
  textureColor1.r = 1.0f;
 } 
  
 vec4 textureColor4 = vec4(vec3(1.0, 1.0, 1.0) - textureColor3.rgb, 1.0);
  
 fragColor = textureColor3 * textureColor1 + textureColor4 * textureColor2;
#else
    gl_FragColor = textureColor1 * textureColor2 * textureColor3;
#endif
}

