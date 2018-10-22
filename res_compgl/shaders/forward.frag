// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_FRAGMENT_IN vec3 vsPos;
PH_FRAGMENT_IN vec3 vsNormal;
PH_FRAGMENT_IN vec2 texcoord;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms (material)
uniform Material uMaterial;
uniform sampler2D uAlbedoTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uOcclusionTexture;
uniform sampler2D uEmissiveTexture;

// Uniforms (static spherelights)
const int MAX_NUM_STATIC_SPHERE_LIGHTS = 32;
uniform SphereLight uStaticSphereLights[MAX_NUM_STATIC_SPHERE_LIGHTS];
uniform int uNumStaticSphereLights;

// Uniforms (dynamic spherelights)
const int MAX_NUM_DYNAMIC_SPHERE_LIGHTS = 32;
uniform SphereLight uDynamicSphereLights[MAX_NUM_DYNAMIC_SPHERE_LIGHTS];
uniform int uNumDynamicSphereLights;

// Gamma Correction Functions
// ------------------------------------------------------------------------------------------------

const vec3 gamma = vec3(2.2);

vec3 linearize(vec3 rgbGamma)
{
	return pow(rgbGamma, gamma);
}

vec4 linearize(vec4 rgbaGamma)
{
	return vec4(linearize(rgbaGamma.rgb), rgbaGamma.a);
}

vec3 applyGammaCorrection(vec3 linearValue)
{
	return pow(linearValue, vec3(1.0 / gamma));
}

vec4 applyGammaCorrection(vec4 linearValue)
{
	return vec4(applyGammaCorrection(linearValue.rgb), linearValue.a);
}

// PBR shading functions
// ------------------------------------------------------------------------------------------------

const float PI = 3.14159265359;

// References used:
// https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// http://blog.selfshadow.com/publications/s2016-shading-course/
// http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html

// Normal distribution function, GGX/Trowbridge-Reitz
// a = roughness^2, UE4 parameterization
// dot(n,h) term should be clamped to 0 if negative
float ggx(float nDotH, float a)
{
	float a2 = a * a;
	float div = PI * pow(nDotH * nDotH * (a2 - 1.0) + 1.0, 2.0);
	return a2 / div;
}

// Schlick's model adjusted to fit Smith's method
// k = a/2, where a = roughness^2, however, for analytical light sources (non image based)
// roughness is first remapped to roughness = (roughnessOrg + 1) / 2.
// Essentially, for analytical light sources:
// k = (roughness + 1)^2 / 8
// For image based lighting:
// k = roughness^2 / 2
float geometricSchlick(float nDotL, float nDotV, float k)
{
	float g1 = nDotL / (nDotL * (1.0 - k) + k);
	float g2 = nDotV / (nDotV * (1.0 - k) + k);
	return g1 * g2;
}

// Schlick's approximation. F0 should typically be 0.04 for dielectrics
vec3 fresnelSchlick(float nDotL, vec3 f0)
{
	return f0 + (vec3(1.0) - f0) * clamp(pow(1.0 - nDotL, 5.0), 0.0, 1.0);
}

// Shade function
// ------------------------------------------------------------------------------------------------

vec3 shade(
	SphereLight light,
	vec3 p,
	vec3 n,
	vec3 v,
	float nDotV,
	vec3 albedo,
	float roughness,
	float metallic)
{
	// Shading parameters
	vec3 toLight = light.vsPos - p;
	float toLightDist = length(toLight);
	vec3 l = toLight * (1.0 / toLightDist); // to light
	vec3 h = normalize(l + v); // half vector (normal of microfacet)

	// If nDotL is <= 0 then the light source is not in the hemisphere of the surface, i.e.
	// no shading needs to be performed
	float nDotL = dot(n, l);
	if (nDotL <= 0.0) return vec3(0.0);

	// Lambert diffuse
	vec3 diffuse = albedo / PI;

	// Cook-Torrance specular
	// Normal distribution function
	float nDotH = max(dot(n, h), 0.0); // max() should be superfluous here
	float ctD = ggx(nDotH, roughness * roughness);

	// Geometric self-shadowing term
	float k = pow(roughness + 1.0, 2.0) / 8.0;
	float ctG = geometricSchlick(nDotL, nDotV, k);

	// Fresnel function
	// Assume all dielectrics have a f0 of 0.04, for metals we assume f0 == albedo
	vec3 f0 = mix(vec3(0.04), albedo, metallic);
	vec3 ctF = fresnelSchlick(nDotV, f0);

	// Calculate final Cook-Torrance specular value
	vec3 specular = ctD * ctF * ctG / (4.0 * nDotL * nDotV);

	// Calculates light strength
	float shadow = 1.0; // TODO: Shadow map
	float fallofNumerator = pow(clamp(1.0 - pow(toLightDist / light.range, 4.0), 0.0, 1.0), 2.0);
	float fallofDenominator = (toLightDist * toLightDist + 1.0);
	float falloff = fallofNumerator / fallofDenominator;
	vec3 lightContrib = falloff * light.strength * shadow;

	vec3 ks = ctF;
	vec3 kd = (1.0 - ks) * (1.0 - metallic);

	// "Solves" reflectance equation under the assumption that the light source is a point light
	// and that there is no global illumination.
	return (kd * diffuse + specular) * lightContrib * nDotL;
}

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	// Albedo (Gamma space)
	vec3 albedo = uMaterial.albedo.rgb;
	float alpha = uMaterial.albedo.a;
	if (uMaterial.hasAlbedoTexture != 0) {
		vec4 tmp = PH_TEXREAD(uAlbedoTexture, texcoord);
		albedo *= tmp.rgb;
		alpha *= tmp.a;
	}
	albedo = linearize(albedo);

	// Skip fragment if it is transparent
	if (alpha < 0.1) discard;

	// Metallic & Roughness (Linear space)
	float metallic = uMaterial.metallic;
	float roughness = uMaterial.roughness;
	if (uMaterial.hasMetallicRoughnessTexture != 0) {
		vec2 tmp = PH_TEXREAD(uMetallicRoughnessTexture, texcoord).rg;
		metallic *= tmp.r;
		roughness *= tmp.g;
	}
	roughness = max(roughness, 0.01); // Div by 0 in ggx() if roughness = 0

	// Emissive (Linear space??? TODO: Maybe gamma)
	vec3 emissive = uMaterial.emissive.rgb;
	if (uMaterial.hasEmissiveTexture != 0) {
		emissive *= PH_TEXREAD(uEmissiveTexture, texcoord).rgb;
	}

	// Fragment's position and normal
	vec3 p = vsPos;
	vec3 n = normalize(vsNormal);

	vec3 v = normalize(-p); // to view
	float nDotV = dot(n, v);

	// Interpolation of normals sometimes makes them face away from the camera. Clamp
	// these to almost zero, to not break shading calculations.
	nDotV = max(0.001, nDotV);

	vec3 totalOutput = emissive;

	// Static lights
	for (int i = 0; i < MAX_NUM_STATIC_SPHERE_LIGHTS; i++) {

		if (i >= uNumStaticSphereLights) break;
		SphereLight light = uStaticSphereLights[i];

		vec3 res = shade(
			light,
			p,
			n,
			v,
			nDotV,
			albedo,
			roughness,
			metallic);
		totalOutput += res;
	}

	// Dynamic lights
	for (int i = 0; i < MAX_NUM_STATIC_SPHERE_LIGHTS; i++) {

		if (i >= uNumDynamicSphereLights) break;
		SphereLight light = uDynamicSphereLights[i];

		vec3 res = shade(
			light,
			p,
			n,
			v,
			nDotV,
			albedo,
			roughness,
			metallic);
		totalOutput += res;
	}

	vec4 outTmp = vec4(applyGammaCorrection(totalOutput), 1.0);
#ifdef PH_WEB_GL
	gl_FragColor = outTmp;
#else
	fragOut = outTmp;
#endif
}
