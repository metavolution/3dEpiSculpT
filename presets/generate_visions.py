import os
import json

def generate_preset(name, inner, color, reduction, seed, time=0.0, latentW=0.0):
    return {
        "version": 1,
        "preset": name,
        "time": time,
        "morph": 0.0,
        "cameraAngle": 0.0,
        "inner": inner,
        "innerB": "Identity",
        "reduction": reduction,
        "seedType": seed,
        "seedTypeB": "Border",
        "colorEffect": color,
        "feedbackIterations": 1,
        "enable4D": 1,
        "outputMode": 0,
        "outputModeB": 0,
        "stageMask": 3,
        "instanceMeshType": 0,
        "attractorMode": 0,
        "patternSpeed": 1.0,
        "latentW": latentW,
        "rotorAlpha": 0.5,
        "rotorBeta": 0.5,
        "rotorTheta4D": 0.0,
        "seedWidth": 0.1,
        "seedAlpha": 0.8,
        "seedBaseline": 0.0,
        "seedPulseFrequency": 2.0,
        "seedPulseAmplitude": 0.5,
        "hueRotation": 0.0,
        "hueSpeed": 0.1,
        "worldScale": 1.0,
        "radiusScale": 1.0,
        "chromaBreath": 0.5,
        "lumaDrift": 0.2,
        "paletteBlend": 0.5,
        "volumetricDensityScale": 1.0,
        "fogAnisotropy": 0.5,
        "advectionSpeed": 0.1,
        "curlStrength": 0.5,
        "trailDecay": 0.95,
        "kineticViscosity": 0.1,
        "attractorStrength": 0.5,
        "filmThickness": 0.5,
        "filmIOR": 1.33,
        "temporalFeedbackBlend": 0.9,
        "temporalFade": 0.1,
        "temporalFixedDelta": 0.016,
        "topologyBlend": 0.5
    }

presets = [
    ("cosmological/09_quantum_interference.epimode", "PoincareHyperbolic", "QuantumInterference", "Torus", "Attractor", 10.0, 0.5),
    ("cosmological/10_calabi_yau_vaporwave.epimode", "CalabiYauQuintic", "Vaporwave", "Torus", "HyperSphere4D", 100.0, 0.8),
    ("biological/15_gyroid_bioluminescence.epimode", "Gyroid", "Bioluminescence", "Torus", "WaveRing", 0.0, 0.2),
    ("biological/16_henon_glitch.epimode", "Henon", "Glitch", "Grid", "Cross", 0.0, 0.0),
    ("dimensional/06_mobius_thermal.epimode", "Mobius", "ThermalHologram", "Clamp", "Dipole", 5.0, 1.0),
    ("dimensional/07_kaleidoscope_iridescence.epimode", "Kaleidoscope", "ThinFilmIridescence", "Torus", "Lissajous", 20.0, -0.5),
    ("kinetic/05_lissajous_chladni.epimode", "Lissajous", "ChladniAcoustic", "Grid", "ChladniCymatics", 15.0, 0.0)
]

for path, inner, color, reduction, seed, time, latent in presets:
    full_path = os.path.join("3dEpiSculpTMorph/presets", path)
    os.makedirs(os.path.dirname(full_path), exist_ok=True)
    with open(full_path, "w") as f:
        json.dump(generate_preset(os.path.basename(path).split('.')[0], inner, color, reduction, seed, time, latent), f, indent=2)

print("Generated visionary presets!")
