import {
  fmtFloat,
  fmtInt,
  hexToRgbComment,
  escapeCString,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ExtraAsset,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/effect_atelier/effect_atelier.h?raw";
import mainRaw from "../../native/effects/effect_atelier/main.c?raw";
import readmeRaw from "../../native/effects/effect_atelier/README.md?raw";
import raylibHeaderUrl from "../../native/effects/raylib.h?url";
import rlglHeaderUrl from "../../native/effects/rlgl.h?url";
import libraylibWinUrl from "../../native/effects/win/libraylib.a?url";
import libraylibLnxUrl from "../../native/effects/lnx/libraylib.a?url";

// --- 1. Parameter definition (Inspector) ------------------------------------

const ELEMENT_OPTIONS = ["neutral", "fire", "water", "earth", "wind", "lightning", "dark", "poison", "light", "ice"];
const SHAPE_OPTIONS = ["sphere", "ring", "spiral", "beam", "pillar", "rain", "wave", "projectile", "jump", "shield", "field", "fire_orbs", "wind_spin", "fire_wind", "water_ring", "earth_burst", "fire_burst", "lightning_burst", "poison_burst", "heal_aura", "dark_slash", "barrier", "fireball", "wind_slash", "rock_throw", "lightning_bolt", "water_jet", "ice_shard", "poison_orb", "dark_orb", "light_arrow", "bubble_burst"];

/** Default core/mid/outer palette per element — this is what makes picking a
 * new element in the Inspector actually change the look, instead of leaving
 * whatever colors happened to be set before. Applied automatically by
 * useAppStore.setParam (see the "effect_atelier" branch there) whenever an
 * "element" field (base or combo layer) changes; colorCore/Mid/Outer stay
 * fully overridable afterwards. */
export const ELEMENT_PALETTES: Record<string, { core: string; mid: string; outer: string }> = {
  neutral:   { core: "#E6E6E6", mid: "#B4B4B4", outer: "#6E6E6E" },
  fire:      { core: "#FFE08C", mid: "#FF7818", outer: "#FF3C00" },
  water:     { core: "#C8F0FF", mid: "#3CAAFF", outer: "#0A5AC8" },
  earth:     { core: "#E6C88C", mid: "#B47832", outer: "#6E4614" },
  wind:      { core: "#E6FFEB", mid: "#78FFAA", outer: "#28C86E" },
  lightning: { core: "#FFFFDC", mid: "#FFFF3C", outer: "#A0DCFF" },
  dark:      { core: "#C88CFF", mid: "#703CA8", outer: "#280F46" },
  poison:    { core: "#C8FFB4", mid: "#50C837", outer: "#1E6E14" },
  light:     { core: "#FFFFFF", mid: "#FFF050", outer: "#FFC83C" },
  ice:       { core: "#EBFAFF", mid: "#78DCFF", outer: "#3C96DC" },
};

function comboLayerParams(n: 2 | 3 | 4, defaults: { element: string; shape: string }): EffectDefinition<"effect_atelier">["params"] {
  const group = `Combo Layer ${n}`;
  const palette = ELEMENT_PALETTES[defaults.element];
  return [
    { key: `layer${n}Enabled`, label: `Layer ${n} Enabled`, type: "bool", default: false, group },
    { key: `layer${n}Element`, label: "Element", type: "select", default: defaults.element, options: ELEMENT_OPTIONS, group, showWhen: { key: `layer${n}Enabled`, equals: true } },
    { key: `layer${n}Shape`, label: "Shape", type: "select", default: defaults.shape, options: SHAPE_OPTIONS, group, showWhen: { key: `layer${n}Enabled`, equals: true } },
    { key: `layer${n}ParticleCount`, label: "Particle Count", type: "int", default: n === 2 ? 24 : 18, min: 4, max: 64, step: 1, group, showWhen: { key: `layer${n}Enabled`, equals: true } },
    { key: `layer${n}ColorCore`, label: "Core Color", type: "color", default: palette.core, group, showWhen: { key: `layer${n}Enabled`, equals: true } },
    { key: `layer${n}ColorMid`, label: "Mid Color", type: "color", default: palette.mid, group, showWhen: { key: `layer${n}Enabled`, equals: true } },
    { key: `layer${n}ColorOuter`, label: "Outer Color", type: "color", default: palette.outer, group, showWhen: { key: `layer${n}Enabled`, equals: true } },
  ];
}

const definition: EffectDefinition<"effect_atelier"> = {
  id: "effect_atelier",
  name: "Effect Atelier",
  description:
    "3D world-space effect on an orbiting camera — same billboard-circle look as Enuma Ichor's CardVFX renderers, plus real-geometry shapes (hex-panel shield arc and full hex-dome field, both ported 1:1 from _CVFX_DrawHexPanel, orbiting fireballs, spinning wind funnel, a fire_wind shape that unifies the two into one fire tornado, water_ring stomp shockwaves ported from CVT_WATER_BURST's wave walls, five more elemental bursts/auras — earth_burst, fire_burst, lightning_burst, poison_burst, heal_aura, dark_slash and barrier — and nine traveling spell projectiles — fireball, wind_slash, rock_throw, lightning_bolt, water_jet, ice_shard, poison_orb, dark_orb and light_arrow — all procedural, driven purely by time/index so they carry no extra per-particle state). Thirty-one shapes × 10 elements, with colors that follow the element automatically. Enable up to 3 combo layers (4 shapes total) to play several elements/shapes at once from the same origin. Name it and export a ready-to-paste preset for the game.",
  params: [
    { key: "element", label: "Element", type: "select", default: "fire", options: ELEMENT_OPTIONS, group: "Element" },
    { key: "mode", label: "Shape", type: "select", default: "sphere", options: SHAPE_OPTIONS, group: "Element" },
    { key: "presetName", label: "Preset Name", type: "string", default: "VFX_", group: "Element" },

    { key: "particleCount", label: "Particle Count", type: "int", default: 20, min: 4, max: 64, step: 1, group: "Emission" },
    { key: "spawnRadiusMin", label: "Spawn Radius Min", type: "float", default: 0.3, min: 0, max: 3, step: 0.01, group: "Emission", modes: ["sphere"] },
    { key: "spawnRadiusMax", label: "Spawn Radius Max", type: "float", default: 1.0, min: 0, max: 3, step: 0.01, group: "Emission", modes: ["sphere"] },
    { key: "radius", label: "Radius / Height / Travel", type: "float", default: 1.0, min: 0, max: 3, step: 0.01, group: "Emission", modes: ["ring", "spiral", "pillar", "rain", "wave", "projectile", "jump", "shield", "field", "water_ring", "earth_burst", "poison_burst", "barrier", "fireball", "wind_slash", "rock_throw", "lightning_bolt", "water_jet", "ice_shard", "poison_orb", "dark_orb", "light_arrow", "bubble_burst"] },
    { key: "directionYaw", label: "Direction (yaw °)", type: "float", default: 0, min: -180, max: 180, step: 1, group: "Emission", modes: ["beam", "projectile", "fireball", "wind_slash", "rock_throw", "lightning_bolt", "water_jet", "ice_shard", "poison_orb", "dark_orb", "light_arrow", "bubble_burst"] },
    { key: "speedMin", label: "Speed Min", type: "float", default: 1.2, min: 0, max: 10, step: 0.05, group: "Emission" },
    { key: "speedMax", label: "Speed Max", type: "float", default: 2.8, min: 0, max: 10, step: 0.05, group: "Emission" },
    { key: "lifeMin", label: "Life Min", type: "float", default: 0.35, min: 0.05, max: 3, step: 0.01, group: "Emission" },
    { key: "lifeMax", label: "Life Max", type: "float", default: 0.8, min: 0.05, max: 3, step: 0.01, group: "Emission" },
    { key: "loopInterval", label: "Loop Interval", type: "float", default: 1.2, min: 0.2, max: 5, step: 0.05, group: "Emission" },

    { key: "gravity", label: "Gravity", type: "float", default: 1.2, min: -5, max: 5, step: 0.05, group: "Physics" },
    { key: "drag", label: "Drag", type: "float", default: 0.6, min: 0, max: 4, step: 0.05, group: "Physics" },

    { key: "colorCore", label: "Core Color", type: "color", default: "#FFE08C", group: "Appearance" },
    { key: "colorMid", label: "Mid Color", type: "color", default: "#FF7818", group: "Appearance" },
    { key: "colorOuter", label: "Outer Color", type: "color", default: "#FF3C00", group: "Appearance" },
    { key: "additive", label: "Additive Blend", type: "bool", default: true, group: "Appearance" },

    { key: "shieldFacingDeg", label: "Facing (°)", type: "float", default: 0, min: 0, max: 360, step: 1, group: "Shield", modes: ["shield"] },
    { key: "shieldAutoRotate", label: "Auto Rotate", type: "bool", default: true, group: "Shield", modes: ["shield"] },
    { key: "shieldRotateSpeedDeg", label: "Rotate Speed (°/s)", type: "float", default: 25, min: -180, max: 180, step: 1, group: "Shield", modes: ["shield"], showWhen: { key: "shieldAutoRotate", equals: true } },
    { key: "shieldArchWidthDeg", label: "Arch Width (°)", type: "float", default: 110, min: 20, max: 180, step: 1, group: "Shield", modes: ["shield"] },
    { key: "shieldArchHeightDeg", label: "Arch Height (°)", type: "float", default: 80, min: 20, max: 180, step: 1, group: "Shield", modes: ["shield"] },
    { key: "shieldHexSize", label: "Hex Size", type: "float", default: 0.16, min: 0.04, max: 0.4, step: 0.005, group: "Shield", modes: ["shield"] },
    { key: "shieldFlickerSpeed", label: "Flicker Speed", type: "float", default: 3.0, min: 0, max: 8, step: 0.05, group: "Shield", modes: ["shield"] },
    { key: "shieldImpactInterval", label: "Impact Interval", type: "float", default: 2.0, min: 0.3, max: 6, step: 0.1, group: "Shield", modes: ["shield"] },

    { key: "fieldHexSize", label: "Hex Size", type: "float", default: 0.18, min: 0.04, max: 0.4, step: 0.005, group: "Field", modes: ["field"] },
    { key: "fieldFlickerSpeed", label: "Flicker Speed", type: "float", default: 2.0, min: 0, max: 8, step: 0.05, group: "Field", modes: ["field"] },
    { key: "fieldPulseSpeed", label: "Pulse Speed", type: "float", default: 1.2, min: 0, max: 6, step: 0.05, group: "Field", modes: ["field"] },
    { key: "fieldPulseAmount", label: "Pulse Amount", type: "float", default: 0.04, min: 0, max: 0.3, step: 0.005, group: "Field", modes: ["field"] },
    { key: "fieldRotationSpeedDeg", label: "Rotation Speed (°/s)", type: "float", default: 12, min: -180, max: 180, step: 1, group: "Field", modes: ["field"] },

    { key: "orbCount", label: "Orb Count", type: "int", default: 4, min: 1, max: 8, step: 1, group: "Orbs", modes: ["fire_orbs", "wind_spin", "fire_wind"] },
    { key: "orbSize", label: "Orb Size", type: "float", default: 0.14, min: 0.03, max: 0.4, step: 0.005, group: "Orbs", modes: ["fire_orbs", "wind_spin", "fire_wind"] },
    { key: "orbitRadius", label: "Orbit Radius", type: "float", default: 1.1, min: 0.2, max: 4, step: 0.05, group: "Orbs", modes: ["fire_orbs", "wind_spin", "fire_wind"] },
    { key: "orbitSpeedDeg", label: "Orbit Speed (°/s)", type: "float", default: 140, min: -720, max: 720, step: 5, group: "Orbs", modes: ["fire_orbs", "wind_spin", "fire_wind"] },
    { key: "orbBobAmount", label: "Bob Amount", type: "float", default: 0.2, min: 0, max: 1, step: 0.01, group: "Orbs", modes: ["fire_orbs", "fire_wind"] },
    { key: "orbBobSpeed", label: "Bob Speed", type: "float", default: 2.2, min: 0, max: 8, step: 0.05, group: "Orbs", modes: ["fire_orbs", "fire_wind"] },
    { key: "trailFade", label: "Trail Fade", type: "float", default: 0.9, min: 0.1, max: 3, step: 0.05, group: "Orbs", modes: ["fire_orbs", "wind_spin", "fire_wind"] },

    { key: "windHelixHeight", label: "Helix Height", type: "float", default: 2.4, min: 0.5, max: 6, step: 0.05, group: "Wind", modes: ["wind_spin", "fire_wind"] },
    { key: "windHelixTurns", label: "Helix Turns", type: "float", default: 2.5, min: 0.5, max: 8, step: 0.1, group: "Wind", modes: ["wind_spin", "fire_wind"] },
    { key: "windRibbonWidth", label: "Ribbon Width", type: "float", default: 0.06, min: 0.01, max: 0.3, step: 0.005, group: "Wind", modes: ["wind_spin", "fire_wind"] },
    { key: "windFunnelLines", label: "Funnel Lines", type: "int", default: 6, min: 0, max: 10, step: 1, group: "Wind", modes: ["wind_spin", "fire_wind"] },

    { key: "waterRingCount", label: "Ring Count", type: "int", default: 4, min: 1, max: 5, step: 1, group: "Water Ring", modes: ["water_ring"] },
    { key: "waterRingDuration", label: "Ring Duration (s)", type: "float", default: 0.9, min: 0.1, max: 3, step: 0.05, group: "Water Ring", modes: ["water_ring"] },
    { key: "waterRingStagger", label: "Stagger Between Rings (s)", type: "float", default: 0.09, min: 0, max: 0.5, step: 0.01, group: "Water Ring", modes: ["water_ring"] },
    { key: "waterRingCrestHeight", label: "Crest Height", type: "float", default: 0.16, min: 0.02, max: 0.6, step: 0.01, group: "Water Ring", modes: ["water_ring"] },
    { key: "waterRingThickness", label: "Wall Thickness", type: "float", default: 0.16, min: 0.02, max: 0.6, step: 0.01, group: "Water Ring", modes: ["water_ring"] },

    { key: "earthBurstRockCount", label: "Rock Count", type: "int", default: 14, min: 0, max: 24, step: 1, group: "Earth Burst", modes: ["earth_burst"] },
    { key: "earthBurstCrackCount", label: "Crack Count", type: "int", default: 10, min: 0, max: 12, step: 1, group: "Earth Burst", modes: ["earth_burst"] },
    { key: "earthBurstDuration", label: "Burst Duration (s)", type: "float", default: 0.9, min: 0.1, max: 3, step: 0.05, group: "Earth Burst", modes: ["earth_burst"] },

    { key: "fireBurstEmberCount", label: "Ember Count", type: "int", default: 18, min: 0, max: 30, step: 1, group: "Fire Burst", modes: ["fire_burst"] },
    { key: "fireBurstDuration", label: "Burst Duration (s)", type: "float", default: 0.9, min: 0.1, max: 3, step: 0.05, group: "Fire Burst", modes: ["fire_burst"] },
    { key: "fireBurstColumnHeight", label: "Column Height", type: "float", default: 2.2, min: 0.2, max: 5, step: 0.1, group: "Fire Burst", modes: ["fire_burst"] },

    { key: "lightningBoltCount", label: "Bolt Count", type: "int", default: 5, min: 0, max: 8, step: 1, group: "Lightning Burst", modes: ["lightning_burst"] },
    { key: "lightningBoltLife", label: "Bolt Lifespan (s)", type: "float", default: 0.4, min: 0.05, max: 1.5, step: 0.01, group: "Lightning Burst", modes: ["lightning_burst"] },
    { key: "lightningJitter", label: "Zigzag Jitter", type: "float", default: 0.35, min: 0, max: 1, step: 0.01, group: "Lightning Burst", modes: ["lightning_burst"] },
    { key: "lightningHeight", label: "Strike Height", type: "float", default: 2.5, min: 0.5, max: 6, step: 0.1, group: "Lightning Burst", modes: ["lightning_burst"] },

    { key: "poisonRingCount", label: "Ring Count", type: "int", default: 3, min: 0, max: 3, step: 1, group: "Poison Burst", modes: ["poison_burst"] },
    { key: "poisonBubbleCount", label: "Bubble Count", type: "int", default: 20, min: 0, max: 30, step: 1, group: "Poison Burst", modes: ["poison_burst"] },
    { key: "poisonDuration", label: "Ring Duration (s)", type: "float", default: 1.1, min: 0.1, max: 3, step: 0.05, group: "Poison Burst", modes: ["poison_burst"] },

    { key: "healCrossCount", label: "Cross Count", type: "int", default: 8, min: 0, max: 16, step: 1, group: "Heal Aura", modes: ["heal_aura"] },
    { key: "healCycleDuration", label: "Orbit Cycle (s)", type: "float", default: 1.4, min: 0.2, max: 4, step: 0.05, group: "Heal Aura", modes: ["heal_aura"] },
    { key: "healPillarHeight", label: "Pillar Height", type: "float", default: 2.0, min: 0.2, max: 5, step: 0.1, group: "Heal Aura", modes: ["heal_aura"] },

    { key: "darkTentacleCount", label: "Tentacle Count", type: "int", default: 8, min: 0, max: 8, step: 1, group: "Dark Slash", modes: ["dark_slash"] },
    { key: "darkOrbCount", label: "Orb Count", type: "int", default: 10, min: 0, max: 16, step: 1, group: "Dark Slash", modes: ["dark_slash"] },
    { key: "darkDuration", label: "Cycle Duration (s)", type: "float", default: 0.7, min: 0.1, max: 2, step: 0.05, group: "Dark Slash", modes: ["dark_slash"] },

    { key: "barrierMoteCount", label: "Mote Count", type: "int", default: 14, min: 0, max: 20, step: 1, group: "Barrier", modes: ["barrier"] },
    { key: "barrierPulseSpeed", label: "Pulse Speed", type: "float", default: 1.2, min: 0.1, max: 5, step: 0.05, group: "Barrier", modes: ["barrier"] },

    { key: "fireballDuration", label: "Travel Duration (s)", type: "float", default: 0.9, min: 0.05, max: 3, step: 0.01, group: "Fireball", modes: ["fireball"] },
    { key: "fireballEmberCount", label: "Ember Count", type: "int", default: 12, min: 0, max: 16, step: 1, group: "Fireball", modes: ["fireball"] },

    { key: "windSlashDuration", label: "Travel Duration (s)", type: "float", default: 0.35, min: 0.02, max: 2, step: 0.01, group: "Wind Slash", modes: ["wind_slash"] },
    { key: "windSlashTrailCount", label: "Afterimage Trails", type: "int", default: 3, min: 0, max: 5, step: 1, group: "Wind Slash", modes: ["wind_slash"] },

    { key: "rockThrowDuration", label: "Travel Duration (s)", type: "float", default: 0.8, min: 0.05, max: 3, step: 0.01, group: "Rock Throw", modes: ["rock_throw"] },
    { key: "rockThrowDustCount", label: "Dust Count", type: "int", default: 10, min: 0, max: 16, step: 1, group: "Rock Throw", modes: ["rock_throw"] },

    { key: "lightningBoltDuration", label: "Flash Duration (s)", type: "float", default: 0.25, min: 0.02, max: 1.5, step: 0.01, group: "Lightning Bolt", modes: ["lightning_bolt"] },
    { key: "lightningBoltBranches", label: "Branch Count", type: "int", default: 3, min: 0, max: 4, step: 1, group: "Lightning Bolt", modes: ["lightning_bolt"] },

    { key: "waterJetDuration", label: "Travel Duration (s)", type: "float", default: 0.7, min: 0.05, max: 3, step: 0.01, group: "Water Jet", modes: ["water_jet"] },
    { key: "waterJetDropCount", label: "Splash Drop Count", type: "int", default: 14, min: 0, max: 20, step: 1, group: "Water Jet", modes: ["water_jet"] },

    { key: "iceShardDuration", label: "Travel Duration (s)", type: "float", default: 0.8, min: 0.05, max: 3, step: 0.01, group: "Ice Shard", modes: ["ice_shard"] },
    { key: "iceShardSparkCount", label: "Frost Spark Count", type: "int", default: 10, min: 0, max: 12, step: 1, group: "Ice Shard", modes: ["ice_shard"] },

    { key: "poisonOrbDuration", label: "Travel Duration (s)", type: "float", default: 1.0, min: 0.05, max: 3, step: 0.01, group: "Poison Orb", modes: ["poison_orb"] },
    { key: "poisonOrbSporeCount", label: "Spore Count", type: "int", default: 12, min: 0, max: 16, step: 1, group: "Poison Orb", modes: ["poison_orb"] },

    { key: "darkOrbDuration", label: "Travel Duration (s)", type: "float", default: 0.8, min: 0.05, max: 3, step: 0.01, group: "Dark Orb", modes: ["dark_orb"] },
    { key: "darkOrbTentacleCount", label: "Tentacle Count", type: "int", default: 3, min: 0, max: 4, step: 1, group: "Dark Orb", modes: ["dark_orb"] },

    { key: "lightArrowDuration", label: "Travel Duration (s)", type: "float", default: 0.35, min: 0.02, max: 2, step: 0.01, group: "Light Arrow", modes: ["light_arrow"] },

    { key: "bubbleBurstDuration", label: "Travel Duration (s)", type: "float", default: 0.9, min: 0.05, max: 3, step: 0.01, group: "Bubble Burst", modes: ["bubble_burst"] },
    { key: "bubbleBurstTrailCount", label: "Trail Bubble Count", type: "int", default: 10, min: 0, max: 16, step: 1, group: "Bubble Burst", modes: ["bubble_burst"] },

    ...comboLayerParams(2, { element: "water", shape: "wave" }),
    ...comboLayerParams(3, { element: "wind", shape: "spiral" }),
    ...comboLayerParams(4, { element: "lightning", shape: "beam" }),

    { key: "cameraDistance", label: "Camera Distance", type: "float", default: 3.5, min: 2, max: 15, step: 0.1, group: "Preview Camera" },
    { key: "cameraOrbitSpeed", label: "Camera Orbit Speed", type: "float", default: 3, min: -180, max: 180, step: 1, group: "Preview Camera" },
    { key: "showGrid", label: "Show Grid", type: "bool", default: true, group: "Preview Camera" },
  ],
};

// --- 2. Codegen ---------------------------------------------------------------

function buildExtraLayerLiteral(params: EffectParams, n: 2 | 3 | 4): string {
  const enabled = params[`layer${n}Enabled`] ? "1" : "0";
  const element = String(params[`layer${n}Element`]).toUpperCase();
  const shape = String(params[`layer${n}Shape`]).toUpperCase();
  const core = hexToRgbComment(String(params[`layer${n}ColorCore`]));
  const mid = hexToRgbComment(String(params[`layer${n}ColorMid`]));
  const outer = hexToRgbComment(String(params[`layer${n}ColorOuter`]));
  const count = fmtInt(params[`layer${n}ParticleCount`]);
  return `        { .enabled = ${enabled}, .element = EB_ELEM_${element}, .shape = EB_SHAPE_${shape}, .colorCore = (Color){ ${core}, 255 }, .colorMid = (Color){ ${mid}, 255 }, .colorOuter = (Color){ ${outer}, 255 }, .particleCount = ${count} }`;
}

function buildParamsBlock(params: EffectParams): string {
  const extraLayers = ([2, 3, 4] as const).map((n) => buildExtraLayerLiteral(params, n)).join(",\n");
  return `static EB_Params EB_g_params = {
    .element = EB_ELEM_${String(params.element).toUpperCase()},
    .shape = EB_SHAPE_${String(params.mode).toUpperCase()},
    .presetName = "${escapeCString(params.presetName)}",
    .particleCount = ${fmtInt(params.particleCount)},
    .spawnRadiusMin = ${fmtFloat(params.spawnRadiusMin)}, .spawnRadiusMax = ${fmtFloat(params.spawnRadiusMax)},
    .radius = ${fmtFloat(params.radius)},
    .directionYaw = ${fmtFloat(params.directionYaw)},
    .speedMin = ${fmtFloat(params.speedMin)}, .speedMax = ${fmtFloat(params.speedMax)},
    .lifeMin = ${fmtFloat(params.lifeMin)}, .lifeMax = ${fmtFloat(params.lifeMax)},
    .loopInterval = ${fmtFloat(params.loopInterval)},
    .gravity = ${fmtFloat(params.gravity)}, .drag = ${fmtFloat(params.drag)},
    .colorCore  = (Color){ ${hexToRgbComment(String(params.colorCore))}, 255 },
    .colorMid   = (Color){ ${hexToRgbComment(String(params.colorMid))}, 255 },
    .colorOuter = (Color){ ${hexToRgbComment(String(params.colorOuter))}, 255 },
    .additive = ${params.additive ? "1" : "0"},
    .shieldFacingDeg = ${fmtFloat(params.shieldFacingDeg)},
    .shieldAutoRotate = ${params.shieldAutoRotate ? "1" : "0"},
    .shieldRotateSpeedDeg = ${fmtFloat(params.shieldRotateSpeedDeg)},
    .shieldArchWidthDeg = ${fmtFloat(params.shieldArchWidthDeg)}, .shieldArchHeightDeg = ${fmtFloat(params.shieldArchHeightDeg)},
    .shieldHexSize = ${fmtFloat(params.shieldHexSize)},
    .shieldFlickerSpeed = ${fmtFloat(params.shieldFlickerSpeed)},
    .shieldImpactInterval = ${fmtFloat(params.shieldImpactInterval)},
    .fieldHexSize = ${fmtFloat(params.fieldHexSize)},
    .fieldFlickerSpeed = ${fmtFloat(params.fieldFlickerSpeed)},
    .fieldPulseSpeed = ${fmtFloat(params.fieldPulseSpeed)}, .fieldPulseAmount = ${fmtFloat(params.fieldPulseAmount)},
    .fieldRotationSpeedDeg = ${fmtFloat(params.fieldRotationSpeedDeg)},
    .orbCount = ${fmtInt(params.orbCount)},
    .orbSize = ${fmtFloat(params.orbSize)}, .orbitRadius = ${fmtFloat(params.orbitRadius)}, .orbitSpeedDeg = ${fmtFloat(params.orbitSpeedDeg)},
    .orbBobAmount = ${fmtFloat(params.orbBobAmount)}, .orbBobSpeed = ${fmtFloat(params.orbBobSpeed)},
    .trailFade = ${fmtFloat(params.trailFade)},
    .windHelixHeight = ${fmtFloat(params.windHelixHeight)}, .windHelixTurns = ${fmtFloat(params.windHelixTurns)}, .windRibbonWidth = ${fmtFloat(params.windRibbonWidth)},
    .windFunnelLines = ${fmtInt(params.windFunnelLines)},
    .waterRingCount = ${fmtInt(params.waterRingCount)},
    .waterRingDuration = ${fmtFloat(params.waterRingDuration)}, .waterRingStagger = ${fmtFloat(params.waterRingStagger)},
    .waterRingCrestHeight = ${fmtFloat(params.waterRingCrestHeight)}, .waterRingThickness = ${fmtFloat(params.waterRingThickness)},
    .earthBurstRockCount = ${fmtInt(params.earthBurstRockCount)}, .earthBurstCrackCount = ${fmtInt(params.earthBurstCrackCount)},
    .earthBurstDuration = ${fmtFloat(params.earthBurstDuration)},
    .fireBurstEmberCount = ${fmtInt(params.fireBurstEmberCount)},
    .fireBurstDuration = ${fmtFloat(params.fireBurstDuration)}, .fireBurstColumnHeight = ${fmtFloat(params.fireBurstColumnHeight)},
    .lightningBoltCount = ${fmtInt(params.lightningBoltCount)},
    .lightningBoltLife = ${fmtFloat(params.lightningBoltLife)}, .lightningJitter = ${fmtFloat(params.lightningJitter)}, .lightningHeight = ${fmtFloat(params.lightningHeight)},
    .poisonRingCount = ${fmtInt(params.poisonRingCount)}, .poisonBubbleCount = ${fmtInt(params.poisonBubbleCount)},
    .poisonDuration = ${fmtFloat(params.poisonDuration)},
    .healCrossCount = ${fmtInt(params.healCrossCount)},
    .healCycleDuration = ${fmtFloat(params.healCycleDuration)}, .healPillarHeight = ${fmtFloat(params.healPillarHeight)},
    .darkTentacleCount = ${fmtInt(params.darkTentacleCount)}, .darkOrbCount = ${fmtInt(params.darkOrbCount)},
    .darkDuration = ${fmtFloat(params.darkDuration)},
    .barrierMoteCount = ${fmtInt(params.barrierMoteCount)}, .barrierPulseSpeed = ${fmtFloat(params.barrierPulseSpeed)},
    .fireballDuration = ${fmtFloat(params.fireballDuration)}, .fireballEmberCount = ${fmtInt(params.fireballEmberCount)},
    .windSlashDuration = ${fmtFloat(params.windSlashDuration)}, .windSlashTrailCount = ${fmtInt(params.windSlashTrailCount)},
    .rockThrowDuration = ${fmtFloat(params.rockThrowDuration)}, .rockThrowDustCount = ${fmtInt(params.rockThrowDustCount)},
    .lightningBoltDuration = ${fmtFloat(params.lightningBoltDuration)}, .lightningBoltBranches = ${fmtInt(params.lightningBoltBranches)},
    .waterJetDuration = ${fmtFloat(params.waterJetDuration)}, .waterJetDropCount = ${fmtInt(params.waterJetDropCount)},
    .iceShardDuration = ${fmtFloat(params.iceShardDuration)}, .iceShardSparkCount = ${fmtInt(params.iceShardSparkCount)},
    .poisonOrbDuration = ${fmtFloat(params.poisonOrbDuration)}, .poisonOrbSporeCount = ${fmtInt(params.poisonOrbSporeCount)},
    .darkOrbDuration = ${fmtFloat(params.darkOrbDuration)}, .darkOrbTentacleCount = ${fmtInt(params.darkOrbTentacleCount)},
    .lightArrowDuration = ${fmtFloat(params.lightArrowDuration)},
    .bubbleBurstDuration = ${fmtFloat(params.bubbleBurstDuration)}, .bubbleBurstTrailCount = ${fmtInt(params.bubbleBurstTrailCount)},
    .cameraDistance = ${fmtFloat(params.cameraDistance)},
    .cameraOrbitSpeed = ${fmtFloat(params.cameraOrbitSpeed)},
    .showGrid = ${params.showGrid ? "1" : "0"},
    .extraLayers = {
${extraLayers}
    },
};
`;
}

const ELEMENT_ENUM_MAP: Record<string, string> = {
  neutral: "ELEM_NEUTRAL",
  fire: "ELEM_FIRE",
  water: "ELEM_WATER",
  earth: "ELEM_EARTH",
  wind: "ELEM_WIND",
  lightning: "ELEM_LIGHTNING",
  dark: "ELEM_DARK",
  poison: "ELEM_POISON",
  light: "ELEM_LIGHT",
  ice: "ELEM_ICE",
};

const SHAPE_ENUM_MAP: Record<string, string> = {
  sphere: "BURST_SHAPE_SPHERE",
  ring: "BURST_SHAPE_RING",
  spiral: "BURST_SHAPE_SPIRAL",
  beam: "BURST_SHAPE_BEAM",
  pillar: "BURST_SHAPE_PILLAR",
  rain: "BURST_SHAPE_RAIN",
  wave: "BURST_SHAPE_WAVE",
  projectile: "BURST_SHAPE_PROJECTILE",
  jump: "BURST_SHAPE_JUMP",
  shield: "BURST_SHAPE_SHIELD",
  field: "BURST_SHAPE_FIELD",
  fire_orbs: "BURST_SHAPE_FIRE_ORBS",
  wind_spin: "BURST_SHAPE_WIND_SPIN",
  fire_wind: "BURST_SHAPE_FIRE_WIND",
  water_ring: "BURST_SHAPE_WATER_RING",
  earth_burst: "BURST_SHAPE_EARTH_BURST",
  fire_burst: "BURST_SHAPE_FIRE_BURST",
  lightning_burst: "BURST_SHAPE_LIGHTNING_BURST",
  poison_burst: "BURST_SHAPE_POISON_BURST",
  heal_aura: "BURST_SHAPE_HEAL_AURA",
  dark_slash: "BURST_SHAPE_DARK_SLASH",
  barrier: "BURST_SHAPE_BARRIER",
  fireball: "BURST_SHAPE_FIREBALL",
  wind_slash: "BURST_SHAPE_WIND_SLASH",
  rock_throw: "BURST_SHAPE_ROCK_THROW",
  lightning_bolt: "BURST_SHAPE_LIGHTNING_BOLT",
  water_jet: "BURST_SHAPE_WATER_JET",
  ice_shard: "BURST_SHAPE_ICE_SHARD",
  poison_orb: "BURST_SHAPE_POISON_ORB",
  dark_orb: "BURST_SHAPE_DARK_ORB",
  light_arrow: "BURST_SHAPE_LIGHT_ARROW",
  bubble_burst: "BURST_SHAPE_BUBBLE_BURST",
};

function buildEnumaIchorPreset(params: EffectParams): string {
  const name = String(params.presetName || "VFX_");
  const elem = ELEMENT_ENUM_MAP[String(params.element)] ?? "ELEM_FIRE";
  const shape = SHAPE_ENUM_MAP[String(params.mode)] ?? "BURST_SHAPE_SPHERE";
  const yaw = Number(params.directionYaw) * (Math.PI / 180);
  const dirX = Math.sin(yaw), dirZ = Math.cos(yaw);
  const radius = params.mode === "sphere" ? params.spawnRadiusMax : params.radius;
  return `/* Generated by Bryncraft — Element Burst effect: "${name}"
   Paste this block into inc/effects_custom.h, inside CUSTOM_BURST_PRESETS[].
   Trigger it from any card's VFX_ field with: "VFX_CUSTOM_${name}" */
{
    .name = "${escapeCString(name)}",
    .element = ${elem},
    .shape = ${shape},
    .particleCount = ${fmtInt(params.particleCount)},
    .radius = ${fmtFloat(radius)},
    .direction = (Vector3){ ${fmtFloat(dirX)}, 0.000f, ${fmtFloat(dirZ)} },
    .speedMin = ${fmtFloat(params.speedMin)}, .speedMax = ${fmtFloat(params.speedMax)},
    .lifeMin = ${fmtFloat(params.lifeMin)}, .lifeMax = ${fmtFloat(params.lifeMax)},
    .gravity = ${fmtFloat(params.gravity)}, .drag = ${fmtFloat(params.drag)},
    .colorCore  = (Color){ ${hexToRgbComment(String(params.colorCore))}, 255 },
    .colorMid   = (Color){ ${hexToRgbComment(String(params.colorMid))}, 255 },
    .colorOuter = (Color){ ${hexToRgbComment(String(params.colorOuter))}, 255 },
    .additive = ${params.additive ? "true" : "false"},
},
`;
}

function buildExtras(params: EffectParams): ExtraAsset[] {
  const name = String(params.presetName || "VFX_").replace(/[^a-zA-Z0-9_]/g, "_");
  return [
    {
      filename: `${name}_enuma_ichor_preset.h`,
      label: `Enuma Ichor preset — paste into inc/effects_custom.h (triggers as VFX_CUSTOM_${name})`,
      kind: "text",
      content: buildEnumaIchorPreset(params),
    },
    { filename: "raylib.h", label: "raylib.h (compartido por las 4 demos standalone)", kind: "binary-url", url: raylibHeaderUrl },
    { filename: "rlgl.h", label: "rlgl.h (requerido por effect_atelier.h)", kind: "binary-url", url: rlglHeaderUrl },
    { filename: "libraylib.a", label: "libraylib.a — Windows (MinGW)", kind: "binary-url", url: libraylibWinUrl },
    { filename: "libraylib.a", label: "libraylib.a — Linux", kind: "binary-url", url: libraylibLnxUrl },
  ];
}

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main.c",
  readmeRaw,
  paramsRegex: /static EB_Params EB_g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: buildExtras,
};

// --- 3. Thumbnail --------------------------------------------------------------

const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  const cx = w / 2, cy = h / 2;
  for (let i = 0; i < 14; i++) {
    const a = (i / 14) * Math.PI * 2 + t * 1.5;
    const r = Math.min(w, h) * (0.12 + 0.16 * ((Math.sin(t * 3 + i) + 1) / 2));
    const x = cx + Math.cos(a) * r;
    const y = cy + Math.sin(a) * r * 0.6 - r * 0.3;
    const grad = ctx.createRadialGradient(x, y, 0, x, y, 4);
    grad.addColorStop(0, "rgba(255,224,140,0.9)");
    grad.addColorStop(1, "rgba(255,60,0,0)");
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, Math.PI * 2);
    ctx.fill();
  }
};

// --- Final package -----------------------------------------------------------

export const EFFECT_ATELIER_MODULE: EffectModule<"effect_atelier"> = { definition, codegen, thumbnail };
