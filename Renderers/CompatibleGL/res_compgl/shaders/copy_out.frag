// Input
PH_FRAGMENT_IN vec2 texcoord;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms
uniform sampler2D uTexture;

void main()
{
	vec3 val = PH_TEXREAD(uTexture, texcoord).rgb;

#ifdef PH_WEB_GL
	gl_FragColor = vec4(val, 1.0);
#else
	fragOut = vec4(val, 1.0);
#endif
}
