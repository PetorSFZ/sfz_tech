#version 330
precision highp float;
#define PH_DESKTOP_GL 1
#define PH_VERTEX_IN in
#define PH_VERTEX_OUT out
#define PH_FRAGMENT_IN in
#define PH_TEXREAD(sampler, coord) texture(sampler, coord)

// Structs
// ------------------------------------------------------------------------------------------------

// Material struct
struct Material {
	vec4 albedo;
	vec3 emissive;
	float roughness;
	float metallic;

	int hasAlbedoTexture;
	int hasMetallicRoughnessTexture;
	int hasNormalTexture;
	int hasOcclusionTexture;
	int hasEmissiveTexture;
};

// SphereLight struct
struct SphereLight {
	vec3 vsPos;
	float radius;
	float range;
	vec3 strength;
};
