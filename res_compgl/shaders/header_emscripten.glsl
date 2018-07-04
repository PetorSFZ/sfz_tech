precision mediump float;
#define PH_WEB_GL 1
#define PH_VERTEX_IN attribute
#define PH_VERTEX_OUT varying
#define PH_FRAGMENT_IN varying
#define PH_TEXREAD(sampler, coord) texture2D(sampler, coord)

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
