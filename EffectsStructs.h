#ifndef EFFECTSSTRUCTS_H
#define EFFECTSSTRUCTS_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QColor>
#include <cmath>

namespace Effects {

// ============================================================================
// GRADIENT POINT - used by emitters for color/alpha over lifetime
// ============================================================================
struct GradientPoint {
    int position;       // 0-1000 (timeline in particle lifetime)
    float r, g, b;      // 0.0-1.0+ (can exceed 1.0 for HDR/bloom)
    float alpha;        // 0.0-1.0

    GradientPoint() : position(0), r(1.0f), g(1.0f), b(1.0f), alpha(1.0f) {}
    GradientPoint(int pos, float _r, float _g, float _b, float _a)
        : position(pos), r(_r), g(_g), b(_b), alpha(_a) {}

    QColor toQColor() const {
        // Clamp to 0-255 for display (HDR values will be clamped)
        return QColor(
            qBound(0, static_cast<int>(r * 255), 255),
            qBound(0, static_cast<int>(g * 255), 255),
            qBound(0, static_cast<int>(b * 255), 255),
            qBound(0, static_cast<int>(alpha * 255), 255)
            );
    }
};

// ============================================================================
// EMITTER - particle emitter definition
// Field-to-argument order ground-truthed 2026-08-20 directly against the
// real engine's own registered Emitter_Create usage/signature string
// (Ghidra, Outforce_w2k.exe address 005596cc - see
// EffectsParser::parseEmitterCreate() for the exact quoted signature and
// EffectsWriter::formatEmitterCreate() for the matching write-back order).
// A previous version of the parser/writer had 8 of these 22 fields reading
// from/writing to the wrong Emitter_Create() argument position (DeathWish/
// ReuseParticles swapped, plus a 6-way cyclic shift across
// SpreadCoofficient/SpeedModifier/Acceleration/Shrink/SpeedVariation/
// SpeedVariationCoefficient) - fixed in both files; this struct's field
// SET and names were already correct and complete, only the index mapping
// was wrong, so no field was added/removed/renamed here.
// The "+NN" byte-offset numbers below describe the real engine's in-memory
// Emitter struct layout (separate from, and not necessarily in the same
// order as, the text argument list above) - NOT independently
// re-verified as part of this fix, carried over from the prior version of
// this file as-is.
// ============================================================================
struct Emitter {
    int id;                         // +32: Unique identifier
    QString name;                   // From Emitter_SetName
    QString material;               // From Emitter_SetMaterial

    // 22 Create parameters, in real Emitter_Create() argument order - see
    // the ground-truth signature quoted above this struct.
    float birthRate;                // BirthRate: particles per second
    int deathWish;                  // DeathWish (bool) - unverified runtime meaning, carried over as "destroy when done" per the existing UI label
    int reuseParticles;             // ReuseParticles (bool) - unverified runtime meaning, carried over as "0=burst, 1=continuous" per prior comment
    float speed;                    // Speed
    float lifeTime;                 // LifeTime: particle lifetime in seconds
    float variableLifeTime;         // VariableLifeTime: pre-warm / growth rate
    int numParticles;               // NumParticles: maximum particle count
    float size;                     // Size
    float spread;                   // Spread: cone angle (radians)
    float spreadCoefficient;        // SpreadCoofficient (engine's own spelling)
    float spreadAround;             // SpreadAround: secondary spread angle
    float spreadAroundStart;        // SpreadAroundStart
    float speedModifier;            // SpeedModifier
    float posVariationX;            // PosVariationX
    float posVariationY;            // PosVariationY
    float posVariationZ;            // PosVariationZ
    float acceleration;             // Acceleration
    float sizeVariation;            // SizeVariation: size growth over lifetime
    float shrink;                   // Shrink: exponential size decay/growth rate over elapsed seconds (size *= exp(shrink*t) - ground-truthed independently in the OutforceMapEditor sibling project's particle renderer, see its FUN_004e0be0 decompile)
    float speedVariation;           // SpeedVariation
    float speedVariationCoeff;      // SpeedVariationCoefficient

    // Gradient points (+196)
    QVector<GradientPoint> gradientPoints;

    Emitter() : id(0), birthRate(10.0f), deathWish(1), reuseParticles(1),
        speed(1.0f), lifeTime(1.0f), variableLifeTime(0.0f), numParticles(50),
        size(0.1f), spread(6.283185f), spreadCoefficient(1.0f), spreadAround(6.283185f),
        spreadAroundStart(0.0f), speedModifier(1.0f), posVariationX(0.05f),
        posVariationY(0.05f), posVariationZ(0.05f), acceleration(0.0f),
        sizeVariation(0.0f), shrink(0.0f), speedVariation(0.0f),
        speedVariationCoeff(1.0f) {}

    // Helper to convert radians to degrees for display
    static float radiansToDegrees(float rad) { return rad * 180.0f / M_PI; }
    static float degreesToRadians(float deg) { return deg * M_PI / 180.0f; }
};

// ============================================================================
// EXPLOSION OBJECT - visual element in explosion
// From IDA: sub_4C85D0 - Effect_Explo_AddObject parser
// Struct offsets: +12(material), +36(scale), +40(delay), +44(duration),
//                 +48(variation), +52(additive), +53(loop), +25(billboard)
// ============================================================================
struct ExplosionObject {
    QString materialName;           // +12: Visual effect material
    float scale;                    // +36: Size/scale of the effect
    float delay;                    // +40: Delay before showing (seconds)
    float duration;                 // +44: How long to show (seconds)
    float variation;                // +48: Random variation
    int additive;                   // +52: Additive blending (0 or 1)
    int loop;                       // +53: Loop animation (0 or 1)
    int billboard;                  // +25: Billboard sprite mode (0 or 1)

    ExplosionObject() : scale(1.0f), delay(0.0f), duration(1.0f), variation(0.5f),
        additive(0), loop(0), billboard(0) {}
};

// ============================================================================
// THROW OBJECT - debris/particles thrown during explosion
// From IDA: sub_4C8860 - Effect_Explo_AddThrowObject parser
// Struct offsets: +24=1(isThrow), +12(material), +36(scale), +40(delay),
//                 +44(duration), +48(variation), +28(speed), +32(count)
// ============================================================================
struct ThrowObject {
    QString materialName;           // +12: Debris material name
    float scale;                    // +36: Size/scale of debris
    float delay;                    // +40: Delay before throwing
    float duration;                 // +44: How long debris exists
    float variation;                // +48: Random variation
    float speed;                    // +28: Throw velocity
    float count;                    // +32: Number of debris pieces

    ThrowObject() : scale(1.0f), delay(0.0f), duration(1.0f), variation(0.5f),
        speed(10.0f), count(2.0f) {}
};

// ============================================================================
// EXPLOSION - complete explosion definition
// ============================================================================
struct Explosion {
    QString name;                   // Explosion name (e.g., "UCMExplosion")

    QVector<ExplosionObject> objects;       // Visual elements
    QVector<ThrowObject> throwObjects;      // Debris

    // Settings
    float scaleX, scaleY, scaleZ;           // Scale factors
    float shake;                            // Camera shake intensity
    float polyBlowSpeed;                    // Polygon destruction speed
    QString debrisExplosion;                // Debris explosion type
    QString soundFile;                      // Sound effect filename
    bool isLocked;                          // Locked flag

    // Pressure Wave - Effect_Explo_SetPressureWave (name, force, innerRadius, outerRadius)
    bool hasPressureWave;                   // True if pressure wave is set
    float pressureWaveForce;                // Pressure wave force/strength
    float pressureWaveInnerRadius;          // Inner radius
    float pressureWaveOuterRadius;          // Outer radius

    Explosion() : scaleX(1.0f), scaleY(1.0f), scaleZ(1.0f), shake(0.0f),
        polyBlowSpeed(0.0f), isLocked(false),
        hasPressureWave(false), pressureWaveForce(0.0f),
        pressureWaveInnerRadius(0.0f), pressureWaveOuterRadius(0.0f) {}
};

// ============================================================================
// SCENE COLORS - global scene color settings
// ============================================================================
struct SceneColors {
    // Ambient
    int ambientR, ambientG, ambientB;

    // Background
    int bgColorR, bgColorG, bgColorB;

    // Sun
    int sunR, sunG, sunB;
    float sunPitch, sunHeading;

    // Fog
    int fogR, fogG, fogB;
    int fogHalfR, fogHalfG, fogHalfB;

    // Light of God (volumetric light)
    float logHeight, logRange, logStrength;
    int logVariation;
    float logColorR, logColorG, logColorB;

    SceneColors() : ambientR(50), ambientG(50), ambientB(50),
        bgColorR(10), bgColorG(0), bgColorB(55),
        sunR(300), sunG(300), sunB(300), sunPitch(0.4f), sunHeading(2.5f),
        fogR(5), fogG(0), fogB(0), fogHalfR(5), fogHalfG(0), fogHalfB(0),
        logHeight(2.0f), logRange(6.5f), logStrength(0.4f), logVariation(3),
        logColorR(-0.1f), logColorG(-0.1f), logColorB(1.0f) {}
};

// ============================================================================
// EFFECTS PROJECT - contains all loaded effects data
// ============================================================================
struct EffectsProject {
    QMap<int, Emitter> emitters;                    // ID -> Emitter
    QMap<int, QString> emitterMaterials;            // ID -> Material name
    QMap<QString, Explosion> explosions;            // Name -> Explosion
    SceneColors sceneColors;

    // File paths
    QString emittersCfgPath;
    QString emitterMaterialsCfgPath;
    QString explosionsCfgPath;
    QString explosionsSettingsCfgPath;
    QString colorsCfgPath;

    void clear() {
        emitters.clear();
        emitterMaterials.clear();
        explosions.clear();
        sceneColors = SceneColors();
        emittersCfgPath.clear();
        emitterMaterialsCfgPath.clear();
        explosionsCfgPath.clear();
        explosionsSettingsCfgPath.clear();
        colorsCfgPath.clear();
    }

    // Apply materials to emitters
    void applyMaterials() {
        for (auto it = emitterMaterials.begin(); it != emitterMaterials.end(); ++it) {
            if (emitters.contains(it.key())) {
                emitters[it.key()].material = it.value();
            }
        }
    }
};

} // namespace Effects

#endif // EFFECTSSTRUCTS_H
