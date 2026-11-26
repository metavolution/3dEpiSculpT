// OpenCL domain producer kernels for the Recursive Counter-Winding Hopf Torus.
//
// Evaluates the stereographic Clifford torus P(u,v) = (cos u, sin u, cos v)/(sqrt(2) - sin v),
// analytic Villarceau circle descriptors for the (1,1) and (-1,1) families,
// the 2m^2 crossing lattice, and the 2m^2 half-step shifted interstitial child
// similarity maps T_i.

#include "RecursiveHopfTorusAbi.clh"

#define EPI_HOPF_SQRT2 1.4142135623730950488f
#define EPI_HOPF_INV_SQRT2 0.7071067811865475244f
#define EPI_HOPF_PI 3.14159265358979323846f
#define EPI_HOPF_TAU 6.28318530717958647692f
#define EPI_HOPF_TORUS_BOUND (EPI_HOPF_SQRT2 + 1.0f)

inline float epi_hopf_wrap_angle(float a, float period) {
    return a - period * rint(a / period);
}

inline float epi_hopf_phase_at(float initialRad, float rateRadPerSec, float domainTime) {
    const float twoTau = 2.0f * EPI_HOPF_TAU;
    const float a0 = epi_hopf_wrap_angle(initialRad, twoTau);
    const float da = epi_hopf_wrap_angle(rateRadPerSec * domainTime, twoTau);
    return epi_hopf_wrap_angle(a0 + da, twoTau);
}

inline float3 epi_hopf_torus_point(float u, float v) {
    const float uw = epi_hopf_wrap_angle(u, EPI_HOPF_TAU);
    const float vw = epi_hopf_wrap_angle(v, EPI_HOPF_TAU);
    const float denom = EPI_HOPF_SQRT2 - sin(vw);
    return (float3)(cos(uw) / denom, sin(uw) / denom, cos(vw) / denom);
}

inline void epi_hopf_torus_frame(float u, float v, float3* outX, float3* outY, float3* outZ) {
    const float uw = epi_hopf_wrap_angle(u, EPI_HOPF_TAU);
    const float vw = epi_hopf_wrap_angle(v, EPI_HOPF_TAU);
    const float cu = cos(uw);
    const float su = sin(uw);
    const float cv = cos(vw);
    const float sv = sin(vw);
    const float denom = EPI_HOPF_SQRT2 - sv;

    const float3 eU = (float3)(-su, cu, 0.0f);
    const float3 eV = (float3)(cu * cv / denom, su * cv / denom, (1.0f - EPI_HOPF_SQRT2 * sv) / denom);
    *outX = eU;
    *outY = -eV;
    *outZ = -cross(eU, eV);
}

inline float3 epi_hopf_basis_apply(float3 bx, float3 by, float3 bz, float3 p) {
    return bx * p.x + by * p.y + bz * p.z;
}

inline float epi_hopf_point_circle_distance(float3 p, float phaseRad, float centerSignY) {
    const float phW = epi_hopf_wrap_angle(phaseRad, EPI_HOPF_TAU);
    const float c = cos(phW);
    const float s = sin(phW);
    const float3 rx = (float3)(c, s, 0.0f);
    const float3 ry = (float3)(-s, c, 0.0f);
    const float3 rz = (float3)(0.0f, 0.0f, 1.0f);
    const float3 planeZ = (float3)(-EPI_HOPF_INV_SQRT2, 0.0f, EPI_HOPF_INV_SQRT2);

    const float3 center = epi_hopf_basis_apply(rx, ry, rz, (float3)(0.0f, centerSignY, 0.0f));
    const float3 normal = epi_hopf_basis_apply(rx, ry, rz, planeZ);
    const float3 diff = p - center;
    const float dPerp = dot(diff, normal);
    const float3 inPlane = diff - normal * dPerp;
    const float rho = length(inPlane);
    const float dRad = rho - EPI_HOPF_SQRT2;
    return hypot(dPerp, dRad);
}

__kernel void evaluate_recursive_hopf_grammar(
    __constant const RecursiveHopfRecipeGpu* recipe,
    float domainTime,
    uint generation,
    __global RecursiveHopfGrammarGpu* outGrammar)
{
    const uint gid = get_global_id(0);
    if (!epi_recursive_hopf_recipe_valid(recipe)) return;

    const uint m = recipe->fibersPerFamily;
    const uint rootCircleCount = 2U * m;
    const uint branching = 2U * m * m;
    const uint childrenCount = (recipe->logicalDepth == 0U) ? 0U : branching;

    const float plusPhase = epi_hopf_phase_at(
        recipe->plusInitialRadians, recipe->plusRadiansPerSecond, domainTime);
    const float minusPhase = epi_hopf_phase_at(
        recipe->minusInitialRadians, recipe->minusRadiansPerSecond, domainTime);

    // Work-item 0 writes the header, recipe mirror, and 2m root circles
    if (gid == 0U) {
        ulong level = 1UL;
        ulong totalNodes = 1UL;
        for (uint d = 0U; d < recipe->logicalDepth; ++d) {
            level *= (ulong)branching;
            totalNodes += level;
        }
        const ulong totalFibers = totalNodes * (ulong)rootCircleCount;

        const float own = EPI_HOPF_TORUS_BOUND + recipe->fiberRadius + recipe->weaveAmplitude;
        const float centers = EPI_HOPF_TORUS_BOUND + fabs(recipe->childNormalOffset);
        const float maxConformal = (recipe->conformalScaleExponent > 0.0f)
            ? pow(EPI_HOPF_SQRT2 / (EPI_HOPF_SQRT2 - 1.0f), recipe->conformalScaleExponent)
            : 1.0f;
        const float effScale = fmin(0.95f, recipe->childScale * maxConformal);
        float bound = own;
        for (uint d = 0U; d < recipe->logicalDepth; ++d) {
            bound = fmax(own, centers + effScale * bound);
        }
        bound *= (1.0f + 1.0e-5f * (float)(recipe->logicalDepth + 1U));

        outGrammar->header.abiVersion = EPI_RECURSIVE_HOPF_ABI_VERSION;
        outGrammar->header.byteSize = (uint)sizeof(RecursiveHopfGrammarGpu);
        outGrammar->header.generation = generation;
        outGrammar->header.rootCircleCount = rootCircleCount;
        outGrammar->header.childrenPerNode = childrenCount;
        outGrammar->header.logicalDepth = recipe->logicalDepth;
        outGrammar->header.flags = recipe->flags;
        outGrammar->header.reserved0 = 0U;
        outGrammar->header.domainTime = domainTime;
        outGrammar->header.plusPhase = plusPhase;
        outGrammar->header.minusPhase = minusPhase;
        outGrammar->header.subtreeRadius = bound;
        outGrammar->header.logicalNodes = totalNodes;
        outGrammar->header.logicalFibers = totalFibers;

        outGrammar->recipe = *recipe;

        const float3 planeX = (float3)(EPI_HOPF_INV_SQRT2, 0.0f, EPI_HOPF_INV_SQRT2);
        const float3 planeY = (float3)(0.0f, 1.0f, 0.0f);
        const float3 planeZ = (float3)(-EPI_HOPF_INV_SQRT2, 0.0f, EPI_HOPF_INV_SQRT2);

        for (uint f = 0U; f < m; ++f) {
            // Plus family (1, 1)
            {
                const float ph = plusPhase + EPI_HOPF_TAU * (float)f / (float)m;
                const float phW = epi_hopf_wrap_angle(ph, EPI_HOPF_TAU);
                const float c = cos(phW);
                const float s = sin(phW);
                const float3 rx = (float3)(c, s, 0.0f);
                const float3 ry = (float3)(-s, c, 0.0f);
                const float3 rz = (float3)(0.0f, 0.0f, 1.0f);

                const float3 center = epi_hopf_basis_apply(rx, ry, rz, (float3)(0.0f, 1.0f, 0.0f));
                const float3 ax = epi_hopf_basis_apply(rx, ry, rz, planeX);
                const float3 ay = epi_hopf_basis_apply(rx, ry, rz, planeY);
                const float3 az = epi_hopf_basis_apply(rx, ry, rz, planeZ);

                outGrammar->rootCircles[f].centerRadius = (float4)(center, EPI_HOPF_SQRT2);
                outGrammar->rootCircles[f].axisX = (float4)(ax, 1.0f);
                outGrammar->rootCircles[f].axisY = (float4)(ay, (float)f);
                outGrammar->rootCircles[f].axisZ = (float4)(az, ph);
            }
            // Minus family (-1, 1)
            {
                const float ph = minusPhase + EPI_HOPF_TAU * (float)f / (float)m;
                const float phW = epi_hopf_wrap_angle(ph, EPI_HOPF_TAU);
                const float c = cos(phW);
                const float s = sin(phW);
                const float3 rx = (float3)(c, s, 0.0f);
                const float3 ry = (float3)(-s, c, 0.0f);
                const float3 rz = (float3)(0.0f, 0.0f, 1.0f);

                const float3 center = epi_hopf_basis_apply(rx, ry, rz, (float3)(0.0f, -1.0f, 0.0f));
                const float3 ax = epi_hopf_basis_apply(rx, ry, rz, planeX);
                const float3 ay = epi_hopf_basis_apply(rx, ry, rz, planeY);
                const float3 az = epi_hopf_basis_apply(rx, ry, rz, planeZ);

                outGrammar->rootCircles[m + f].centerRadius = (float4)(center, EPI_HOPF_SQRT2);
                outGrammar->rootCircles[m + f].axisX = (float4)(ax, -1.0f);
                outGrammar->rootCircles[m + f].axisY = (float4)(ay, (float)f);
                outGrammar->rootCircles[m + f].axisZ = (float4)(az, ph);
            }
        }
    }

    // Parallel evaluation of the 2m^2 crossing & interstitial child maps
    if (gid < childrenCount) {
        const uint pair = gid / 2U;
        const uint k = pair / m;
        const uint l = pair % m;
        const uint branch = gid % 2U;

        const float aCross = plusPhase + EPI_HOPF_TAU * (float)k / (float)m;
        const float bCross = minusPhase + EPI_HOPF_TAU * (float)l / (float)m;
        const float uCross = 0.5f * (aCross + bCross) + (float)branch * EPI_HOPF_PI;
        const float vCross = 0.5f * (bCross - aCross) + (float)branch * EPI_HOPF_PI;
        const float3 pCross = epi_hopf_torus_point(uCross, vCross);

        const float shift = EPI_HOPF_PI / (float)m;
        const float aInter = aCross + shift;
        const float bInter = bCross + shift;
        const float uInter = 0.5f * (aInter + bInter) + (float)branch * EPI_HOPF_PI;
        const float vInter = 0.5f * (bInter - aInter) + (float)branch * EPI_HOPF_PI;

        const float3 pInter = epi_hopf_torus_point(uInter, vInter);
        float3 fx, fy, fz;
        epi_hopf_torus_frame(uInter, vInter, &fx, &fy, &fz);

        const float twistW = epi_hopf_wrap_angle(recipe->childTwistRadians, EPI_HOPF_TAU);
        const float ct = cos(twistW);
        const float st = sin(twistW);
        const float3 tx = epi_hopf_basis_apply(fx, fy, fz, (float3)(ct, st, 0.0f));
        const float3 ty = epi_hopf_basis_apply(fx, fy, fz, (float3)(-st, ct, 0.0f));
        const float3 tz = fz;

        const float3 translation = pInter + fz * recipe->childNormalOffset;

        float siteScale = recipe->childScale;
        if (recipe->placementMode == EPI_RECURSIVE_HOPF_PLACEMENT_CLEARANCE) {
            const uint kNext = (k + 1U) % m;
            const uint lNext = (l + 1U) % m;
            const float aNext = plusPhase + EPI_HOPF_TAU * (float)kNext / (float)m;
            const float bNext = minusPhase + EPI_HOPF_TAU * (float)lNext / (float)m;
            const float d0 = epi_hopf_point_circle_distance(translation, aCross, 1.0f);
            const float d1 = epi_hopf_point_circle_distance(translation, aNext, 1.0f);
            const float d2 = epi_hopf_point_circle_distance(translation, bCross, -1.0f);
            const float d3 = epi_hopf_point_circle_distance(translation, bNext, -1.0f);
            const float minDist = fmin(fmin(d0, d1), fmin(d2, d3));
            const float clearRadius = fmax(0.0f, minDist - (recipe->fiberRadius + recipe->weaveAmplitude));
            const float outerExtent = EPI_HOPF_TORUS_BOUND + recipe->fiberRadius + recipe->weaveAmplitude;
            const float clearScale = clamp(clearRadius / outerExtent, 1.0e-5f, 0.95f);
            siteScale = clamp(recipe->childScale * clearScale, 1.0e-5f, 0.95f);
        } else if (recipe->placementMode == EPI_RECURSIVE_HOPF_PLACEMENT_CONFORMAL ||
                   recipe->conformalScaleExponent > 0.0f) {
            const float expVal = (recipe->placementMode == EPI_RECURSIVE_HOPF_PLACEMENT_CONFORMAL &&
                                  recipe->conformalScaleExponent == 0.0f)
                ? 1.0f
                : recipe->conformalScaleExponent;
            const float vw = epi_hopf_wrap_angle(vInter, EPI_HOPF_TAU);
            const float conf = EPI_HOPF_SQRT2 / (EPI_HOPF_SQRT2 - sin(vw));
            siteScale = fmin(0.95f, recipe->childScale * pow(conf, expVal));
        }

        outGrammar->children[gid].translationScale = (float4)(translation, siteScale);
        outGrammar->children[gid].axisX = (float4)(tx, uInter);
        outGrammar->children[gid].axisY = (float4)(ty, vInter);
        outGrammar->children[gid].axisZ = (float4)(tz, uCross);
        outGrammar->children[gid].crossingPosition = (float4)(pCross, vCross);
        outGrammar->children[gid].plusFiber = k;
        outGrammar->children[gid].minusFiber = l;
        outGrammar->children[gid].branch = branch;
        outGrammar->children[gid].childIndex = gid;
    }
}

// Parallel B-ary hierarchy expansion kernel.
// Each work-item gid corresponds to breadth-first nodeId = gid in the regular
// B-ary tree (B = grammar->header.childrenPerNode), decodes its root-to-node
// path in O(depth) steps, composes the similarity chain in registers, and
// emits all 2m world Villarceau circle instances for that node.
__kernel void expand_recursive_hopf_instances(
    __global const RecursiveHopfGrammarGpu* grammar,
    uint effectiveMaxDepth,
    float minNodeScale,
    uint maxInstances,
    __global RecursiveHopfInstanceGpu* outInstances,
    __global volatile uint* outInstanceCount)
{
    if (grammar == 0 || outInstances == 0 || outInstanceCount == 0) return;
    if (grammar->header.abiVersion != EPI_RECURSIVE_HOPF_ABI_VERSION) return;

    const uint gid = get_global_id(0);
    const uint m = grammar->recipe.fibersPerFamily;
    const uint rootCircleCount = grammar->header.rootCircleCount;
    const uint branching = grammar->header.childrenPerNode;
    const uint maxDepth = min(effectiveMaxDepth, grammar->header.logicalDepth);

    if (rootCircleCount == 0U || rootCircleCount > EPI_RECURSIVE_HOPF_MAX_ROOT_CIRCLES) return;

    // Locate depth d and within-level offset rem for breadth-first nodeId = gid.
    ulong levelStart = 0UL;
    ulong levelCount = 1UL;
    uint depth = 0U;
    bool validNode = false;
    for (uint d = 0U; d <= maxDepth; ++d) {
        if ((ulong)gid >= levelStart && (ulong)gid < levelStart + levelCount) {
            depth = d;
            validNode = true;
            break;
        }
        if (branching == 0U) break;
        levelStart += levelCount;
        levelCount *= (ulong)branching;
    }
    if (!validNode) return;

    uint path[EPI_RECURSIVE_HOPF_MAX_LOGICAL_DEPTH];
    ulong rem = (ulong)gid - levelStart;
    for (uint step = 0U; step < depth; ++step) {
        path[depth - 1U - step] = (uint)(rem % (ulong)branching);
        rem /= (ulong)branching;
    }

    float3 nodePos = (float3)(0.0f, 0.0f, 0.0f);
    float nodeScale = 1.0f;
    float3 nodeAx = (float3)(1.0f, 0.0f, 0.0f);
    float3 nodeAy = (float3)(0.0f, 1.0f, 0.0f);
    float3 nodeAz = (float3)(0.0f, 0.0f, 1.0f);

    for (uint step = 0U; step < depth; ++step) {
        const uint childIdx = path[step];
        const RecursiveHopfChildMapGpu child = grammar->children[childIdx];
        const float3 cTrans = child.translationScale.xyz;
        const float cScale = child.translationScale.w;
        const float3 cAx = child.axisX.xyz;
        const float3 cAy = child.axisY.xyz;
        const float3 cAz = child.axisZ.xyz;

        nodePos = nodePos + nodeScale * epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, cTrans);
        nodeScale = nodeScale * cScale;

        const float3 nextAx = epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, cAx);
        const float3 nextAy = epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, cAy);
        const float3 nextAz = epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, cAz);
        nodeAx = nextAx;
        nodeAy = nextAy;
        nodeAz = nextAz;
    }

    if (nodeScale < minNodeScale) return;

    const uint baseSlot = atomic_add(outInstanceCount, rootCircleCount);
    if (baseSlot >= maxInstances) return;

    const uint emitCount = min(rootCircleCount, maxInstances - baseSlot);
    const float baseTubeRadius = grammar->recipe.fiberRadius;

    for (uint c = 0U; c < emitCount; ++c) {
        const RecursiveHopfCircleGpu rc = grammar->rootCircles[c];
        const float3 localCenter = rc.centerRadius.xyz;
        const float localRadius = rc.centerRadius.w;
        const float3 worldCenter = nodePos + nodeScale * epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, localCenter);
        const float worldRadius = nodeScale * localRadius;
        const float3 worldAx = epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, rc.axisX.xyz);
        const float3 worldAy = epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, rc.axisY.xyz);
        const float3 worldAz = epi_hopf_basis_apply(nodeAx, nodeAy, nodeAz, rc.axisZ.xyz);
        const uint chirality = (c < m) ? 0U : 1U;
        const uint fiberIdx = (c < m) ? c : (c - m);
        const float chiralitySign = (c < m) ? 1.0f : -1.0f;

        RecursiveHopfInstanceGpu inst;
        inst.centerRadius = (float4)(worldCenter, worldRadius);
        inst.axisX = (float4)(worldAx, nodeScale);
        inst.axisY = (float4)(worldAy, chiralitySign);
        inst.axisZ = (float4)(worldAz, baseTubeRadius * nodeScale);
        inst.nodeIdLow = gid;
        inst.depth = depth;
        inst.chirality = chirality;
        inst.fiberIndex = fiberIdx;

        outInstances[baseSlot + c] = inst;
    }
}

