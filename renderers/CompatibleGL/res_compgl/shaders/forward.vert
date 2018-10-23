// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_VERTEX_IN vec3 inPos;
PH_VERTEX_IN vec3 inNormal;
PH_VERTEX_IN vec2 inTexcoord;

// Output
PH_VERTEX_OUT vec3 vsPos;
PH_VERTEX_OUT vec3 vsNormal;
PH_VERTEX_OUT vec2 texcoord;

// Uniforms
uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat4 uNormalMatrix; // inverse(transpose(modelViewMatrix)) for non-uniform scaling

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	vec4 vsPosTmp = uViewMatrix * uModelMatrix * vec4(inPos, 1.0);

	vsPos = vsPosTmp.xyz / vsPosTmp.w; // Unsure if division necessary.
	vsNormal = (uNormalMatrix * vec4(inNormal, 0.0)).xyz;
	texcoord = inTexcoord;

	gl_Position = uProjMatrix * vsPosTmp;
}
