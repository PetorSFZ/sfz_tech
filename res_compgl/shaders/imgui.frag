// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_FRAGMENT_IN vec2 texcoord;
PH_FRAGMENT_IN vec4 color;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms
uniform sampler2D uTexture;

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	vec4 outTmp = vec4(color.rgb, color.a * PH_TEXREAD(uTexture, texcoord).x);

#ifdef PH_WEB_GL
	gl_FragColor = outTmp;
#else
	fragOut = outTmp;
#endif
}
