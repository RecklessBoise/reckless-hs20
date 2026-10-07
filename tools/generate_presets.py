#!/usr/bin/env python3
"""Generates resources/factory_presets.json for the Reckless HS20.

Every preset is written as a compact set of overrides on top of the init
patch. Knob values use the 0..10 panel scale. Quick reference for the DSP
calibration (see Source/dsp/SynthEngine.cpp):

  cutoff  knob = octaves above 20 Hz   (5 = 640 Hz, 7 = 2.5 kHz, 9 = 10 kHz)
  peak    9.3+ self-oscillates on the Rev 1 filter
  EG2 mod knob x 0.8 = octaves of sweep
  MG freq 4 = 0.8 Hz, 6 = 3.2 Hz, 8 = 12.6 Hz
  FM MG   (k/10)^2 x 24 semitones, FM EG1 (k/10)^2 x 48 semitones
  times   attack: 4 = 26 ms, 6 = 190 ms, 8 = 1.4 s; decay/release: 4 = 60 ms, 6 = 330 ms, 8 = 1.8 s

Usage:  python3 tools/generate_presets.py [--levels levels.json]
The optional levels file (written by `HS20Tests --levels`) normalises every
preset's volume so the whole library plays at a consistent loudness.
"""
import argparse
import json
import math
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "resources" / "factory_presets.json"

# Waveforms / scales
TRI, SAW, PULSE, NOISE = 0, 1, 2, 3            # VCO1
SAW2, SQR2, NARROW2, RING2 = 0, 1, 2, 3        # VCO2
F32, F16, F8, F4 = 0, 1, 2, 3

SRC = dict(off=0, mgtri=1, mgsq=2, eg1=3, eg1rev=4, eg2=5, eg2rev=6, sh=7, white=8,
           pink=9, kbd=10, vel=11, wheel=12, espenv=13, espfv=14)
DST = dict(pitch=0, vco2=1, pw=2, hpf=3, lpf=4, hpeak=5, lpeak=6, vca=7, mgrate=8, lvl1=9, lvl2=10)

INIT = {
    "volume": 7.0, "voice_mode": 0, "filter_model": 0, "quality": 0, "bend_range": 2,
    "drift": 3.0, "drive": 3.0, "legato": 1, "portamento": 0.0, "master_tune": 0.0, "vca_gain": 0.0,
    "vco1_wave": SAW, "vco1_pw": 0.0, "vco1_scale": F8,
    "vco2_wave": SAW2, "vco2_pitch": 0.0, "vco2_scale": F8,
    "vco1_level": 8.0, "vco2_level": 0.0, "fm_mg": 0.0, "fm_eg1": 0.0,
    "hpf_cutoff": 0.0, "hpf_peak": 0.0, "hpf_mod_mg": 0.0, "hpf_mod_eg2": 0.0,
    "lpf_cutoff": 10.0, "lpf_peak": 0.0, "lpf_mod_mg": 0.0, "lpf_mod_eg2": 0.0,
    "mg_wave": 5.0, "mg_freq": 4.0,
    "eg1_delay": 0.0, "eg1_attack": 0.0, "eg1_release": 3.0,
    "eg2_hold": 0.0, "eg2_attack": 0.0, "eg2_decay": 5.0, "eg2_sustain": 10.0, "eg2_release": 3.0,
    "esp_mode": 0, "esp_level": 5.0, "esp_lowcut": 0.0, "esp_highcut": 10.0,
    "esp_cv_adjust": 5.0, "esp_threshold": 3.0,
}
for i in range(1, 7):
    INIT[f"patch{i}_src"] = 0
    INIT[f"patch{i}_dst"] = 0
    INIT[f"patch{i}_amt"] = 5.0

ALIAS = {
    "w1": "vco1_wave", "pw": "vco1_pw", "s1": "vco1_scale",
    "w2": "vco2_wave", "p2": "vco2_pitch", "s2": "vco2_scale",
    "l1": "vco1_level", "l2": "vco2_level", "fmg": "fm_mg", "feg": "fm_eg1",
    "hc": "hpf_cutoff", "hp": "hpf_peak", "hmg": "hpf_mod_mg", "heg": "hpf_mod_eg2",
    "lc": "lpf_cutoff", "lp": "lpf_peak", "lmg": "lpf_mod_mg", "leg": "lpf_mod_eg2",
    "mgw": "mg_wave", "mgf": "mg_freq",
    "d1": "eg1_delay", "a1": "eg1_attack", "r1": "eg1_release",
    "h2": "eg2_hold", "a2": "eg2_attack", "dc": "eg2_decay", "su": "eg2_sustain", "r2": "eg2_release",
    "vol": "volume", "mode": "voice_mode", "rev": "filter_model", "port": "portamento",
    "drv": "drive", "drift": "drift", "vca": "vca_gain", "leg1": "legato", "tune": "master_tune",
    "esp": "esp_mode", "espl": "esp_level", "esplc": "esp_lowcut", "esphc": "esp_highcut",
    "espcv": "esp_cv_adjust", "espth": "esp_threshold", "bend": "bend_range",
}

PRESETS = []


def p(name, cat, cables=(), **kw):
    params = dict(INIT)
    for k, v in kw.items():
        params[ALIAS.get(k, k)] = v
    for i, (src, dst, amt) in enumerate(cables, start=1):
        params[f"patch{i}_src"] = SRC[src]
        params[f"patch{i}_dst"] = DST[dst]
        params[f"patch{i}_amt"] = amt
    assert all(k in INIT for k in params), name
    PRESETS.append({"name": name, "category": cat, "author": "Reckless", "params": params})


KBD = ("kbd", "lpf", 6)  # common half-tracking cable

# ---------------------------------------------------------------- Basic
C = "Basic"
p("Init Patch", C)
p("Init Two VCO", C, l2=8, p2=0.03)
p("Init Square", C, w1=PULSE, lc=7, dc=5, su=8)
p("Init Poly", C, mode=1, l2=7, p2=0.03, lc=7.5, r2=4)

# ---------------------------------------------------------------- Bass
C = "Bass"
p("Thick MS Bass", C, s1=F16, w2=SQR2, s2=F16, p2=0.04, l1=9, l2=7, lc=4.2, lp=3, leg=4, dc=4.5, su=2, r2=2.5)
p("Sub Rumble", C, w1=TRI, s1=F32, w2=SQR2, s2=F16, l1=10, l2=4, lc=3.5, leg=2, dc=6, su=7, r2=3, drv=5)
p("Rubber Pluck", C, s1=F16, lc=3, lp=6.5, leg=5.5, dc=4, su=0, r2=3.5, cables=[KBD])
p("Growl Bass", C, s1=F16, w2=SAW2, s2=F16, p2=0.08, l2=8, lc=4, lp=7.5, leg=3, lmg=1.5, mgf=6.2, dc=6, su=5, drv=7)
p("Punch Square Bass", C, w1=PULSE, pw=2, s1=F16, w2=SQR2, s2=F32, l2=6, lc=4.5, lp=2, leg=3.5, dc=3.5, su=3, r2=2)
p("Rev2 Smooth Bass", C, rev=1, s1=F16, w2=SAW2, s2=F16, p2=-0.04, l2=7, lc=4.6, lp=4, leg=3, dc=5, su=5, r2=3)
p("Fifth Bass", C, s1=F16, w2=SAW2, s2=F16, p2=2.92, l1=9, l2=6, lc=4.4, lp=4, leg=3.5, dc=4.5, su=3, r2=2.5)
p("Knock Bass", C, w1=PULSE, pw=5, s1=F16, l1=10, lc=3, lp=5, leg=6, dc=3, su=0, r2=2, feg=1.5, a1=0, r1=2.5)
p("Glide Mono Bass", C, s1=F16, w2=SQR2, s2=F16, p2=0.05, l2=7, lc=4.2, lp=5, leg=3.5, dc=5, su=5, port=4.5)
p("Hollow Bass", C, w1=PULSE, pw=0, s1=F16, l1=9, hc=3.5, hp=4, lc=5, lp=3, leg=2.5, dc=5, su=6)
p("Wobble Bass", C, s1=F16, w2=SAW2, s2=F16, p2=0.06, l2=8, lc=3.6, lp=6.5, lmg=4.5, mgw=5, mgf=5.8, su=10, r2=2.5, drv=6)
p("Dirty Saw Bass", C, s1=F16, w2=SAW2, s2=F32, l1=10, l2=6, lc=5, lp=6, leg=3, dc=5, su=6, drv=10)
p("Velvet Bass", C, rev=1, w1=TRI, s1=F16, w2=SQR2, s2=F16, l1=9, l2=3, lc=4.5, leg=2, a2=1, dc=6, su=7, r2=4)
p("Snappy Pulse Bass", C, w1=PULSE, pw=6, s1=F16, l1=10, lc=3.5, lp=4, leg=6, dc=3.2, su=1, r2=2, cables=[("eg2", "pw", -4)])
p("Ring Bass", C, s1=F16, w2=RING2, s2=F16, p2=0.5, l1=6, l2=7, lc=4, lp=4, leg=4, dc=4, su=2)
p("Sync-ish Bass", C, s1=F16, w2=SAW2, s2=F8, l1=7, l2=7, lc=4.6, lp=5, leg=3, dc=4, su=4, cables=[("eg2", "vco2", 4)])
p("Moog Killer", C, rev=1, s1=F16, w2=SAW2, s2=F16, p2=0.05, l1=9, l2=9, lc=3.8, lp=5.5, leg=4.2, dc=5.5, su=3, r2=3, drv=4)
p("Picked Bass", C, w1=TRI, s1=F16, w2=SAW2, s2=F16, l1=8, l2=5, lc=3.4, lp=3, leg=5, dc=3.8, su=0, r2=3, cables=[("vel", "lpf", 4)])
p("Fat 32 Foot", C, s1=F32, w2=SAW2, s2=F16, p2=0.03, l1=9, l2=8, lc=4, lp=4, leg=3.5, dc=6, su=4, drv=6)
p("Reese Hum", C, s1=F16, w2=SAW2, s2=F16, p2=0.12, l1=9, l2=9, lc=4.8, lp=2, su=10, r2=3, drift=6)
p("Deep Pulse", C, w1=PULSE, pw=3, s1=F32, l1=10, lc=4, lp=3, leg=2.5, dc=5, su=5, cables=[("mgtri", "pw", 2)], mgf=2.5)
p("Scream Bass", C, s1=F16, w2=SQR2, s2=F16, l2=8, lc=3.4, lp=8.6, leg=4.8, dc=4.5, su=2, drv=8)
p("Funk Bass", C, w1=PULSE, pw=4, s1=F16, l1=10, lc=4, lp=6, leg=4.5, dc=3, su=1, r2=1.5)
p("Kick Bass", C, w1=TRI, s1=F16, l1=10, lc=6, feg=4, a1=0, r1=2.2, dc=4, su=0, r2=2.5)
p("Bouncy Bass", C, s1=F16, lc=3.5, lp=7, leg=5, dc=3.8, su=1, port=2.5, leg1=1)
p("Nasal Bass", C, w1=PULSE, pw=7.5, s1=F16, l1=10, hc=3.2, hp=6, lc=5.5, lp=3, leg=2, dc=5, su=6)

# ---------------------------------------------------------------- Lead
C = "Lead"
p("Screaming Lead", C, w2=SQR2, p2=0.04, l2=7, lc=6, lp=8.8, leg=2.5, fmg=1.2, mgf=6, su=10, port=2, drv=6)
p("Classic Mono Lead", C, w2=SAW2, p2=0.05, l2=7, lc=6.5, lp=4, leg=2, fmg=1, mgf=5.8, d1=6, su=9, r2=3, port=1.5)
p("Whistle Lead", C, l1=0, l2=0, lc=6, lp=10, su=10, r2=3, fmg=1.1, mgf=6, cables=[("kbd", "lpf", 10)])
p("Pulse Lead", C, w1=PULSE, pw=4, w2=SQR2, s2=F4, l2=4, lc=7, lp=3, leg=1.5, su=9, fmg=1.3, mgf=6, port=1.5)
p("Hard Edge Lead", C, w2=SAW2, p2=-0.06, l2=8, lc=6.8, lp=6.5, leg=2, su=8, drv=9)
p("Fifths Lead", C, w2=SAW2, p2=2.92, l1=8, l2=6, lc=6.5, lp=5, leg=2.2, su=9, port=2.5)
p("Octave Lead", C, w2=SAW2, s2=F4, l1=8, l2=6, lc=6.8, lp=4.5, leg=1.5, su=9, port=2, fmg=1, mgf=6.2)
p("Soft Triangle Lead", C, rev=1, w1=TRI, w2=SQR2, s2=F4, l1=9, l2=3, lc=7, lp=2, a2=3, su=9, r2=4, fmg=1.2, mgf=5.6, port=3)
p("Delayed Vibrato", C, w2=SAW2, p2=0.03, l2=7, lc=6.5, lp=5, leg=1.5, su=9, r2=4, port=2,
  cables=[("eg1", "pitch", 0.4), ("mgtri", "pitch", 0)], d1=6.5, a1=6, feg=0, fmg=0)
p("Wheel Vibrato Lead", C, w2=SQR2, p2=0.03, l2=6, lc=6.5, lp=5, leg=2, su=9, port=2, mgf=6, cables=[("wheel", "pitch", 0.6)])
p("Growling Lead", C, w2=SAW2, p2=0.07, l2=8, lc=5.5, lp=7.5, lmg=1.5, mgf=6.8, leg=2.5, su=8, drv=8)
p("Narrow Pulse Lead", C, w1=PULSE, pw=8, w2=NARROW2, s2=F4, l2=4, lc=7.5, lp=3, su=9, fmg=1, mgf=6)
p("Bright Saw Lead", C, w2=SAW2, p2=0.06, l2=9, lc=8, lp=3, leg=1, su=10, r2=3, port=1)
p("Portamento Lead", C, w2=SQR2, p2=0.04, l2=7, lc=6.5, lp=6, leg=2, su=10, port=6)
p("Siren Lead", C, w1=PULSE, l1=9, lc=7, lp=6, su=10, fmg=3.5, mgf=4.5, mgw=5)
p("Rev2 Silk Lead", C, rev=1, w2=SAW2, p2=0.04, l2=7, lc=6.4, lp=7, leg=2, su=9, port=2, fmg=1.1, mgf=6)
p("Ring Lead", C, w1=PULSE, w2=RING2, p2=3.6, l1=6, l2=7, lc=7, lp=4, su=9, port=1.5)
p("Pitch Drop Lead", C, w2=SAW2, l2=7, lc=6.5, lp=5, su=9, feg=2.5, a1=0, r1=3.5, cables=[("eg1", "pitch", -0.6)])
p("Talking Lead", C, w1=PULSE, pw=5, l1=10, hc=4, hp=7, lc=6, lp=7, heg=2, leg=2, a2=4, su=8, r2=4)
p("Dual Peak Lead", C, w2=SAW2, p2=0.05, l2=8, hc=3.8, hp=8, lc=6.2, lp=8, su=9, port=1.5, drv=5)
p("Thin Lead", C, w1=PULSE, pw=9, l1=9, hc=5, lc=8, lp=2, su=10, fmg=1, mgf=6.2)
p("Scorched Lead", C, w2=SQR2, p2=0.05, l2=9, lc=7, lp=7.8, leg=1, su=10, drv=10)
p("Mellow Flute Lead", C, rev=1, w1=TRI, l1=9, lc=6, lp=1, a2=4.5, dc=5, su=8, r2=4.5, fmg=1, mgf=5.8,
  cables=[("pink", "vca", 0.6)])
p("Stabby Lead", C, w2=SAW2, p2=0.05, l2=7, lc=5.5, lp=6, leg=4, dc=4, su=3, r2=3)
p("Space Lead", C, w2=SQR2, s2=F4, p2=0.03, l2=5, lc=6.8, lp=5, su=9, r2=6, port=4, fmg=1.4, mgf=5.5, drift=7)
p("Chirp Lead", C, w1=PULSE, pw=3, lc=5, lp=7.5, leg=4.5, dc=3.2, su=6, r2=3, feg=1.4, r1=2)

# ---------------------------------------------------------------- Acid
C = "Acid"
acid_variants = [
    ("Acid Squelch", dict(w1=SAW, lc=3.6, lp=8.5, leg=5, dc=3.6, su=0)),
    ("Acid Square", dict(w1=PULSE, lc=3.8, lp=8.2, leg=4.8, dc=3.8, su=0)),
    ("Acid Scream", dict(w1=SAW, lc=4, lp=9.1, leg=5.5, dc=4, su=1, drv=9)),
    ("Acid Rubber", dict(w1=SAW, rev=1, lc=3.5, lp=8.8, leg=5, dc=3.5, su=0)),
    ("Acid Slide", dict(w1=SAW, lc=3.8, lp=8.6, leg=5, dc=4, su=1, port=3.5)),
    ("Acid Mid Bite", dict(w1=SAW, hc=2.4, hp=4, lc=4.2, lp=8.4, leg=4.5, dc=3.4, su=0)),
    ("Acid Long Decay", dict(w1=SAW, lc=3.4, lp=8.7, leg=5, dc=5.5, su=0)),
    ("Acid Accent", dict(w1=SAW, lc=3.5, lp=8.3, leg=4, dc=3.5, su=0, cables=[("vel", "lpf", 6), ("vel", "lpeak", 2)])),
    ("Acid Dirty 303", dict(w1=PULSE, pw=1, lc=3.6, lp=8.9, leg=5.2, dc=3.7, su=0, drv=10)),
    ("Acid Wah", dict(w1=SAW, lc=4.2, lp=8.6, lmg=3, mgf=4.5, leg=2, dc=5, su=6)),
    ("Acid Hollow", dict(w1=PULSE, pw=0, hc=2.6, hp=3, lc=4, lp=8.5, leg=5, dc=3.6, su=0)),
    ("Acid Ghost", dict(w1=SAW, rev=1, lc=3, lp=9.3, leg=4, dc=4.5, su=0)),
    ("Acid Octave", dict(w1=SAW, w2=SAW2, s2=F4, l2=4, lc=3.7, lp=8.4, leg=5, dc=3.6, su=0)),
    ("Acid Thump", dict(w1=SAW, s1=F16, lc=3.4, lp=8.2, leg=5.5, dc=3.2, su=0, feg=1.5, r1=2)),
    ("Acid Swirl", dict(w1=SAW, lc=4, lp=8.7, lmg=2, mgf=3, mgw=0, leg=4.5, dc=4, su=1)),
    ("Acid Resonator", dict(w1=SAW, lc=3.8, lp=9.0, leg=4.6, dc=3.8, su=0, cables=[("kbd", "lpf", 5)])),
]
for name, kw in acid_variants:
    kw.setdefault("s1", F16)
    kw.setdefault("r2", 2.5)
    kw.setdefault("drv", 6)
    p(name, C, **kw)

# ---------------------------------------------------------------- Keys & Brass (poly)
C = "Keys & Brass"
p("MS Brass", C, mode=1, w2=SAW2, p2=0.05, l2=7, lc=4.8, lp=2, leg=3.5, a2=3.5, dc=6, su=7, r2=4)
p("Brass Stabs", C, mode=1, w2=SAW2, p2=0.06, l2=8, lc=5, lp=3, leg=4, a2=2, dc=4.5, su=3, r2=3.5)
p("Mono Brass", C, w2=SAW2, p2=0.05, l2=8, lc=4.8, lp=2.5, leg=3.6, a2=3.8, dc=6, su=7, r2=4, port=1.5)
p("Hard Brass", C, mode=1, w2=SQR2, p2=0.04, l2=6, lc=5.2, lp=5, leg=3.6, a2=2.5, dc=5, su=6, drv=7)
p("Poly Keys", C, mode=1, rev=1, w2=SQR2, s2=F4, l1=8, l2=4, lc=6, lp=1, leg=2, dc=6, su=3, r2=5)
p("Clav Pluck", C, mode=1, w1=PULSE, pw=7, l1=10, hc=4, lc=5.5, lp=4, leg=4, dc=4, su=0, r2=3)
p("Electric Piano", C, mode=1, rev=1, w1=TRI, w2=SQR2, s2=F4, l1=9, l2=2, lc=6.5, leg=1.5, dc=7.2, su=0, r2=5.5,
  cables=[("vel", "lpf", 4), ("vel", "vca", 3)])
p("Organ Pipes", C, mode=1, w1=PULSE, w2=SQR2, s2=F4, l1=8, l2=6, lc=7.2, lp=0, su=10, r2=2.5)
p("Reed Organ", C, mode=1, w1=PULSE, pw=6, w2=NARROW2, s2=F16, l1=8, l2=5, lc=6.8, lp=2, a2=2, su=10, r2=3)
p("Harpsi Pluck", C, mode=1, w1=PULSE, pw=8, w2=SAW2, s2=F4, l1=8, l2=5, hc=4.5, lc=7.5, leg=1.5, dc=5.5, su=0, r2=5)
p("Muted Trumpet", C, w1=PULSE, pw=2, lc=5, lp=4, leg=3, a2=3, dc=5, su=7, fmg=1, mgf=6, d1=5, port=1)
p("Sax Lead", C, w1=PULSE, pw=5, hc=3.5, hp=4, lc=5.5, lp=5, leg=2.2, a2=3, su=8, fmg=1, mgf=6, port=1.5)
p("Bell Keys", C, mode=1, w1=TRI, w2=RING2, s2=F4, p2=1.25, l1=6, l2=7, lc=8, dc=7.5, su=0, r2=6)
p("Poly Strings", C, mode=1, w2=SAW2, p2=0.06, l2=8, lc=6.2, lp=1, a2=6, dc=6, su=9, r2=6.5, fmg=0.8, mgf=5.5, drift=6)
p("Mono Strings", C, w2=SAW2, p2=0.07, l2=8, lc=6.3, lp=1, a2=6, su=9, r2=6, fmg=0.8, mgf=5.5, port=2)
p("Synth Horn", C, mode=1, w2=SAW2, s2=F16, p2=0.03, l2=6, lc=4.6, lp=3, leg=3.8, a2=4.5, dc=6, su=8, r2=4.5)

# ---------------------------------------------------------------- Pad & Drone
C = "Pad & Drone"
p("Warm Poly Pad", C, mode=1, rev=1, w2=SAW2, p2=0.07, l2=8, lc=5.6, lp=2, leg=1.5, a2=7, dc=7, su=8, r2=7.5, drift=6)
p("Dark Pad", C, mode=1, w2=SQR2, s2=F16, p2=0.05, l2=7, lc=4.6, lp=3, a2=7.5, su=9, r2=7.5, lmg=0.6, mgf=3)
p("Sweep Pad", C, mode=1, w2=SAW2, p2=0.08, l2=8, lc=4.5, lp=6, lmg=3, mgf=2, a2=7, su=9, r2=7)
p("Glass Pad", C, mode=1, w1=PULSE, pw=6, w2=SQR2, s2=F4, l2=5, hc=4, lc=7, lp=4, a2=6.5, su=8, r2=7.5,
  cables=[("mgtri", "pw", 2.5)], mgf=3.5)
p("Choir Pad", C, mode=1, rev=1, w1=PULSE, pw=4, w2=SAW2, p2=0.05, l2=6, hc=3.5, hp=4, lc=5.8, lp=4, a2=7, su=9, r2=7, drift=6)
p("Analog Drone", C, s1=F16, w2=SAW2, s2=F16, p2=0.1, l2=9, lc=5, lp=6, lmg=2, mgf=1.5, vca=8, su=0, dc=1, drift=8)
p("Fifth Drone", C, s1=F16, w2=SAW2, s2=F16, p2=2.92, l2=8, lc=4.8, lp=7, lmg=2.5, mgf=1, vca=8, drift=7)
p("Dark Matter", C, w1=NOISE, s1=F16, w2=SAW2, s2=F32, l1=4, l2=9, lc=4, lp=8, lmg=3, mgf=0.8, vca=8)
p("Resonant Drone", C, w2=SQR2, s2=F16, l1=6, l2=6, hc=3, hp=7, lc=5.5, lp=8.8, hmg=2, lmg=2, mgf=1.2, mgw=0, vca=7)
p("Ocean Pad", C, mode=1, w1=NOISE, w2=SAW2, p2=0.06, l1=5, l2=7, lc=5, lp=5, lmg=3.5, mgf=1, a2=8, su=9, r2=8)
p("Slow Bloom", C, mode=1, w2=SAW2, p2=0.06, l2=8, lc=3.5, lp=4, leg=4, a2=8, dc=8, su=8, r2=8)
p("Cosmic Pad", C, mode=1, w2=SQR2, s2=F4, p2=0.05, l2=5, lc=6, lp=7, lmg=2, mgf=2.5, a2=7, su=9, r2=8.5, drift=8)
p("Breathing Pad", C, mode=1, w1=TRI, w2=SAW2, p2=0.07, l1=8, l2=6, lc=5, lp=3, lmg=2.5, mgf=2.2, mgw=5, a2=6.5, su=9, r2=7,
  cables=[("mgtri", "vca", 1.5)])
p("Noise Wind", C, w1=NOISE, l1=10, lc=5.5, lp=8.7, lmg=4, mgf=1.3, vca=7, hc=2)
p("Cathedral Drone", C, mode=1, w1=PULSE, w2=SAW2, s2=F16, l1=7, l2=7, lc=5.5, lp=3, a2=7.5, su=10, r2=8.5, drift=5)
p("Detuned Haze", C, mode=1, w2=SAW2, p2=0.16, l2=9, lc=5, lp=2, a2=6, su=9, r2=7.5, drift=9)
p("Pulsing Drone", C, s1=F16, w2=SQR2, s2=F16, p2=0.05, l2=7, lc=5, lp=5, vca=0, su=10,
  cables=[("mgsq", "vca", 8)], mgf=5.5)
p("Infinite Hum", C, w1=TRI, s1=F32, w2=SAW2, s2=F16, p2=0.04, l1=10, l2=6, lc=4.5, lp=4, vca=9, drv=6, drift=8)

# ---------------------------------------------------------------- Sequence & S&H
C = "Sequence & S&H"
p("Random Bleeps", C, w1=PULSE, l1=9, lc=5, lp=8.7, su=10, mgf=6, cables=[("sh", "lpf", 5)])
p("S&H Pitch Melody", C, w1=PULSE, pw=3, lc=6.5, lp=5, su=10, mgf=6.5, cables=[("sh", "pitch", 3)])
p("Computer Talk", C, w1=SAW, lc=5.8, lp=9, mgf=7, su=10, cables=[("sh", "lpf", 6), ("sh", "pitch", 0.8)])
p("Random Arp Bass", C, s1=F16, w2=SQR2, s2=F16, l2=6, lc=4.5, lp=7, mgf=5.8, su=10, r2=2, cables=[("sh", "lpf", 3)])
p("Stepped Filter", C, s1=F16, w2=SAW2, s2=F16, p2=0.05, l2=8, lc=5, lp=8, mgf=5.2, su=10, cables=[("sh", "lpf", 4.5)])
p("Gated Saws", C, w2=SAW2, p2=0.06, l2=8, lc=5.5, lp=5, mgf=6.2, su=10, cables=[("mgsq", "vca", 10)])
p("Trance Gate", C, mode=1, w2=SAW2, p2=0.07, l2=8, lc=6, lp=4, mgf=7, su=10, r2=4, cables=[("mgsq", "vca", 10)])
p("Bubble Machine", C, w1=TRI, l1=10, lc=5, lp=8.5, mgf=6, su=10, cables=[("sh", "pitch", 2), ("sh", "lpf", 3)])
p("Tick Tock", C, w1=PULSE, pw=8, l1=10, hc=6, hp=6, lc=8, mgf=5.5, su=10, cables=[("mgsq", "vca", 10), ("mgsq", "pitch", 2)])
p("Square Trill", C, w1=PULSE, l1=9, lc=6.5, lp=4, mgf=6.8, su=10, cables=[("mgsq", "pitch", 0.5)])
p("Arp Fifths", C, w2=SAW2, l1=8, l2=6, p2=2.92, lc=6, lp=6, mgf=6.4, su=10, cables=[("mgsq", "pitch", 0.5), ("mgsq", "vca", 6)])
p("Random Resonance", C, w2=SAW2, s2=F16, l2=6, lc=6, lp=6, mgf=5.5, su=10, cables=[("sh", "lpeak", 4), ("sh", "lpf", 2)])
p("Wandering Wave", C, w1=PULSE, l1=9, lc=6, lp=6, mgf=4.5, su=10, cables=[("sh", "pw", 8), ("pink", "lpf", 2)])
p("Clockwork", C, w1=PULSE, pw=6, hc=5, hp=5, lc=7, lp=6, mgf=6.8, su=10, cables=[("mgsq", "vca", 10), ("sh", "hpf", 3)])

# ---------------------------------------------------------------- Percussion
C = "Percussion"
p("Analog Kick", C, w1=TRI, l1=10, lc=6, feg=4.5, a1=0, r1=2, dc=4.2, su=0, r2=4, drv=5)
p("Deep Kick", C, w1=TRI, s1=F16, l1=10, lc=5, feg=4.8, a1=0, r1=2.3, dc=5, su=0, r2=5, drv=6)
p("Snare", C, w1=NOISE, w2=SQR2, s2=F4, l1=9, l2=4, hc=4.5, lc=8, leg=1, dc=3.6, su=0, r2=3.6)
p("Clap", C, w1=NOISE, l1=10, hc=5, hp=3, lc=7.5, lp=4, dc=3.4, su=0, r2=3.4, cables=[("mgsq", "vca", 4)], mgf=8.5)
p("Closed Hat", C, w1=NOISE, l1=10, hc=7.4, hp=4, dc=2, su=0, r2=2)
p("Open Hat", C, w1=NOISE, l1=10, hc=7.8, hp=4, dc=4.6, su=0, r2=4.6)
p("Analog Tom", C, w1=TRI, l1=10, lc=6.5, feg=2.5, r1=3.5, dc=4.6, su=0, r2=4.6)
p("Zap", C, w1=PULSE, l1=10, lc=7, lp=5, feg=5, r1=2.8, dc=3.5, su=0, r2=3.5)
p("Resonant Ping", C, l1=0, lc=6.5, lp=9.9, leg=2, dc=4, su=0, r2=4, cables=[("kbd", "lpf", 10)])
p("Wood Block", C, w1=TRI, l1=10, hc=4.2, lc=7.5, lp=6, dc=2.6, su=0, r2=2.6)
p("Cowbell", C, w1=PULSE, w2=SQR2, p2=3.4, l1=7, l2=7, hc=5.5, hp=4, lc=7.5, dc=3.4, su=0, r2=3.4)
p("Metal Hit", C, w1=PULSE, w2=RING2, p2=4.3, l1=5, l2=9, hc=5, lc=9, dc=5, su=0, r2=5)
p("Laser Snare", C, w1=NOISE, w2=SQR2, l1=7, l2=6, lc=8, lp=5, feg=3, r1=2.6, dc=3.5, su=0, r2=3.5)
p("Noise Burst", C, w1=NOISE, l1=10, lc=6, lp=6, leg=4, dc=3, su=0, r2=3)
p("Blip", C, w1=PULSE, l1=10, s1=F4, lc=8, dc=2.2, su=0, r2=2.2)
p("Conga", C, w1=TRI, l1=10, lc=6, lp=6, feg=1.6, r1=2.5, dc=4.2, su=0, r2=4.2)
p("Rim Shot", C, w1=PULSE, w2=SQR2, s2=F4, l1=8, l2=6, hc=5.5, hp=5, lc=8, dc=2.4, su=0, r2=2.4)
p("Shaker", C, w1=NOISE, l1=10, hc=8.4, hp=2, a2=2.5, dc=3.2, su=0, r2=3.2)

# ---------------------------------------------------------------- FX
C = "FX"
p("Alien Siren", C, w1=PULSE, lc=8, lp=4, su=10, fmg=5, mgf=4.2)
p("Laser Gun", C, w1=PULSE, s1=F4, lc=8, lp=7, feg=7, r1=3.6, dc=4, su=0, r2=4, cables=[("eg1rev", "pitch", 3)])
p("Space Wind", C, w1=NOISE, l1=10, lc=6, lp=9.3, lmg=4, mgf=1.6, su=10, a2=6, r2=7)
p("Bomb Drop", C, w1=TRI, l1=10, lc=7, su=10, a2=0, r2=8, cables=[("eg1rev", "pitch", 10)], a1=8.6, r1=6)
p("Riser", C, w1=NOISE, w2=SAW2, l1=7, l2=6, lc=3, lp=8, leg=6, a2=8.6, su=10, r2=5, cables=[("eg2", "pitch", 3)])
p("Robot Voice", C, w1=PULSE, w2=RING2, p2=1.8, l1=5, l2=8, hc=4, hp=6, lc=6, lp=7, su=10, cables=[("sh", "hpf", 3)], mgf=6.5)
p("UFO", C, w1=TRI, l1=10, lc=8, su=10, r2=6, fmg=4, mgf=6.5, cables=[("mgtri", "mgrate", 4)])
p("Theremin", C, rev=1, w1=TRI, l1=9, lc=8, su=10, a2=4, r2=5, fmg=1.4, mgf=6.2, port=6)
p("Whistling Peak", C, l1=0, l2=0, lc=7, lp=10, lmg=0.5, mgf=6, su=10, r2=5, cables=[("kbd", "lpf", 10)])
p("Feedback Howl", C, w1=NOISE, l1=3, hc=6, hp=9.5, lc=7, lp=9.5, hmg=1, lmg=1, mgf=1, su=10, r2=6)
p("Zapper", C, w1=PULSE, lc=9, lp=6, feg=8, r1=3, dc=3.5, su=0, r2=3.5, cables=[("eg1rev", "pitch", 5)])
p("Dive Bomb", C, w2=SQR2, l2=8, lc=7, lp=6, su=10, r1=8, a1=0, cables=[("eg1rev", "pitch", 6)], feg=0, port=0)
p("Radio Tuning", C, w1=NOISE, w2=SAW2, l1=6, l2=7, lc=6, lp=8, su=10, fmg=3, mgf=7.5, cables=[("sh", "pitch", 4)])
p("Underwater", C, w1=TRI, w2=SQR2, l1=8, l2=5, lc=5, lp=8.5, lmg=4, mgf=5, su=10, r2=4)
p("Helicopter", C, w1=NOISE, l1=10, lc=5.5, lp=6, su=10, cables=[("mgsq", "vca", 10)], mgf=7.7)
p("Thunder", C, w1=NOISE, s1=F32, l1=10, lc=4.6, lp=6, lmg=3, mgf=3, a2=2, dc=8, su=0, r2=8, drv=10)
p("Steam Valve", C, w1=NOISE, l1=10, hc=6, hp=5, lc=9, a2=3, dc=6, su=0, r2=6)
p("Insect Swarm", C, w1=PULSE, w2=SQR2, p2=0.4, l1=7, l2=7, hc=6, lc=8, lp=4, su=10, fmg=2, mgf=8.5)
p("Glitch Rain", C, w1=PULSE, s1=F4, lc=7, lp=8, su=10, mgf=8.8, cables=[("sh", "pitch", 5), ("mgsq", "vca", 10)])
p("Engine Rev", C, w1=PULSE, s1=F32, w2=SQR2, s2=F32, p2=0.2, l2=8, lc=5, lp=6, su=10, cables=[("wheel", "pitch", 8), ("wheel", "lpf", 4)])
p("Ray Beam", C, w1=TRI, w2=RING2, l1=5, l2=7, lc=8, su=10, fmg=2.5, mgf=7)
p("Wobbly Tape", C, w2=SAW2, p2=0.08, l2=7, lc=6, lp=4, su=10, fmg=1.6, mgf=3.5, drift=10)
p("Sonar Ping", C, w1=TRI, s1=F4, l1=10, lc=8, lp=8, leg=1, dc=7.8, su=0, r2=7.8)
p("Self-Osc Swoop", C, l1=0, l2=0, lc=4, lp=10, leg=6, a2=6, dc=7, su=0, r2=6)

# ---------------------------------------------------------------- Ring & Metal
C = "Ring & Metal"
p("Ring Bells", C, mode=1, w1=PULSE, w2=RING2, p2=2.1, l1=4, l2=9, lc=8.5, dc=7.5, su=0, r2=7)
p("Clang", C, w1=PULSE, w2=RING2, p2=3.3, l1=3, l2=10, hc=4, lc=8, lp=4, dc=5.5, su=0, r2=5.5)
p("Gong", C, w1=TRI, w2=RING2, s2=F16, p2=1.13, l1=6, l2=9, lc=7, lp=3, a2=1, dc=8.4, su=0, r2=8.4, drv=5)
p("Tubular", C, mode=1, w1=PULSE, w2=RING2, s2=F4, p2=1.5, l1=4, l2=9, lc=8, dc=8, su=0, r2=7.5)
p("Ring Sweep", C, w1=PULSE, w2=RING2, l1=4, l2=9, lc=6, lp=6, su=10, cables=[("mgtri", "vco2", 2)], mgf=3)
p("Steel Drum", C, mode=1, w1=TRI, w2=RING2, p2=5, l1=8, l2=5, lc=7, lp=5, dc=5.4, su=0, r2=5.4)
p("Broken Radio", C, w1=PULSE, w2=RING2, p2=0.7, l1=5, l2=9, hc=5, hp=6, lc=6.5, lp=6, su=10, cables=[("sh", "vco2", 3)], mgf=7)
p("Robot Pluck", C, w1=PULSE, w2=RING2, p2=1.25, l1=5, l2=9, lc=5, lp=6, leg=5, dc=4, su=0, r2=4)
p("Anvil", C, w1=PULSE, w2=RING2, s2=F16, p2=4.2, l1=4, l2=10, hc=5.5, lc=9, dc=4.2, su=0, r2=4.2, drv=8)
p("Xylo Ring", C, mode=1, w1=TRI, w2=RING2, s2=F4, l1=7, l2=6, lc=7.5, dc=4.6, su=0, r2=4.6)
p("Chime Tree", C, mode=1, w1=PULSE, w2=RING2, s2=F4, p2=3.6, l1=3, l2=9, hc=5, lc=9, dc=7, su=0, r2=7.5)
p("Metallic Bass", C, s1=F16, w2=RING2, s2=F16, p2=0.6, l1=7, l2=8, lc=4.5, lp=6, leg=4, dc=4.4, su=2)

# ---------------------------------------------------------------- Classic MS
C = "Classic MS"
p("Patch 1: Dual VCO", C, w2=SAW2, p2=0.05, l2=8, lc=6, lp=4, leg=2, su=9, fmg=1, mgf=5.8)
p("Patch 2: Peak Scream", C, w2=SQR2, l2=8, lc=5, lp=9.2, hc=3, hp=7, leg=3, su=8, drv=7)
p("Patch 3: Keyboard Filter", C, l1=0, l2=0, lc=6, lp=10, su=10, cables=[("kbd", "lpf", 10)])
p("Patch 4: EG1 FM", C, w1=PULSE, lc=7, lp=4, su=9, feg=3, a1=4, r1=5)
p("Patch 5: Delayed MG", C, w2=SAW2, l2=7, lc=6.5, lp=5, su=9, cables=[("eg1", "pitch", 0.3)], d1=7, a1=6.5)
p("Patch 6: Noise Sweep", C, w1=NOISE, l1=10, lc=4, lp=9, leg=7, a2=6, dc=7, su=0, r2=7)
p("Patch 7: HPF Scream", C, w1=SAW, hc=4, hp=9.6, lc=8, leg=0, heg=3, su=10, drv=7)
p("Patch 8: Sample & Hold", C, w1=PULSE, lc=5.5, lp=8.2, su=10, mgf=6, cables=[("sh", "lpf", 4.5)])
p("Patch 9: Ring Bells", C, w1=PULSE, w2=RING2, p2=1.6, l1=3, l2=9, lc=8, dc=7, su=0, r2=7)
p("Patch 10: Big Bass", C, s1=F32, w2=SAW2, s2=F16, p2=0.04, l2=9, lc=4, lp=6, leg=4, dc=5, su=4, drv=7)
p("Patch 11: Pulse Mod", C, w1=PULSE, l1=10, lc=6.5, lp=3, su=10, mgf=3.5, cables=[("mgtri", "pw", 4)])
p("Patch 12: Inverted Sweep", C, w2=SAW2, l2=7, lc=8, lp=6, su=10, a2=4, r2=5, cables=[("eg2rev", "lpf", 6)])
p("Patch 13: Twin Peaks", C, w2=SAW2, p2=0.06, l2=8, hc=4.5, hp=8.6, lc=5.5, lp=8.6, hmg=2, lmg=2, mgf=3.5, mgw=0, su=10)
p("Patch 14: Portamento Sweep", C, w2=SQR2, l2=7, lc=6, lp=6, su=10, port=7)
p("Patch 15: Velocity Bite", C, w2=SAW2, l2=7, lc=4.5, lp=6, leg=4, dc=5, su=3, cables=[("vel", "lpf", 7)])
p("Patch 16: Wheel Filter", C, w2=SAW2, p2=0.05, l2=8, lc=4, lp=7.5, su=10, cables=[("wheel", "lpf", 9)])

# ---------------------------------------------------------------- ESP (external input)
C = "ESP External"
p("ESP Voice Filter", C, esp=1, l1=0, l2=0, lc=6.5, lp=7, hc=3, hp=3, vca=10, su=10, espl=5, esplc=2, esphc=8)
p("ESP Pitch Follower", C, esp=2, w2=SAW2, l2=7, lc=6, lp=5, su=10, r2=3, espl=6, esplc=3, esphc=6, espth=3)
p("ESP Envelope Wah", C, esp=1, l1=0, l2=0, lc=4, lp=8, vca=10, cables=[("espenv", "lpf", 8)], espl=5, esplc=1)
p("ESP Guitar Synth", C, esp=3, s1=F16, w2=SQR2, l1=6, l2=6, lc=5, lp=6, leg=4, dc=5, su=6, espl=6, esplc=3, esphc=6.5)
p("ESP Drum Trigger", C, esp=1, w1=TRI, l1=10, lc=6, feg=4.5, r1=2, dc=4.2, su=0, r2=4, espth=5, espl=5)
p("ESP Octave Down", C, esp=2, s1=F16, w2=SQR2, s2=F32, l1=8, l2=6, lc=4.8, lp=4, su=10, espl=6, esplc=3, esphc=6)


def apply_levels(levels_path):
    """Adjusts each preset's volume knob towards a common RMS target."""
    levels = json.loads(pathlib.Path(levels_path).read_text())
    measured_volume = {}
    if OUT.exists():
        for old in json.loads(OUT.read_text())["presets"]:
            measured_volume[old["name"]] = old["params"]["volume"]
    target_db = -14.0
    for preset in PRESETS:
        rms_db = levels.get(preset["name"])
        if rms_db is None or rms_db < -80:
            continue
        # volume the measurement was taken with (the current JSON), so passes can iterate
        vol = measured_volume.get(preset["name"], preset["params"]["volume"])
        # output gain is proportional to (vol/10)^2 before the soft limiter
        gain = 10 ** ((target_db - rms_db) / 20.0)
        new_vol = max(2.0, min(10.0, vol * math.sqrt(gain)))
        preset["params"]["volume"] = round(new_vol, 2)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--levels", help="JSON map of preset name -> RMS dBFS from HS20Tests --levels")
    args = parser.parse_args()

    names = [p_["name"] for p_ in PRESETS]
    dupes = {n for n in names if names.count(n) > 1}
    assert not dupes, f"duplicate preset names: {dupes}"

    if args.levels:
        apply_levels(args.levels)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({"version": 1, "presets": PRESETS}, indent=1))
    cats = {}
    for p_ in PRESETS:
        cats[p_["category"]] = cats.get(p_["category"], 0) + 1
    print(f"Wrote {len(PRESETS)} presets to {OUT.relative_to(ROOT)}")
    for c, n in cats.items():
        print(f"  {c:16s} {n}")


if __name__ == "__main__":
    main()
