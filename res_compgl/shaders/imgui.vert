// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_VERTEX_IN vec2 inPos;
PH_VERTEX_IN vec2 inTexcoord;
PH_VERTEX_IN vec4 inColor;

// Output
PH_VERTEX_OUT vec2 texcoord;
PH_VERTEX_OUT vec4 color;

// Uniforms
uniform mat4 uProjMatrix;

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	texcoord = inTexcoord;
	color = inColor;
	gl_Position = uProjMatrix * vec4(inPos, 0.0, 1.0);
}
