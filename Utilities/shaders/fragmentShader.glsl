#version 330 core

// Inputs from vertex shader
in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

// Output
out vec4 outFragmentColor;

// --- Mode toggles ---
uniform bool bUseTexture       = false;
uniform bool bUsePBR           = false;
uniform bool bUseCheckerboard  = false;

// --- Original simple-texture / color uniforms ---
uniform vec4 objectColor  = vec4(1.0);
uniform sampler2D objectTexture;
uniform vec2 UVscale      = vec2(1.0, 1.0);

// --- PBR texture maps ---
uniform sampler2D albedoMap;      // unit 0
uniform sampler2D normalMap;      // unit 1
uniform sampler2D metallicMap;    // unit 2
uniform sampler2D roughnessMap;   // unit 3
uniform sampler2D aoMap;          // unit 4
uniform sampler2D heightMap;      // unit 5

// --- PBR color tint (multiplied with albedo) ---
uniform vec3 pbrTint = vec3(1.0, 1.0, 1.0);

// --- Parallax ---
uniform bool  bUseParallax   = false;
uniform float parallaxScale  = 0.04;

// --- Checkerboard custom colours ---
uniform vec3 checkerColor1 = vec3(1.0);
uniform vec3 checkerColor2 = vec3(0.0);

// --- Multi-light system ---
const int MAX_LIGHTS = 8;
uniform int       numLights                   = 0;
uniform vec3      lightPositions[MAX_LIGHTS];
uniform vec3      lightColors[MAX_LIGHTS];
uniform float     lightIntensities[MAX_LIGHTS];

// --- Legacy single light (used for simple texture / flat-colour paths) ---
uniform vec3 lightPos   = vec3(0.0, 8.0, 0.0);
uniform vec3 lightColor = vec3(1.0);
uniform vec3 viewPos    = vec3(0.0);

// --- Environment hemisphere (fake ambient) ---
uniform vec3  envColorTop    = vec3(0.85, 0.90, 1.00);
uniform vec3  envColorBottom = vec3(0.15, 0.12, 0.10);
uniform float envIntensity   = 0.60;

// ====================================================================
// Cotangent-frame TBN (no tangent attribute required)
// ====================================================================
mat3 CotangentFrame(vec3 N, vec3 p, vec2 uv)
{
    vec3 dp1  = dFdx(p);
    vec3 dp2  = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);

    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

vec3 PerturbNormal(vec3 N, vec3 worldPos, vec2 uv)
{
    vec3 mapNormal = texture(normalMap, uv).rgb * 2.0 - 1.0;
    mat3 TBN = CotangentFrame(N, worldPos, uv);
    return normalize(TBN * mapNormal);
}

// ====================================================================
// Parallax Occlusion Mapping
// ====================================================================
vec2 ParallaxOcclusionMap(vec2 texCoords, vec3 viewDirTangent)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDirTangent)));

    float layerDepth    = 1.0 / numLayers;
    float currentDepth  = 0.0;
    vec2  P             = viewDirTangent.xy * parallaxScale;
    vec2  deltaTexCoords = P / numLayers;

    vec2  currentTexCoords = texCoords;
    float currentMapValue  = 1.0 - texture(heightMap, currentTexCoords).r;  // invert: white = high

    for (int i = 0; i < 32; ++i)
    {
        if (currentDepth >= currentMapValue) break;
        currentTexCoords -= deltaTexCoords;
        currentMapValue   = 1.0 - texture(heightMap, currentTexCoords).r;
        currentDepth     += layerDepth;
    }

    // Interpolate between current and previous sample for smoother result
    vec2  prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth    = currentMapValue - currentDepth;
    float beforeDepth   = (1.0 - texture(heightMap, prevTexCoords).r) - currentDepth + layerDepth;
    float weight        = afterDepth / (afterDepth - beforeDepth);

    return mix(currentTexCoords, prevTexCoords, weight);
}

// ====================================================================
// GGX / Cook-Torrance helpers
// ====================================================================
const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness)
         * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ====================================================================
// Hemisphere ambient
// ====================================================================
vec3 HemisphereAmbient(vec3 N)
{
    float blend = N.y * 0.5 + 0.5;  // -1..1 -> 0..1
    return mix(envColorBottom, envColorTop, blend) * envIntensity;
}

// ====================================================================
// Simple Blinn-Phong for non-PBR paths
// ====================================================================
vec3 BlinnPhong(vec3 baseColor, vec3 N, vec3 fragPos)
{
    // Hemisphere ambient
    vec3 ambient = HemisphereAmbient(N) * baseColor * 0.3;

    vec3 result = ambient;

    // Accumulate contribution from all lights
    vec3 V = normalize(viewPos - fragPos);
    int count = max(numLights, 1);

    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (i >= count) break;

        vec3  lPos       = (numLights > 0) ? lightPositions[i] : lightPos;
        vec3  lColor     = (numLights > 0) ? lightColors[i]    : lightColor;
        float lIntensity = (numLights > 0) ? lightIntensities[i] : 30.0;

        vec3  L    = lPos - fragPos;
        float dist = length(L);
        L = normalize(L);

        float attenuation = lIntensity / (dist * dist + 1.0);
        vec3  radiance    = lColor * attenuation;

        // Diffuse
        float diff = max(dot(N, L), 0.0);

        // Specular (Blinn-Phong)
        vec3  H    = normalize(V + L);
        float spec = pow(max(dot(N, H), 0.0), 32.0);

        result += (diff * baseColor + spec * vec3(0.3)) * radiance;
    }

    return result;
}

// ====================================================================
void main()
{
    // ----------------------------------------------------------------
    // PATH 0: Procedural checkerboard (with simple lighting)
    // ----------------------------------------------------------------
    if (bUseCheckerboard)
    {
        vec2 uv = fragmentTextureCoordinate * UVscale;
        float checker = mod(floor(uv.x) + floor(uv.y), 2.0);
        vec3 baseColor = mix(checkerColor1, checkerColor2, checker);

        vec3 N = normalize(fragmentVertexNormal);
        vec3 lit = BlinnPhong(baseColor, N, fragmentPosition);
        lit = lit / (lit + vec3(1.0));  // Reinhard tonemap
        lit = pow(lit, vec3(1.0 / 2.2));  // gamma
        outFragmentColor = vec4(lit, 1.0);
        return;
    }

    // ----------------------------------------------------------------
    // PATH 1: PBR textures with Cook-Torrance lighting + parallax
    // ----------------------------------------------------------------
    if (bUsePBR)
    {
        vec3 N = normalize(fragmentVertexNormal);
        vec3 V = normalize(viewPos - fragmentPosition);

        vec2 uv = fragmentTextureCoordinate * UVscale;

        // --- Parallax ---
        if (bUseParallax)
        {
            mat3 TBN = CotangentFrame(N, fragmentPosition, uv);
            vec3 viewDirTangent = normalize(transpose(TBN) * V);
            uv = ParallaxOcclusionMap(uv, viewDirTangent);

            // Discard fragments outside [0,1] to avoid border artifacts
            if (uv.x < 0.0 || uv.x > UVscale.x || uv.y < 0.0 || uv.y > UVscale.y)
                discard;
        }

        // --- Sample PBR textures ---
        vec3  albedo    = pow(texture(albedoMap, uv).rgb, vec3(2.2)) * pbrTint;
        float metallic  = texture(metallicMap, uv).r;
        float roughness = clamp(texture(roughnessMap, uv).r, 0.05, 1.0);
        float ao        = texture(aoMap, uv).r;

        // Perturb normal with normal map
        N = PerturbNormal(N, fragmentPosition, uv);

        // --- Reflectance at normal incidence ---
        vec3 F0 = mix(vec3(0.04), albedo, metallic);

        // --- Hemisphere ambient ---
        vec3 ambient = HemisphereAmbient(N) * albedo * ao;

        // --- Accumulate direct lighting ---
        vec3 Lo = vec3(0.0);
        int count = max(numLights, 1);

        for (int i = 0; i < MAX_LIGHTS; ++i)
        {
            if (i >= count) break;

            vec3  lPos       = (numLights > 0) ? lightPositions[i] : lightPos;
            vec3  lColor     = (numLights > 0) ? lightColors[i]    : lightColor;
            float lIntensity = (numLights > 0) ? lightIntensities[i] : 30.0;

            vec3  L    = lPos - fragmentPosition;
            float dist = length(L);
            L = normalize(L);
            vec3 H = normalize(V + L);

            // Point-light attenuation
            float attenuation = lIntensity / (dist * dist + 1.0);
            vec3  radiance    = lColor * attenuation;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness);
            vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3  numerator   = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3  specular    = numerator / denominator;

            // Energy conservation
            vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }

        vec3 color = ambient + Lo;

        // Tone mapping (Reinhard) + gamma
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));

        outFragmentColor = vec4(color, 1.0);
        return;
    }

    // ----------------------------------------------------------------
    // PATH 2: Simple texture or flat color (with Blinn-Phong lighting)
    // ----------------------------------------------------------------
    vec3 baseColor;
    float alpha = 1.0;

    if (bUseTexture)
    {
        vec4 texSample = texture(objectTexture, fragmentTextureCoordinate * UVscale);
        baseColor = texSample.rgb;
        alpha     = texSample.a;
    }
    else
    {
        baseColor = objectColor.rgb;
        alpha     = objectColor.a;
    }

    vec3 N = normalize(fragmentVertexNormal);
    vec3 lit = BlinnPhong(baseColor, N, fragmentPosition);
    lit = lit / (lit + vec3(1.0));  // Reinhard tonemap
    lit = pow(lit, vec3(1.0 / 2.2));  // gamma

    outFragmentColor = vec4(lit, alpha);
}
