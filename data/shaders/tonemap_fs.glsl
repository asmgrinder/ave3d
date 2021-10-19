#version 450 core
// Physically Based Rendering
// * Forked from Michał Siejak PBR project

// Tone-mapping & gamma correction.

const float gamma     = 2.2;
const float exposure  = 1.0;
const float pureWhite = 1.0;

layout(location=0) in  vec2 screenPosition;
layout(binding=0) uniform sampler2D opaqueTex;
layout(binding=1) uniform sampler2D accTex;
layout(binding=2) uniform sampler2D counter;

layout(location=0) out vec4 outColor;

void main()
{
    // Order Independent Transparency (OIT), see https://developer.download.nvidia.com/SDK/10/opengl/src/dual_depth_peeling/doc/DualDepthPeeling.pdf
	vec4 Cbg = texture(opaqueTex, screenPosition);
	float cnt = texture(counter, screenPosition).r;
    if (cnt > 0)
    {
		vec4 acc = texture(accTex, screenPosition);
		vec3 C = acc.rgb / cnt;
        float A = acc.a / cnt;
        float oneMinusA_N = pow(1. - A, cnt);
        Cbg = vec4(C * (1. - oneMinusA_N) + Cbg.rgb * oneMinusA_N, 1.);
    }
	vec3 color = Cbg.rgb * exposure;

	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	outColor = vec4(pow(mappedColor, vec3(1.0/gamma)), 1.0);//vec4(vec3(cnt), 1.);//
}
