

#include "ocean2HF.hlsli"

#if 0
    #include "objectHF.hlsli"
    float4 CalculateMiplevels(float2 largestCascadeUV)
    {
        float2 uvdx = ddx(largestCascadeUV * g_windWavesTextureSizeInTexels);
        float2 uvdy = ddy(largestCascadeUV * g_windWavesTextureSizeInTexels);
        float texArea = max(dot(uvdx, uvdx), dot(uvdy, uvdy));
        float largerCascadeToSmallerCascadeMiplevelShift = log2(g_cascadeToCascadeScale);
        float4 levels;
        levels.x = 0.5 * log2(texArea);
        levels.y = levels.x + largerCascadeToSmallerCascadeMiplevelShift;
        levels.z = levels.y + largerCascadeToSmallerCascadeMiplevelShift;
        levels.w = levels.z + largerCascadeToSmallerCascadeMiplevelShift;
        return levels;
    }

    float4 SampleTextureArrayWithBicubic(Texture2DArray t, SamplerState s, float2 iTc, float slice)
    {
        iTc *= g_windWavesTextureSizeInTexels;
        float2 tc = floor(iTc - 0.5) + 0.5;

        float2 f = iTc - tc;
        float2 f2 = f * f;
        float2 f3 = f2 * f;
        float2 omf = 1.0 - f;
        float2 omf2 = omf * omf;
        float2 omf3 = omf2 * omf;

        float2 w0 = omf3 / 6.0;
        float2 w1 = (4.0 + 3.0 * f3 - 6.0 * f2) / 6.0;
        float2 w2 = (4.0 + 3.0 * omf3 - 6.0 * omf2) / 6.0;
        float2 w3 = f3 / 6.0f;

        float2 s0 = w0 + w1;
        float2 s1 = w2 + w3;

        float2 f0 = w1 / (w0 + w1);
        float2 f1 = w3 / (w2 + w3);

        float2 t0 = tc - 1.0 + f0;
        float2 t1 = tc + 1.0 + f1;

        t0 /= g_windWavesTextureSizeInTexels;
        t1 /= g_windWavesTextureSizeInTexels;

        return
		    t.SampleLevel(s, float3(t0.x, t0.y, slice), 0.0) * s0.x * s0.y +
		    t.SampleLevel(s, float3(t1.x, t0.y, slice), 0.0) * s1.x * s0.y +
		    t.SampleLevel(s, float3(t0.x, t1.y, slice), 0.0) * s0.x * s1.y +
		    t.SampleLevel(s, float3(t1.x, t1.y, slice), 0.0) * s1.x * s1.y;
    }

    float4 SampleTextureWithBicubic(Texture2D t, SamplerState s, float2 iTc)
    {
        iTc *= g_localWavesTextureSizeInTexels;
        float2 tc = floor(iTc - 0.5) + 0.5;

        float2 f = iTc - tc;
        float2 f2 = f * f;
        float2 f3 = f2 * f;
        float2 omf = 1.0 - f;
        float2 omf2 = omf * omf;
        float2 omf3 = omf2 * omf;

        float2 w0 = omf3 / 6.0;
        float2 w1 = (4.0 + 3.0 * f3 - 6.0 * f2) / 6.0;
        float2 w2 = (4.0 + 3.0 * omf3 - 6.0 * omf2) / 6.0;
        float2 w3 = f3 / 6.0f;

        float2 s0 = w0 + w1;
        float2 s1 = w2 + w3;

        float2 f0 = w1 / (w0 + w1);
        float2 f1 = w3 / (w2 + w3);

        float2 t0 = tc - 1.0 + f0;
        float2 t1 = tc + 1.0 + f1;

        t0 /= g_localWavesTextureSizeInTexels;
        t1 /= g_localWavesTextureSizeInTexels;

        return
		    t.SampleLevel(s, float2(t0.x, t0.y), 0.0) * s0.x * s0.y +
		    t.SampleLevel(s, float2(t1.x, t0.y), 0.0) * s1.x * s0.y +
		    t.SampleLevel(s, float2(t0.x, t1.y), 0.0) * s0.x * s1.y +
		    t.SampleLevel(s, float2(t1.x, t1.y), 0.0) * s1.x * s1.y;
    }

    float3 CombineMoments(float2 FirstOrderMomentsA, float2 FirstOrderMomentsB, float3 SecondOrderMomentsA, float3 SecondOrderMomentsB)
    {
        return float3(
		    SecondOrderMomentsA.x + SecondOrderMomentsB.x + 2.0 * FirstOrderMomentsA.x * FirstOrderMomentsB.x,
		    SecondOrderMomentsA.y + SecondOrderMomentsB.y + 2.0 * FirstOrderMomentsA.y * FirstOrderMomentsB.y,
		    SecondOrderMomentsA.z + SecondOrderMomentsB.z + FirstOrderMomentsA.x * FirstOrderMomentsB.y + FirstOrderMomentsA.y * FirstOrderMomentsB.x);
    }

    SURFACE_PARAMETERS GFSDK_WaveWorks_GetSurfaceParameters(DS_OUTPUT In)
    {
	    // Reading the gradient/surface folding data from texture array for wind simulation and from texture for local waves
        float4 gradFold0 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.xy, 0.0));
        float4 gradFold1 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.zw, 1.0));
        float4 gradFold2 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.xy, 2.0));
        float4 gradFold3 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.zw, 3.0));
        float4 localWavesGradFold = SampleTextureWithBicubic(g_gradientsTextureLocalWaves, g_samplerBilinearClamp, In.UVForLocalWaves);

	    // Reading the second order moments from texture array
        float3 secondOrderMoments0 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.xy, 0.0)).xyz;
        float3 secondOrderMoments1 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.zw, 1.0)).xyz;
        float3 secondOrderMoments2 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.xy, 2.0)).xyz;
        float3 secondOrderMoments3 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.zw, 3.0)).xyz;

	    // Read the lowest LOD second order moments from texture array, value 20 for miplevel will be clamped to the actual maximum miplevel for the texture
        float3 secondOrderMomentsLowestLOD0 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 0.0), 20.0).xyz;
        float3 secondOrderMomentsLowestLOD1 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 1.0), 20.0).xyz;
        float3 secondOrderMomentsLowestLOD2 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 2.0), 20.0).xyz;
        float3 secondOrderMomentsLowestLOD3 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 3.0), 20.0).xyz;

	    // Fading first order moments from bilinear filtered to bicubic filtered in case of magnification for the highest resolution cascade only.
	    // Lower resolution cascades don't suffer from this issue that much, so don't do bicubic for them to save time.
        float4 mipFade = 0.25 * CalculateMiplevels(In.UVForCascade01.xy);
        if (mipFade.w < 1.0)
            gradFold3 = lerp(SampleTextureArrayWithBicubic(g_gradientsTextureArrayWindWaves, g_samplerBilinear, In.UVForCascade23.zw, 3.0), gradFold3, max(0.0, min(1.0, mipFade.w)));

	    // Splitting data from textures to meaningfully named variables
        float2 firstOrderMoments0 = gradFold0.xy;
        float2 firstOrderMoments1 = gradFold1.xy;
        float2 firstOrderMoments2 = gradFold2.xy;
        float2 firstOrderMoments3 = gradFold3.xy;
        float2 localWavesGradients = localWavesGradFold.xy;

        float surfaceFolding0 = gradFold0.z;
        float surfaceFolding1 = gradFold1.z;
        float surfaceFolding2 = gradFold2.z;
        float surfaceFolding3 = gradFold3.z;
        float localWavesSurfaceFolding = localWavesGradFold.z;

        float foamEnergy0 = gradFold0.w;
        float foamEnergy1 = gradFold1.w;
        float foamEnergy2 = gradFold2.w;
        float foamEnergy3 = gradFold3.w;
        float localWavesFoamEnergy = localWavesGradFold.w;

	    // Fading second moments to squares of first order moments in case of magnification
	    // This is needed to fix corruptions when magnifying textures and microfacet model becomes incorrect
        secondOrderMoments0 = lerp(float3(firstOrderMoments0.x * firstOrderMoments0.x, firstOrderMoments0.y * firstOrderMoments0.y, firstOrderMoments0.x * firstOrderMoments0.y), secondOrderMoments0, max(0.0, min(1.0, mipFade.x)));
        secondOrderMoments1 = lerp(float3(firstOrderMoments1.x * firstOrderMoments1.x, firstOrderMoments1.y * firstOrderMoments1.y, firstOrderMoments1.x * firstOrderMoments1.y), secondOrderMoments1, max(0.0, min(1.0, mipFade.y)));
        secondOrderMoments2 = lerp(float3(firstOrderMoments2.x * firstOrderMoments2.x, firstOrderMoments2.y * firstOrderMoments2.y, firstOrderMoments2.x * firstOrderMoments2.y), secondOrderMoments2, max(0.0, min(1.0, mipFade.z)));
        secondOrderMoments3 = lerp(float3(firstOrderMoments3.x * firstOrderMoments3.x, firstOrderMoments3.y * firstOrderMoments3.y, firstOrderMoments3.x * firstOrderMoments3.y), secondOrderMoments3, max(0.0, min(1.0, mipFade.w)));

	    // Combining gradients (first order moments) from all the cascades and the local waves to calculate normal
        float2 gradients =
				    firstOrderMoments0 * In.cascadeBlendingFactors.x +
				    firstOrderMoments1 * In.cascadeBlendingFactors.y +
				    firstOrderMoments2 * In.cascadeBlendingFactors.z +
				    firstOrderMoments3 * In.cascadeBlendingFactors.w +
				    localWavesGradients;
        float3 normal = normalize(float3(gradients, 1.0));

	    // Calculating foam energy
        float energyC2CScale = 0.7 / sqrt(g_cascadeToCascadeScale); // larger cascades cover larger areas, so foamed texels cover larger area, thus, foam intensity on these needs to be scaled down for uniform foam look
	                                                                // also multiplying by arbitrary number 0.7 to make distant foam less intense (30% less intense on each larger cascade)

        float foamEnergy = // accumulated foam energy with blendfactors
				    100.0 * foamEnergy0 *
				    lerp(energyC2CScale, foamEnergy1, In.cascadeBlendingFactors.y) *
				    lerp(energyC2CScale, foamEnergy2, In.cascadeBlendingFactors.z) *
				    lerp(energyC2CScale, foamEnergy3, In.cascadeBlendingFactors.w) +
				    localWavesFoamEnergy;

	    // Calculating surface folding and wave hats
        float surfaceFolding =
				    max(-100.0,
				    (1.0 - surfaceFolding0) +
				    (1.0 - surfaceFolding1) +
				    (1.0 - surfaceFolding2) +
				    (1.0 - surfaceFolding3) +
				    (1.0 - localWavesSurfaceFolding));

        float waveHatsC2CScale = 0.5; // the larger is the wave, the higher is the chance to start breaking at high folding, so folding for smaller cascades is decreased
        float foamWaveHats =
				    10.0 * (-g_windWavesFoamWhitecapsThreshold + // this allows whitecaps to appear on breaking places only.
				    (1.0 - surfaceFolding0) +
				    (1.0 - surfaceFolding1) * waveHatsC2CScale +
				    (1.0 - surfaceFolding2) * waveHatsC2CScale * waveHatsC2CScale +
				    (1.0 - surfaceFolding3) * waveHatsC2CScale * waveHatsC2CScale * waveHatsC2CScale +
				    (1.0 - localWavesSurfaceFolding - g_localWavesFoamWhitecapsThreshold));


        float4 cascadeBlendingFactorsSquared = In.cascadeBlendingFactors * In.cascadeBlendingFactors;

	    // Fading second order moments to lowest LOD (single values) where cascades fade out
	    // to minimize visible repeats on microfacet specular
        secondOrderMoments0 = cascadeBlendingFactorsSquared.x * secondOrderMoments0 + (1.0 - cascadeBlendingFactorsSquared.x) * secondOrderMomentsLowestLOD0;
        secondOrderMoments1 = cascadeBlendingFactorsSquared.y * secondOrderMoments1 + (1.0 - cascadeBlendingFactorsSquared.y) * secondOrderMomentsLowestLOD1;
        secondOrderMoments2 = cascadeBlendingFactorsSquared.z * secondOrderMoments2 + (1.0 - cascadeBlendingFactorsSquared.z) * secondOrderMomentsLowestLOD2;
        secondOrderMoments3 = cascadeBlendingFactorsSquared.w * secondOrderMoments3 + (1.0 - cascadeBlendingFactorsSquared.w) * secondOrderMomentsLowestLOD3;

	    // Fading first order moments to zero where cascades fade out
        firstOrderMoments0 *= In.cascadeBlendingFactors.x;
        firstOrderMoments1 *= In.cascadeBlendingFactors.y;
        firstOrderMoments2 *= In.cascadeBlendingFactors.z;
        firstOrderMoments3 *= In.cascadeBlendingFactors.w;

	    // Combining first and second order moments from all 4 cascades, step by step
        float2 firstOrderMoments01 = firstOrderMoments0 + firstOrderMoments1;
        float3 secondOrderMoments01 = CombineMoments(firstOrderMoments0, firstOrderMoments1, secondOrderMoments0, secondOrderMoments1);

        float2 firstOrderMoments012 = firstOrderMoments01 + firstOrderMoments2;
        float3 secondOrderMoments012 = CombineMoments(firstOrderMoments01, firstOrderMoments2, secondOrderMoments01, secondOrderMoments2);

        float2 firstOrderMoments0123 = firstOrderMoments012 + firstOrderMoments3;
        float3 secondOrderMoments0123 = CombineMoments(firstOrderMoments012, firstOrderMoments3, secondOrderMoments012, secondOrderMoments3);

	    // Combining with first order moments of local waves, taking squares of first order moments of local waves as second order moments of local waves
        float2 firstOrderMoments0123L = firstOrderMoments0123 + localWavesGradients;
        float3 secondOrderMoments0123L = CombineMoments(firstOrderMoments0123, localWavesGradients, secondOrderMoments0123, float3(localWavesGradients.xy * localWavesGradients.xy, localWavesGradients.x * localWavesGradients.y));

	    // Combining lowest LOD second order moments, fading in where simulation cascades fade out
        float2 secondOrderMomentsLowestLOD =
				    secondOrderMomentsLowestLOD0.xy * (1.0 - cascadeBlendingFactorsSquared.x) +
				    secondOrderMomentsLowestLOD1.xy * (1.0 - cascadeBlendingFactorsSquared.y) +
				    secondOrderMomentsLowestLOD2.xy * (1.0 - cascadeBlendingFactorsSquared.z) +
				    secondOrderMomentsLowestLOD3.xy * (1.0 - cascadeBlendingFactorsSquared.w) +
				    localWavesGradients * localWavesGradients;

	    // Output
        SURFACE_PARAMETERS Output;

        Output.normal = normal;
        Output.foamSurfaceFolding = surfaceFolding;
        Output.foamEnergy = foamEnergy;
        Output.foamWaveHats = foamWaveHats;
        Output.firstOrderMoments = firstOrderMoments0123L;
        Output.secondOrderMomentsLowestLOD = secondOrderMomentsLowestLOD;
        Output.secondOrderMoments = secondOrderMoments0123L;
        return Output;
    }

    // Roughness projected to particular direction in worldspace, assuming the moments frame is worldspace, z up
    float ProjectedRoughness(float3 Direction, float2 Roughness)
    {
	    // Simplified case, assuming the distribution of slopes is centered
        return sqrt(dot(Roughness, Direction.xy * Direction.xy));
    }

    // Walter's approximation
    float LambdaApprox(float a)
    {
        if (a < 1.6)
        {
            return (1.0 - 1.259 * a + 0.396 * a * a) / (3.535 * a + 2.181 * a * a);
        }
        else
        {
            return 0;
        }
    }

    // Smith's Masking-shadowing factor 
    float SmithMasking(float3 D, float3 N, float3 secondOrderMoments)
    {

        float cos = abs(dot(D, N)); // abs() is hack that allows to gracefully handle pixels with normals backfaced to light
        float tan = sqrt(1.0 - cos * cos) / cos;
        float a = 1.0 / (tan * ProjectedRoughness(D, secondOrderMoments.xy));
        return LambdaApprox(a);
    }

    // Shlick's approximation for Ross BRDF
    float EffectiveFresnel(float3 V, float3 N, float2 secondOrderMomentsLowestLOD)
    {
        float r = (1.0 - 1.33) * (1.0 - 1.33) / ((1.0 + 1.33) * (1.0 + 1.33));
        float s = ProjectedRoughness(V, secondOrderMomentsLowestLOD);
        if (g_useMicrofacetFresnel > 0)
        {
            return r + (1.0 - r) * pow(1.0 - dot(N, V), 5.0 * exp(-2.69 * s)) / (1.0 + 22.7 * pow(s, 1.5));
        }
        else
        {
            return r + (1.0 - r) * pow(1.0 - dot(N, V), 5.0);
        }
    }

    float DirectionPDF(float2 slope, float2 firstOrderMoments, float3 secondOrderMoments)
    {
        float2 s = slope - firstOrderMoments;
        float3 E = secondOrderMoments - float3(firstOrderMoments.x * firstOrderMoments.x, firstOrderMoments.y * firstOrderMoments.y, firstOrderMoments.x * firstOrderMoments.y);
        float det = E.x * E.y - E.z * E.z;
        float e = -0.5 * (s.x * s.x * E.y + s.y * s.y * E.x - 2.0 * s.x * s.y * E.z);
        return (det <= 0) ? 0 : exp(e / det) / (2.0 * PI * sqrt(det));
    }

    float Distribution(float3 w, float2 firstOrderMoments, float3 secondOrderMoments)
    {
        float2 slope = w.xy / w.z;
        return DirectionPDF(slope, firstOrderMoments, secondOrderMoments);
    }

    float MicrofacetSpecular(float3 L, float3 V, float3 N, float2 firstOrderMoments, float3 secondOrderMoments)
    {
        float3 H = normalize(L + V);
        float F = EffectiveFresnel(V, N, secondOrderMoments.xy);
        float D = Distribution(H, firstOrderMoments, secondOrderMoments);
        float cosV = max(0.0001, abs(V.z)); // we only care about microfacet properties of water when normal degrades to vertical due to mipmapping,  
										    // abs() is hack that allows to render specular waves higher than camera position more or less correctly
        float I = (1.0 / (4.0 * cosV)) * F * D / (1.0 + SmithMasking(L, float3(0.0, 0.0, 1.0), secondOrderMoments) + SmithMasking(V, float3(0.0, 0.0, 1.0), secondOrderMoments));
        return I;
    }

    // Quick and dirty atmospherics scattering calculation (need to use it for distant ocean)
    #define Kr float3(5.8e-6, 13.5e-6, 33.1e-6)
    #define Km 1e-4 // using Mie scatter 10x higher than normal for better looks of foggy ocean and since we're using fake math anyway (not tracing atmosphere for eye->ocean direction)
    #define G 0.88
    #define PlanetRadius 6360000.0 // meters
    #define AverageRayleighScatterHeight 8000.0 // meters
    #define AverageMieScatterHeight 1200.0 // meters

    float OpticalDepth(float H, float r, float mu, float d)
    {
        float a = sqrt((0.5 / H) * r);
        float2 a01 = a * float2(mu, mu + d / r);
        float2 a01s = sign(a01);
        float2 a01sq = a01 * a01;
        float x = a01s.y > a01s.x ? exp(a01sq.x) : 0.0;
        float2 y = a01s / (2.3193 * abs(a01) + sqrt(1.52 * a01sq + 4.0)) * float2(1.0, exp(-d / H * (d / (2.0 * r) + mu)));
        return sqrt((6.2831 * H) * r) * exp((PlanetRadius - r) / H) * (x + dot(y, float2(1.0, -1.0)));
    }

    float3 AnalyticTransmittance(float r, float mu, float d)
    {
        return exp(-Kr * OpticalDepth(AverageRayleighScatterHeight, r, mu, d) - Km * OpticalDepth(AverageMieScatterHeight, r, mu, d));
    }

    // Using "Filming Tonemapping"
    float3 ToneMapping(float3 color)
    {
        float A = 0.22;
        float B = 0.30;
        float C = 0.1;
        float D = 0.2;
        float E = 0.01;
        float F = 0.3;
        return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    }


    float2 ClampUV(float2 uv)
    {
        return max(0.0, min(1.0, uv));
    }

    #define NumSamples 5
    #define SampleLength 0.9 // length of a sample in slope space

    // Based on environment reflection code from LEADR paper
    float3 MicrofacetReflection(float3 V, float3 N, float2 firstOrderMoments, float3 secondOrderMoments)
    {
        float3 TSX = float3(1.0, 0.0, 0.0);
        float3 TSY = float3(0.0, 1.0, 0.0);
        float3 TSZ = float3(0.0, 0.0, 1.0);
        float sigma_x = sqrt(max(0.0, secondOrderMoments.x - firstOrderMoments.x * firstOrderMoments.x));
        float sigma_y = sqrt(max(0.0, secondOrderMoments.y - firstOrderMoments.y * firstOrderMoments.y));
        float r_xy = max(0.001, secondOrderMoments.z - firstOrderMoments.x * firstOrderMoments.y) / (sigma_x * sigma_y + 0.001);
        float r_xy_ = sqrt(max(0.001, 1.0 - r_xy * r_xy));
        float LODoffset = -1.0 + log2(SampleLength * (0.5 / 1.5) * max(sigma_x, sigma_y) * 2048.0);

        float S = 0.0;
        float3 I = 0.0;

        for (int i = 0; i < NumSamples; i++)
        {
            for (int j = 0; j < NumSamples; j++)
            {
                float pi = -1.8 + SampleLength * i;
                float pj = -1.8 + SampleLength * j;
                float wi = exp(-0.5 * pi * pi);
                float wj = exp(-0.5 * pj * pj);

			    // microfacet slope
                float x = -firstOrderMoments.x + pi * sigma_x;
                float y = -firstOrderMoments.y + (pi * r_xy + pj * r_xy_) * sigma_y;

			    // microfacet normal
                float3 NN = normalize(-x * TSX - y * TSY + TSZ);

			    // microfacet projected area
                float VdotNN = max(0.001, dot(V, NN));
                float ZdotNN = 1; // dot(TSZ, NN);
                float S_ = VdotNN / ZdotNN;
                float3 R = reflect(-V, NN);
                float J = abs(4.0 * ZdotNN * ZdotNN * ZdotNN * VdotNN);
                float LOD = 0.72 * log(max(0.0001, J * (0.5 / 1.5))) + LODoffset;

			    // LOD correction due to our UV distortion
                LOD += (4.0 / ((1.0 + R.z) * (1.0 + R.z)));

                float2 uv = R.xy / (1.0 + R.z);
            float3 I_ = 1.0;//g_textureDynamicSkyDome.SampleLevel(g_samplerBilinear, ClampUV(uv * (0.5 / 1.5) + 0.5), LOD).xyz;

                S += wi * wj * S_;
                I += wi * wj * S_ * I_;
            }
        }
        return I / S;
    }

    float GetFoam(float3 worldspacePosition, SURFACE_PARAMETERS surfParams)
    {
	    // Physically correct simulation of surface foam is barely possible in realtime,
	    // so we use foam turbulent energy we get from simulation, surface folding, and a bunch of arbitrary numbers that give plausible look
	
	    // Using 3 octaves of foam density texture and foam bubbles texture
        float foamDensityMapLowFrequency = g_textureFoam.Sample(g_samplerBilinear, worldspacePosition.xy * 0.04).x - 1.0;
        float foamDensityMapHighFrequency = g_textureFoam.Sample(g_samplerBilinear, worldspacePosition.xy * 0.15).x - 1.0;
        float foamDensityMapVeryHighFrequency = g_textureFoam.Sample(g_samplerBilinear, worldspacePosition.xy * 0.3).x;
        float foamBubbles = g_textureBubbles.Sample(g_samplerAnisotropic, worldspacePosition.xy * 0.25).r;

	    // Combining 2 octaves of foam density texture
        float foamDensity;
        foamDensity = saturate(foamDensityMapHighFrequency + min(3.5, 1.0 * surfParams.foamEnergy - 0.2));
        foamDensity += (foamDensityMapLowFrequency + min(1.5, 1.0 * surfParams.foamEnergy));

	    // Applying surface folding/stretching so foam is thicker in folded areas and thinner in stretched areas
        foamDensity -= 0.1 * saturate(-surfParams.foamSurfaceFolding);
        foamDensity = max(0, foamDensity);
        foamDensity *= 1.0 + 0.8 * saturate(surfParams.foamSurfaceFolding);

	    // Adding foam on wave hats 
        foamDensity += max(0, foamDensityMapVeryHighFrequency * 2.0 * surfParams.foamWaveHats);
        foamDensity = pow(foamDensity, 0.7);

	    // Modulating by foam bubbles to get very detailed look
        foamBubbles = saturate(5.0 * (foamBubbles - 0.8));
        foamDensity = saturate(foamDensity * foamBubbles);
        return foamDensity;
    }

    // ---------------------------------------------------------------------------
    // Pixel shader
    // ---------------------------------------------------------------------------
    float4 main(DS_OUTPUT In) : SV_Target
    {
        

	    // Calculating surface parameters and foam density
        SURFACE_PARAMETERS surfaceParameters = GFSDK_WaveWorks_GetSurfaceParameters(In);
        float foamIntensity = GetFoam(In.worldspacePositionUndisplaced, surfaceParameters);

     
    float3 eyeVec = g_eyePos - In.worldspacePositionUndisplaced;
        float dist = length(eyeVec);
        float3 V = normalize(eyeVec);
        float3 N = surfaceParameters.normal;
        float3 R = reflect(-V, N);

	    // Bending backfacing normals if we encounter those
    #if 0
        float NdotV = dot(N, V);
        if (NdotV < 0.01)
        {
            N += V * (-NdotV + 0.01);
            N = -normalize(N);
        }
    #endif

        float4 color = g_WaterColor;

	  
        Surface surface;
        surface.init();
        surface.albedo = color.rgb;
        surface.f0 = 0.02;
        surface.roughness = 0.1;
    surface.P = In.worldspacePositionUndisplaced;
        surface.N = N;
        surface.V = V;
        surface.update();

        Lighting lighting;
        lighting.create(0.5, 0, GetAmbient(surface.N), 0);

    
        surface.pixel = In.clipSpacePosition.xy;
        float depth = In.clipSpacePosition.z;

    #if 1
        TiledLighting(surface, lighting);
    #endif
    
        float4 ReflectionMapSamplingPos = mul(GetCamera().reflection_view_projection, float4(surface.P, 1));

        float2 refUV = float2(1, -1) * ReflectionMapSamplingPos.xy / ReflectionMapSamplingPos.w * 0.5f + 0.5f;
        float2 ScreenCoord = surface.pixel * GetCamera().internal_resolution_rcp;

    
    #if 0
	    [branch]
        if (GetCamera().texture_reflection_index >= 0)
        {
		    //REFLECTION
            float2 RefTex = float2(1, -1) * ReflectionMapSamplingPos.xy / ReflectionMapSamplingPos.w / 2.0f + 0.5f;
            float4 reflectiveColor = bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_mirror, RefTex + surface.N.xz * 0.04f, 0);
            lighting.indirect.specular = reflectiveColor.rgb * surface.F;
        }

    
    #endif

    #if 0
	    [branch]
    if (GetCamera().texture_refraction_index >= 0)
    {
		    // WATER REFRACTION 
        Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
        float lineardepth = In.clipSpacePosition.w;
        float sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy + surface.N.xz * 0.04f, 0) * GetCamera().z_far;
        float depth_difference = sampled_lineardepth - lineardepth;
        surface.refraction.rgb = texture_refraction.SampleLevel(sampler_linear_mirror, ScreenCoord.xy + surface.N.xz * 0.04f * saturate(0.5 * depth_difference), 0).rgb;
        if (depth_difference < 0)
        {
			    // Fix cutoff by taking unperturbed depth diff to fill the holes with fog:
            sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy, 0) * GetCamera().z_far;
            depth_difference = sampled_lineardepth - lineardepth;
        }
		    // WATER FOG:
            
            //surface.refraction.a = 1 - saturate(color.a * 0.1f * depth_difference);
            
    }
    #endif
    
	    // Blend out at distance:
        color.a = pow(saturate(1 - saturate(dist / GetCamera().z_far)), 2);

        ApplyLighting(surface, lighting, color);

        //ApplyFog(dist, GetCamera().position, V, color);

    
    color.rgb = lerp(color.rgb, g_FoamColor, foamIntensity);

 
	    // Showing cascade edges if needed
    #if 0
        if (g_showCascades > 0)
        {
            if (frac(In.UVForCascade01.x) < 0.05)
                color.r += 0.25;
            if (frac(In.UVForCascade01.y) < 0.05)
                 color.r += 0.25;

            if (frac(In.UVForCascade01.z) < 0.05)
                color.g += 0.25;
            if (frac(In.UVForCascade01.w) < 0.05)
             color.g += 0.25;

            if (frac(In.UVForCascade23.x) < 0.05)
                color.b += 0.5;
            if (frac(In.UVForCascade23.y) < 0.05)
                color.b += 0.5;

            if (frac(In.UVForCascade23.z) < 0.05)
                color.rgb += 0.25;
            if (frac(In.UVForCascade23.w) < 0.05)
                color.rgb += 0.25;
    }
    #endif
        
    return float4(color.rgb, 1);
}

#else
//float4 main() : SV_TARGET
//{
 //  return g_WaterColor;
//}



// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

float4 CalculateMiplevels(float2 largestCascadeUV)
{
    float2 uvdx = ddx(largestCascadeUV * g_windWavesTextureSizeInTexels);
    float2 uvdy = ddy(largestCascadeUV * g_windWavesTextureSizeInTexels);
    float texArea = max(dot(uvdx, uvdx), dot(uvdy, uvdy));
    float largerCascadeToSmallerCascadeMiplevelShift = log2(g_cascadeToCascadeScale);
    float4 levels;
    levels.x = 0.5 * log2(texArea);
    levels.y = levels.x + largerCascadeToSmallerCascadeMiplevelShift;
    levels.z = levels.y + largerCascadeToSmallerCascadeMiplevelShift;
    levels.w = levels.z + largerCascadeToSmallerCascadeMiplevelShift;
    return levels;
}

float4 SampleTextureArrayWithBicubic(Texture2DArray t, SamplerState s, float2 iTc, float slice)
{
    iTc *= g_windWavesTextureSizeInTexels;
    float2 tc = floor(iTc - 0.5) + 0.5;

    float2 f = iTc - tc;
    float2 f2 = f * f;
    float2 f3 = f2 * f;
    float2 omf = 1.0 - f;
    float2 omf2 = omf * omf;
    float2 omf3 = omf2 * omf;

    float2 w0 = omf3 / 6.0;
    float2 w1 = (4.0 + 3.0 * f3 - 6.0 * f2) / 6.0;
    float2 w2 = (4.0 + 3.0 * omf3 - 6.0 * omf2) / 6.0;
    float2 w3 = f3 / 6.0f;

    float2 s0 = w0 + w1;
    float2 s1 = w2 + w3;

    float2 f0 = w1 / (w0 + w1);
    float2 f1 = w3 / (w2 + w3);

    float2 t0 = tc - 1.0 + f0;
    float2 t1 = tc + 1.0 + f1;

    t0 /= g_windWavesTextureSizeInTexels;
    t1 /= g_windWavesTextureSizeInTexels;

    return
		t.SampleLevel(s, float3(t0.x, t0.y, slice), 0.0) * s0.x * s0.y +
		t.SampleLevel(s, float3(t1.x, t0.y, slice), 0.0) * s1.x * s0.y +
		t.SampleLevel(s, float3(t0.x, t1.y, slice), 0.0) * s0.x * s1.y +
		t.SampleLevel(s, float3(t1.x, t1.y, slice), 0.0) * s1.x * s1.y;
}

float4 SampleTextureWithBicubic(Texture2D t, SamplerState s, float2 iTc)
{
    iTc *= g_localWavesTextureSizeInTexels;
    float2 tc = floor(iTc - 0.5) + 0.5;

    float2 f = iTc - tc;
    float2 f2 = f * f;
    float2 f3 = f2 * f;
    float2 omf = 1.0 - f;
    float2 omf2 = omf * omf;
    float2 omf3 = omf2 * omf;

    float2 w0 = omf3 / 6.0;
    float2 w1 = (4.0 + 3.0 * f3 - 6.0 * f2) / 6.0;
    float2 w2 = (4.0 + 3.0 * omf3 - 6.0 * omf2) / 6.0;
    float2 w3 = f3 / 6.0f;

    float2 s0 = w0 + w1;
    float2 s1 = w2 + w3;

    float2 f0 = w1 / (w0 + w1);
    float2 f1 = w3 / (w2 + w3);

    float2 t0 = tc - 1.0 + f0;
    float2 t1 = tc + 1.0 + f1;

    t0 /= g_localWavesTextureSizeInTexels;
    t1 /= g_localWavesTextureSizeInTexels;

    return
		t.SampleLevel(s, float2(t0.x, t0.y), 0.0) * s0.x * s0.y +
		t.SampleLevel(s, float2(t1.x, t0.y), 0.0) * s1.x * s0.y +
		t.SampleLevel(s, float2(t0.x, t1.y), 0.0) * s0.x * s1.y +
		t.SampleLevel(s, float2(t1.x, t1.y), 0.0) * s1.x * s1.y;
}

float3 CombineMoments(float2 FirstOrderMomentsA, float2 FirstOrderMomentsB, float3 SecondOrderMomentsA, float3 SecondOrderMomentsB)
{
    return float3(
		SecondOrderMomentsA.x + SecondOrderMomentsB.x + 2.0 * FirstOrderMomentsA.x * FirstOrderMomentsB.x,
		SecondOrderMomentsA.y + SecondOrderMomentsB.y + 2.0 * FirstOrderMomentsA.y * FirstOrderMomentsB.y,
		SecondOrderMomentsA.z + SecondOrderMomentsB.z + FirstOrderMomentsA.x * FirstOrderMomentsB.y + FirstOrderMomentsA.y * FirstOrderMomentsB.x);
}

SURFACE_PARAMETERS GFSDK_WaveWorks_GetSurfaceParameters(DS_OUTPUT In)
{
	// Reading the gradient/surface folding data from texture array for wind simulation and from texture for local waves
    float4 gradFold0 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.xy, 0.0));
    float4 gradFold1 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.zw, 1.0));
    float4 gradFold2 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.xy, 2.0));
    float4 gradFold3 = g_gradientsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.zw, 3.0));
    float4 localWavesGradFold = SampleTextureWithBicubic(g_gradientsTextureLocalWaves, g_samplerBilinearClamp, In.UVForLocalWaves);

	// Reading the second order moments from texture array
    float3 secondOrderMoments0 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.xy, 0.0)).xyz;
    float3 secondOrderMoments1 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade01.zw, 1.0)).xyz;
    float3 secondOrderMoments2 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.xy, 2.0)).xyz;
    float3 secondOrderMoments3 = g_momentsTextureArrayWindWaves.Sample(g_samplerAnisotropic, float3(In.UVForCascade23.zw, 3.0)).xyz;

	// Read the lowest LOD second order moments from texture array, value 20 for miplevel will be clamped to the actual maximum miplevel for the texture
    float3 secondOrderMomentsLowestLOD0 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 0.0), 20.0).xyz;
    float3 secondOrderMomentsLowestLOD1 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 1.0), 20.0).xyz;
    float3 secondOrderMomentsLowestLOD2 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 2.0), 20.0).xyz;
    float3 secondOrderMomentsLowestLOD3 = g_momentsTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(0.0, 0.0, 3.0), 20.0).xyz;

	// Fading first order moments from bilinear filtered to bicubic filtered in case of magnification for the highest resolution cascade only.
	// Lower resolution cascades don't suffer from this issue that much, so don't do bicubic for them to save time.
    float4 mipFade = 0.25 * CalculateMiplevels(In.UVForCascade01.xy);
    if (mipFade.w < 1.0)
        gradFold3 = lerp(SampleTextureArrayWithBicubic(g_gradientsTextureArrayWindWaves, g_samplerBilinear, In.UVForCascade23.zw, 3.0), gradFold3, max(0.0, min(1.0, mipFade.w)));

	// Splitting data from textures to meaningfully named variables
    float2 firstOrderMoments0 = gradFold0.xy;
    float2 firstOrderMoments1 = gradFold1.xy;
    float2 firstOrderMoments2 = gradFold2.xy;
    float2 firstOrderMoments3 = gradFold3.xy;
    float2 localWavesGradients = localWavesGradFold.xy;

    float surfaceFolding0 = gradFold0.z;
    float surfaceFolding1 = gradFold1.z;
    float surfaceFolding2 = gradFold2.z;
    float surfaceFolding3 = gradFold3.z;
    float localWavesSurfaceFolding = localWavesGradFold.z;

    float foamEnergy0 = gradFold0.w;
    float foamEnergy1 = gradFold1.w;
    float foamEnergy2 = gradFold2.w;
    float foamEnergy3 = gradFold3.w;
    float localWavesFoamEnergy = localWavesGradFold.w;

	// Fading second moments to squares of first order moments in case of magnification
	// This is needed to fix corruptions when magnifying textures and microfacet model becomes incorrect
    secondOrderMoments0 = lerp(float3(firstOrderMoments0.x * firstOrderMoments0.x, firstOrderMoments0.y * firstOrderMoments0.y, firstOrderMoments0.x * firstOrderMoments0.y), secondOrderMoments0, max(0.0, min(1.0, mipFade.x)));
    secondOrderMoments1 = lerp(float3(firstOrderMoments1.x * firstOrderMoments1.x, firstOrderMoments1.y * firstOrderMoments1.y, firstOrderMoments1.x * firstOrderMoments1.y), secondOrderMoments1, max(0.0, min(1.0, mipFade.y)));
    secondOrderMoments2 = lerp(float3(firstOrderMoments2.x * firstOrderMoments2.x, firstOrderMoments2.y * firstOrderMoments2.y, firstOrderMoments2.x * firstOrderMoments2.y), secondOrderMoments2, max(0.0, min(1.0, mipFade.z)));
    secondOrderMoments3 = lerp(float3(firstOrderMoments3.x * firstOrderMoments3.x, firstOrderMoments3.y * firstOrderMoments3.y, firstOrderMoments3.x * firstOrderMoments3.y), secondOrderMoments3, max(0.0, min(1.0, mipFade.w)));

	// Combining gradients (first order moments) from all the cascades and the local waves to calculate normal
    float2 gradients =
				firstOrderMoments0 * In.cascadeBlendingFactors.x +
				firstOrderMoments1 * In.cascadeBlendingFactors.y +
				firstOrderMoments2 * In.cascadeBlendingFactors.z +
				firstOrderMoments3 * In.cascadeBlendingFactors.w +
				localWavesGradients;
    float3 normal = normalize(float3(gradients, 1.0));

	// Calculating foam energy
    float energyC2CScale = 0.7 / sqrt(g_cascadeToCascadeScale); // larger cascades cover larger areas, so foamed texels cover larger area, thus, foam intensity on these needs to be scaled down for uniform foam look
	                                                            // also multiplying by arbitrary number 0.7 to make distant foam less intense (30% less intense on each larger cascade)

    float foamEnergy = // accumulated foam energy with blendfactors
				100.0 * foamEnergy0 *
				lerp(energyC2CScale, foamEnergy1, In.cascadeBlendingFactors.y) *
				lerp(energyC2CScale, foamEnergy2, In.cascadeBlendingFactors.z) *
				lerp(energyC2CScale, foamEnergy3, In.cascadeBlendingFactors.w) +
				localWavesFoamEnergy;

	// Calculating surface folding and wave hats
    float surfaceFolding =
				max(-100.0,
				(1.0 - surfaceFolding0) +
				(1.0 - surfaceFolding1) +
				(1.0 - surfaceFolding2) +
				(1.0 - surfaceFolding3) +
				(1.0 - localWavesSurfaceFolding));

    float waveHatsC2CScale = 0.5; // the larger is the wave, the higher is the chance to start breaking at high folding, so folding for smaller cascades is decreased
    float foamWaveHats =
				10.0 * (-g_windWavesFoamWhitecapsThreshold + // this allows whitecaps to appear on breaking places only.
				(1.0 - surfaceFolding0) +
				(1.0 - surfaceFolding1) * waveHatsC2CScale +
				(1.0 - surfaceFolding2) * waveHatsC2CScale * waveHatsC2CScale +
				(1.0 - surfaceFolding3) * waveHatsC2CScale * waveHatsC2CScale * waveHatsC2CScale +
				(1.0 - localWavesSurfaceFolding - g_localWavesFoamWhitecapsThreshold));


    float4 cascadeBlendingFactorsSquared = In.cascadeBlendingFactors * In.cascadeBlendingFactors;

	// Fading second order moments to lowest LOD (single values) where cascades fade out
	// to minimize visible repeats on microfacet specular
    secondOrderMoments0 = cascadeBlendingFactorsSquared.x * secondOrderMoments0 + (1.0 - cascadeBlendingFactorsSquared.x) * secondOrderMomentsLowestLOD0;
    secondOrderMoments1 = cascadeBlendingFactorsSquared.y * secondOrderMoments1 + (1.0 - cascadeBlendingFactorsSquared.y) * secondOrderMomentsLowestLOD1;
    secondOrderMoments2 = cascadeBlendingFactorsSquared.z * secondOrderMoments2 + (1.0 - cascadeBlendingFactorsSquared.z) * secondOrderMomentsLowestLOD2;
    secondOrderMoments3 = cascadeBlendingFactorsSquared.w * secondOrderMoments3 + (1.0 - cascadeBlendingFactorsSquared.w) * secondOrderMomentsLowestLOD3;

	// Fading first order moments to zero where cascades fade out
    firstOrderMoments0 *= In.cascadeBlendingFactors.x;
    firstOrderMoments1 *= In.cascadeBlendingFactors.y;
    firstOrderMoments2 *= In.cascadeBlendingFactors.z;
    firstOrderMoments3 *= In.cascadeBlendingFactors.w;

	// Combining first and second order moments from all 4 cascades, step by step
    float2 firstOrderMoments01 = firstOrderMoments0 + firstOrderMoments1;
    float3 secondOrderMoments01 = CombineMoments(firstOrderMoments0, firstOrderMoments1, secondOrderMoments0, secondOrderMoments1);

    float2 firstOrderMoments012 = firstOrderMoments01 + firstOrderMoments2;
    float3 secondOrderMoments012 = CombineMoments(firstOrderMoments01, firstOrderMoments2, secondOrderMoments01, secondOrderMoments2);

    float2 firstOrderMoments0123 = firstOrderMoments012 + firstOrderMoments3;
    float3 secondOrderMoments0123 = CombineMoments(firstOrderMoments012, firstOrderMoments3, secondOrderMoments012, secondOrderMoments3);

	// Combining with first order moments of local waves, taking squares of first order moments of local waves as second order moments of local waves
    float2 firstOrderMoments0123L = firstOrderMoments0123 + localWavesGradients;
    float3 secondOrderMoments0123L = CombineMoments(firstOrderMoments0123, localWavesGradients, secondOrderMoments0123, float3(localWavesGradients.xy * localWavesGradients.xy, localWavesGradients.x * localWavesGradients.y));

	// Combining lowest LOD second order moments, fading in where simulation cascades fade out
    float2 secondOrderMomentsLowestLOD =
				secondOrderMomentsLowestLOD0.xy * (1.0 - cascadeBlendingFactorsSquared.x) +
				secondOrderMomentsLowestLOD1.xy * (1.0 - cascadeBlendingFactorsSquared.y) +
				secondOrderMomentsLowestLOD2.xy * (1.0 - cascadeBlendingFactorsSquared.z) +
				secondOrderMomentsLowestLOD3.xy * (1.0 - cascadeBlendingFactorsSquared.w) +
				localWavesGradients * localWavesGradients;

	// Output
    SURFACE_PARAMETERS Output;

    Output.normal = normal;
    Output.foamSurfaceFolding = surfaceFolding;
    Output.foamEnergy = foamEnergy;
    Output.foamWaveHats = foamWaveHats;
    Output.firstOrderMoments = firstOrderMoments0123L;
    Output.secondOrderMomentsLowestLOD = secondOrderMomentsLowestLOD;
    Output.secondOrderMoments = secondOrderMoments0123L;
    return Output;
}

// Roughness projected to particular direction in worldspace, assuming the moments frame is worldspace, z up
float ProjectedRoughness(float3 Direction, float2 Roughness)
{
	// Simplified case, assuming the distribution of slopes is centered
    return sqrt(dot(Roughness, Direction.xy * Direction.xy));
}

// Walter's approximation
float LambdaApprox(float a)
{
    if (a < 1.6)
    {
        return (1.0 - 1.259 * a + 0.396 * a * a) / (3.535 * a + 2.181 * a * a);
    }
    else
    {
        return 0;
    }
}

// Smith's Masking-shadowing factor 
float SmithMasking(float3 D, float3 N, float3 secondOrderMoments)
{

    float cos = abs(dot(D, N)); // abs() is hack that allows to gracefully handle pixels with normals backfaced to light
    float tan = sqrt(1.0 - cos * cos) / cos;
    float a = 1.0 / (tan * ProjectedRoughness(D, secondOrderMoments.xy));
    return LambdaApprox(a);
}

// Shlick's approximation for Ross BRDF
float EffectiveFresnel(float3 V, float3 N, float2 secondOrderMomentsLowestLOD)
{
    float r = (1.0 - 1.33) * (1.0 - 1.33) / ((1.0 + 1.33) * (1.0 + 1.33));
    float s = ProjectedRoughness(V, secondOrderMomentsLowestLOD);
    if (g_useMicrofacetFresnel > 0)
    {
        return r + (1.0 - r) * pow(1.0 - dot(N, V), 5.0 * exp(-2.69 * s)) / (1.0 + 22.7 * pow(s, 1.5));
    }
    else
    {
        return r + (1.0 - r) * pow(1.0 - dot(N, V), 5.0);
    }
}

float DirectionPDF(float2 slope, float2 firstOrderMoments, float3 secondOrderMoments)
{
    float2 s = slope - firstOrderMoments;
    float3 E = secondOrderMoments - float3(firstOrderMoments.x * firstOrderMoments.x, firstOrderMoments.y * firstOrderMoments.y, firstOrderMoments.x * firstOrderMoments.y);
    float det = E.x * E.y - E.z * E.z;
    float e = -0.5 * (s.x * s.x * E.y + s.y * s.y * E.x - 2.0 * s.x * s.y * E.z);
    return (det <= 0) ? 0 : exp(e / det) / (2.0 * PI * sqrt(det));
}

float Distribution(float3 w, float2 firstOrderMoments, float3 secondOrderMoments)
{
    float2 slope = w.xy / w.z;
    return DirectionPDF(slope, firstOrderMoments, secondOrderMoments);
}

float MicrofacetSpecular(float3 L, float3 V, float3 N, float2 firstOrderMoments, float3 secondOrderMoments)
{
    float3 H = normalize(L + V);
    float F = EffectiveFresnel(V, N, secondOrderMoments.xy);
    float D = Distribution(H, firstOrderMoments, secondOrderMoments);
    float cosV = max(0.0001, abs(V.z)); // we only care about microfacet properties of water when normal degrades to vertical due to mipmapping,  
										// abs() is hack that allows to render specular waves higher than camera position more or less correctly
    float I = (1.0 / (4.0 * cosV)) * F * D / (1.0 + SmithMasking(L, float3(0.0, 0.0, 1.0), secondOrderMoments) + SmithMasking(V, float3(0.0, 0.0, 1.0), secondOrderMoments));
    return I;
}

// Quick and dirty atmospherics scattering calculation (need to use it for distant ocean)
#define Kr float3(5.8e-6, 13.5e-6, 33.1e-6)
#define Km 1e-4 // using Mie scatter 10x higher than normal for better looks of foggy ocean and since we're using fake math anyway (not tracing atmosphere for eye->ocean direction)
#define G 0.88
#define PlanetRadius 6360000.0 // meters
#define AverageRayleighScatterHeight 8000.0 // meters
#define AverageMieScatterHeight 1200.0 // meters

float OpticalDepth(float H, float r, float mu, float d)
{
    float a = sqrt((0.5 / H) * r);
    float2 a01 = a * float2(mu, mu + d / r);
    float2 a01s = sign(a01);
    float2 a01sq = a01 * a01;
    float x = a01s.y > a01s.x ? exp(a01sq.x) : 0.0;
    float2 y = a01s / (2.3193 * abs(a01) + sqrt(1.52 * a01sq + 4.0)) * float2(1.0, exp(-d / H * (d / (2.0 * r) + mu)));
    return sqrt((6.2831 * H) * r) * exp((PlanetRadius - r) / H) * (x + dot(y, float2(1.0, -1.0)));
}

float3 AnalyticTransmittance(float r, float mu, float d)
{
    return exp(-Kr * OpticalDepth(AverageRayleighScatterHeight, r, mu, d) - Km * OpticalDepth(AverageMieScatterHeight, r, mu, d));
}

// Using "Filming Tonemapping"
float3 ToneMapping(float3 color)
{
    float A = 0.22;
    float B = 0.30;
    float C = 0.1;
    float D = 0.2;
    float E = 0.01;
    float F = 0.3;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}


float2 ClampUV(float2 uv)
{
    return max(0.0, min(1.0, uv));
}

#define NumSamples 5
#define SampleLength 0.9 // length of a sample in slope space

// Based on environment reflection code from LEADR paper
float3 MicrofacetReflection(float3 V, float3 N, float2 firstOrderMoments, float3 secondOrderMoments)
{
    float3 TSX = float3(1.0, 0.0, 0.0);
    float3 TSY = float3(0.0, 1.0, 0.0);
    float3 TSZ = float3(0.0, 0.0, 1.0);
    float sigma_x = sqrt(max(0.0, secondOrderMoments.x - firstOrderMoments.x * firstOrderMoments.x));
    float sigma_y = sqrt(max(0.0, secondOrderMoments.y - firstOrderMoments.y * firstOrderMoments.y));
    float r_xy = max(0.001, secondOrderMoments.z - firstOrderMoments.x * firstOrderMoments.y) / (sigma_x * sigma_y + 0.001);
    float r_xy_ = sqrt(max(0.001, 1.0 - r_xy * r_xy));
    float LODoffset = -1.0 + log2(SampleLength * (0.5 / 1.5) * max(sigma_x, sigma_y) * 2048.0);

    float S = 0.0;
    float3 I = 0.0;

    for (int i = 0; i < NumSamples; i++)
    {
        for (int j = 0; j < NumSamples; j++)
        {
            float pi = -1.8 + SampleLength * i;
            float pj = -1.8 + SampleLength * j;
            float wi = exp(-0.5 * pi * pi);
            float wj = exp(-0.5 * pj * pj);

			// microfacet slope
            float x = -firstOrderMoments.x + pi * sigma_x;
            float y = -firstOrderMoments.y + (pi * r_xy + pj * r_xy_) * sigma_y;

			// microfacet normal
            float3 NN = normalize(-x * TSX - y * TSY + TSZ);

			// microfacet projected area
            float VdotNN = max(0.001, dot(V, NN));
            float ZdotNN = 1; // dot(TSZ, NN);
            float S_ = VdotNN / ZdotNN;
            float3 R = reflect(-V, NN);
            float J = abs(4.0 * ZdotNN * ZdotNN * ZdotNN * VdotNN);
            float LOD = 0.72 * log(max(0.0001, J * (0.5 / 1.5))) + LODoffset;

			// LOD correction due to our UV distortion
            LOD += (4.0 / ((1.0 + R.z) * (1.0 + R.z)));

            float2 uv = R.xy / (1.0 + R.z);
            float3 I_ =g_textureDynamicSkyDome.SampleLevel(g_samplerBilinear, ClampUV(uv * (0.5 / 1.5) + 0.5), LOD).xyz;

            S += wi * wj * S_;
            I += wi * wj * S_ * I_;
        }
    }
    return I / S;
}

float GetFoam(float3 worldspacePosition, SURFACE_PARAMETERS surfParams)
{
	// Physically correct simulation of surface foam is barely possible in realtime,
	// so we use foam turbulent energy we get from simulation, surface folding, and a bunch of arbitrary numbers that give plausible look
	
	// Using 3 octaves of foam density texture and foam bubbles texture
    float foamDensityMapLowFrequency = g_textureFoam.Sample(g_samplerBilinear, worldspacePosition.xy * 0.04).x - 1.0;
    float foamDensityMapHighFrequency = g_textureFoam.Sample(g_samplerBilinear, worldspacePosition.xy * 0.15).x - 1.0;
    float foamDensityMapVeryHighFrequency = g_textureFoam.Sample(g_samplerBilinear, worldspacePosition.xy * 0.3).x;
    float foamBubbles = g_textureBubbles.Sample(g_samplerAnisotropic, worldspacePosition.xy * 0.25).r;

	// Combining 2 octaves of foam density texture
    float foamDensity;
    foamDensity = saturate(foamDensityMapHighFrequency + min(3.5, 1.0 * surfParams.foamEnergy - 0.2));
    foamDensity += (foamDensityMapLowFrequency + min(1.5, 1.0 * surfParams.foamEnergy));

	// Applying surface folding/stretching so foam is thicker in folded areas and thinner in stretched areas
    foamDensity -= 0.1 * saturate(-surfParams.foamSurfaceFolding);
    foamDensity = max(0, foamDensity);
    foamDensity *= 1.0 + 0.8 * saturate(surfParams.foamSurfaceFolding);

	// Adding foam on wave hats 
    foamDensity += max(0, foamDensityMapVeryHighFrequency * 2.0 * surfParams.foamWaveHats);
    foamDensity = pow(foamDensity, 0.7);

	// Modulating by foam bubbles to get very detailed look
    foamBubbles = saturate(5.0 * (foamBubbles - 0.8));
    foamDensity = saturate(foamDensity * foamBubbles);
    return foamDensity;
}

// ---------------------------------------------------------------------------
// Pixel shader
// ---------------------------------------------------------------------------
float4 main(DS_OUTPUT In) : SV_Target
{
    float3 g_WaterDeepColor = { 0.0, 0.2, 0.4 };
    float3 g_WaterScatterColor = { 0.0, 0.7, 0.6 };
    float4 g_WaterColorIntensity = { 0.02, 0.02, 0.01, 0.2 };
    float3 g_FoamColor = { 0.9, 0.9, 0.9 };
    float3 g_FoamUnderwaterColor = { 0.6, 0.6, 0.6 };

	// Calculating surface parameters and foam density
    SURFACE_PARAMETERS surfaceParameters = GFSDK_WaveWorks_GetSurfaceParameters(In);
    float foamIntensity = GetFoam(In.worldspacePositionUndisplaced, surfaceParameters);

    float fresnelFactor;
    float scatterFactor;

    float2 firstOrderMoments = surfaceParameters.firstOrderMoments;
    float3 secondOrderMoments = surfaceParameters.secondOrderMoments;
    float2 secondOrderMomentsLowestLOD = surfaceParameters.secondOrderMomentsLowestLOD;

    float3 eyeVec = g_eyePos - In.worldspacePositionDisplaced;
    float3 L = normalize(g_sunDirection);
    float3 V = normalize(eyeVec);
    float3 N = surfaceParameters.normal;
    float3 R = reflect(-V, N);

	// Bending backfacing normals if we encounter those
    float NdotV = dot(N, V);
    if (NdotV < 0.01)
    {
        N += V * (-NdotV + 0.01);
        N = -normalize(N);
    }

	// Adding roughness defined by application
    secondOrderMoments.xy += g_beckmannRoughness;

	// Applying overall roughness variation of surface roughness to fake wind gusts by adding two octaves of noise from texture 
    float windGusts =
		g_textureWindGusts.Sample(g_samplerBilinear, In.worldspacePositionDisplaced.xy * 0.0001).x *
		g_textureWindGusts.Sample(g_samplerBilinear, In.worldspacePositionDisplaced.xy * 0.00001).x;
    secondOrderMoments.xy *= 1.0 + 2.0 * windGusts; // TODO: fade down to 1.0 near camera

	// Adding roughness in areas covered by foam
    secondOrderMoments.xy += min(0.2, 2.5 * pow(foamIntensity, 4.0));

	// Calculating atmosphere and sun colors
    float3 airTransmittance = AnalyticTransmittance(PlanetRadius, V.z, length(eyeVec));

    float2 skyUV = (-V.xy / (1.0 - V.z)) * (0.5 / 1.5) + 0.5;
    skyUV = ClampUV(skyUV);
    float3 airColor = g_textureDynamicSkyDome.SampleLevel(g_samplerBilinear, skyUV, 0).xyz * g_sunIntensity; // need SampleLevel to avoid discontinuity on water edge where ddx/ddy break

    skyUV = (g_sunDirection.xy / (1.0 + g_sunDirection.z)) * (0.5 / 1.5) + 0.5;
    float3 sunColor =g_textureDynamicSkyDome.Sample(g_samplerBilinear, skyUV, 0).xyz; // need SampleLevel to avoid discontinuity on water edge where ddx/ddy break

	// Calculating environment reflection color
    float3 reflectionColor;
    if (g_useMicrofacetReflection > 0.0)
    {
        reflectionColor = MicrofacetReflection(V, N, firstOrderMoments, secondOrderMoments) * g_sunIntensity;
    }
    else
    {
        skyUV = (R.xy / (1.0 + R.z)) * (0.5 / 1.5) + 0.5; // Mapping sphere to quad 
        skyUV = ClampUV(skyUV);
        reflectionColor = g_textureDynamicSkyDome.SampleLevel(g_samplerBilinear, skyUV, 0).xyz * g_sunIntensity; // need SampleLevel to avoid discontinuity on water edge where ddx/ddy break
    }

	// Fresnel factor 
	// TODO: really need to use Lowest LOD?
    fresnelFactor = EffectiveFresnel(V, N, (1.0 + 2.0 * windGusts) * secondOrderMomentsLowestLOD + g_beckmannRoughness);

	// Simulating scattering/double refraction: light hits the side of wave, travels some distance in water, and leaves wave on the other side
	// it's difficult to do it physically correct without photon mapping/ray tracing, so using simple but plausible fake below

	// Only the crests of water waves generate double refracted light
    scatterFactor = 1.0 * max(0, (In.worldspacePositionDisplaced.z - In.worldspacePositionUndisplaced.z) * 0.001 + 0.3);

	// The waves that lie between camera and light projection on water plane generate maximal amount of double refracted light 
    scatterFactor *= pow(max(0.0, dot(L, -V)), 4.0);

	// The slopes of waves that are oriented back to light generate maximal amount of double refracted light 
    scatterFactor *= g_WaterColorIntensity.w * pow(max(0.0, 0.5 - 0.5 * dot(L, N)), 3.0);

	// The scattered light is best seen if observing direction is normal to slope surface
    scatterFactor += g_WaterColorIntensity.z * max(0, pow(dot(V, N), 2.0));

	// Applying Smith masking to it
    scatterFactor /= (1.0 + SmithMasking(V, N, secondOrderMoments));

	// Calculating refraction color
    float3 refractionColor = g_WaterScatterColor * scatterFactor * sunColor * g_sunIntensity;

	// Adding shallow scattering as if the water surface reflects as a diffuse surface and emits some light
    refractionColor += (g_WaterColorIntensity.x + g_WaterColorIntensity.y * max(0, dot(L, N))) * g_WaterDeepColor * sunColor * g_sunIntensity;

	// Adding color that provide foam bubbles spread in water 
    refractionColor += g_FoamUnderwaterColor * saturate(surfaceParameters.foamEnergy * 0.05) * sunColor * g_sunIntensity;

	// Applying Fresnel law
    float3 waterColor = lerp(refractionColor, reflectionColor, fresnelFactor);

	// Calculating and applying specular reflection
    float specularLight;
    if (g_useMicrofacetSpecular > 0)
    {
        specularLight = MicrofacetSpecular(L, V, N, firstOrderMoments, secondOrderMoments);
    }
    else
    {
        float3 H = normalize(L + V);
        float HdotN = dot(H, N);
        specularLight = 100.0 * pow(max(0.0, HdotN), 1000.0) * EffectiveFresnel(V, H, secondOrderMoments.xy);
    }
    waterColor += specularLight * g_sunIntensity * sunColor;

	// Calculating and applying foam diffuse color
    float3 foamDiffuseColor = g_FoamColor * max(0, 0.3 + max(0, 0.7 * dot(L, N)));
    waterColor = lerp(waterColor, foamDiffuseColor * sunColor * g_sunIntensity, foamIntensity);

	// Applying atmosphere color
    waterColor = lerp(airColor, waterColor, airTransmittance);

	// Applying tonemapping
    float ExposureBias = 1.0;
    float3 curr =2.0f //ToneMapping(ExposureBias * waterColor);

    float3 whiteScale = 1.0 / ToneMapping(3.0);
    waterColor = curr * whiteScale;
    waterColor = pow(waterColor, 1.0 / 2.2);

	// Showing cascade edges if needed
    if (g_showCascades > 0)
    {
        if (frac(In.UVForCascade01.x) < 0.05)
            waterColor.r += 0.25;
        if (frac(In.UVForCascade01.y) < 0.05)
            waterColor.r += 0.25;

        if (frac(In.UVForCascade01.z) < 0.05)
            waterColor.g += 0.25;
        if (frac(In.UVForCascade01.w) < 0.05)
            waterColor.g += 0.25;

        if (frac(In.UVForCascade23.x) < 0.05)
            waterColor.b += 0.5;
        if (frac(In.UVForCascade23.y) < 0.05)
            waterColor.b += 0.5;

        if (frac(In.UVForCascade23.z) < 0.05)
            waterColor.rgb += 0.25;
        if (frac(In.UVForCascade23.w) < 0.05)
            waterColor.rgb += 0.25;
    }

    return float4(waterColor, 1.0);
}



#endif