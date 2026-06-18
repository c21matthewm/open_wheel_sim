#!/usr/bin/env python3
"""Generate deterministic PBR-style texture maps for the simulator."""

from __future__ import annotations

import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
TEXTURE_DIR = ROOT / "assets" / "textures"


def value_noise(rng: np.random.Generator, size: int, grid: int) -> np.ndarray:
    coarse = rng.random((grid, grid), dtype=np.float32)
    image = Image.fromarray(np.uint8(coarse * 255), mode="L")
    image = image.resize((size, size), Image.Resampling.BICUBIC)
    return np.asarray(image, dtype=np.float32) / 255.0


def fbm(rng: np.random.Generator, size: int, octaves: tuple[tuple[int, float], ...]) -> np.ndarray:
    result = np.zeros((size, size), dtype=np.float32)
    amplitude_total = 0.0
    for grid, amplitude in octaves:
        result += value_noise(rng, size, grid) * amplitude
        amplitude_total += amplitude
    return np.clip(result / max(amplitude_total, 0.001), 0.0, 1.0)


def normal_from_height(height: np.ndarray, strength: float) -> np.ndarray:
    dy, dx = np.gradient(height.astype(np.float32))
    nx = -dx * strength
    ny = -dy * strength
    nz = np.ones_like(height, dtype=np.float32)
    length = np.sqrt(nx * nx + ny * ny + nz * nz)
    normal = np.stack((nx / length, ny / length, nz / length), axis=-1)
    return np.uint8(np.clip(normal * 0.5 + 0.5, 0.0, 1.0) * 255)


def save_rgb(path: Path, rgb: np.ndarray, quality: int = 92) -> None:
    image = Image.fromarray(np.uint8(np.clip(rgb, 0.0, 1.0) * 255), mode="RGB")
    image.save(path, "WEBP", quality=quality, method=6)


def save_luma(path: Path, value: np.ndarray, quality: int = 92) -> None:
    luma = np.uint8(np.clip(value, 0.0, 1.0) * 255)
    rgb = np.repeat(luma[..., None], 3, axis=2)
    save_rgb(path, rgb, quality)


def asphalt_maps(rng: np.random.Generator) -> None:
    size = 4096
    large = fbm(rng, size, ((5, 0.46), (13, 0.30), (41, 0.16), (119, 0.08)))
    aggregate = fbm(rng, size, ((53, 0.26), (181, 0.36), (487, 0.38)))
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    lane = 0.5 + 0.5 * np.sin(xx * 0.0052 + yy * 0.0011)
    scratches = np.maximum(
        0.0,
        np.sin((xx * 0.025 + yy * 0.0015) + fbm(rng, size, ((17, 1.0),)) * 2.2),
    )
    cracks = np.clip((fbm(rng, size, ((9, 0.40), (25, 0.34), (81, 0.26))) - 0.62) * 5.0, 0.0, 1.0)
    height = np.clip(aggregate * 0.74 + large * 0.20 + scratches * 0.035 - cracks * 0.22, 0.0, 1.0)
    albedo = np.stack(
        (
            0.090 + large * 0.040 + aggregate * 0.050,
            0.092 + large * 0.042 + aggregate * 0.047,
            0.096 + large * 0.046 + aggregate * 0.044,
        ),
        axis=-1,
    )
    albedo *= 0.82 + lane[..., None] * 0.12
    albedo *= 1.0 - cracks[..., None] * 0.36
    roughness = np.clip(0.78 + aggregate * 0.16 - lane * 0.06 + cracks * 0.10, 0.48, 0.96)
    save_rgb(TEXTURE_DIR / "asphalt_albedo.webp", albedo)
    save_rgb(TEXTURE_DIR / "asphalt_normal.webp", normal_from_height(height, 22.0) / 255.0, 94)
    save_luma(TEXTURE_DIR / "asphalt_roughness.webp", roughness)
    save_luma(TEXTURE_DIR / "asphalt_height.webp", height)


def grass_maps(rng: np.random.Generator) -> None:
    size = 4096
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    base_noise = fbm(rng, size, ((7, 0.44), (19, 0.32), (67, 0.18), (193, 0.06)))
    blades_a = 0.5 + 0.5 * np.sin(xx * 0.085 + yy * 0.022 + base_noise * 4.0)
    blades_b = 0.5 + 0.5 * np.sin(xx * -0.030 + yy * 0.096 + base_noise * 3.2)
    mowing = 0.5 + 0.5 * np.sin((xx - yy * 0.23) * 0.008)
    clumps = np.clip((base_noise - 0.40) * 1.7, 0.0, 1.0)
    height = np.clip(base_noise * 0.48 + blades_a * 0.22 + blades_b * 0.18 + clumps * 0.18, 0.0, 1.0)
    color_a = np.array([0.050, 0.205, 0.070], dtype=np.float32)
    color_b = np.array([0.135, 0.360, 0.112], dtype=np.float32)
    color_c = np.array([0.245, 0.345, 0.120], dtype=np.float32)
    mix_ab = np.clip(base_noise * 0.85 + mowing * 0.25, 0.0, 1.0)
    albedo = color_a * (1.0 - mix_ab[..., None]) + color_b * mix_ab[..., None]
    albedo = albedo * (0.84 + blades_a[..., None] * 0.20 + blades_b[..., None] * 0.10)
    albedo = albedo * (1.0 - clumps[..., None] * 0.20) + color_c * (mowing[..., None] * 0.10)
    roughness = np.clip(0.86 + clumps * 0.08 + blades_a * 0.04, 0.74, 0.98)
    save_rgb(TEXTURE_DIR / "grass_albedo.webp", albedo)
    save_rgb(TEXTURE_DIR / "grass_normal.webp", normal_from_height(height, 18.0) / 255.0, 94)
    save_luma(TEXTURE_DIR / "grass_roughness.webp", roughness)
    save_luma(TEXTURE_DIR / "grass_height.webp", height)


def concrete_maps(rng: np.random.Generator) -> None:
    size = 2048
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    pores = fbm(rng, size, ((7, 0.42), (31, 0.32), (113, 0.20), (391, 0.06)))
    seams = np.maximum(
        np.exp(-((np.mod(xx, 340.0) - 170.0) ** 2) / 90.0),
        np.exp(-((np.mod(yy, 210.0) - 105.0) ** 2) / 85.0),
    )
    stains = np.clip((fbm(rng, size, ((11, 0.55), (47, 0.45))) - 0.53) * 2.1, 0.0, 1.0)
    height = np.clip(pores * 0.78 - seams * 0.26 + stains * 0.07, 0.0, 1.0)
    albedo = np.stack(
        (
            0.56 + pores * 0.14 - stains * 0.08,
            0.55 + pores * 0.13 - stains * 0.08,
            0.51 + pores * 0.11 - stains * 0.07,
        ),
        axis=-1,
    )
    albedo *= 1.0 - seams[..., None] * 0.12
    roughness = np.clip(0.74 + pores * 0.18 + stains * 0.06, 0.60, 0.94)
    save_rgb(TEXTURE_DIR / "concrete_albedo.webp", albedo)
    save_rgb(TEXTURE_DIR / "concrete_normal.webp", normal_from_height(height, 12.0) / 255.0)
    save_luma(TEXTURE_DIR / "concrete_roughness.webp", roughness)
    save_luma(TEXTURE_DIR / "concrete_height.webp", height)


def carbon_maps(rng: np.random.Generator) -> None:
    size = 2048
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    weave_a = 0.5 + 0.5 * np.sin((xx + yy * 0.20) * 0.105)
    weave_b = 0.5 + 0.5 * np.sin((yy - xx * 0.18) * 0.112)
    thread = np.maximum(weave_a, weave_b)
    micro = fbm(rng, size, ((19, 0.35), (89, 0.35), (233, 0.30)))
    height = np.clip(thread * 0.72 + micro * 0.20, 0.0, 1.0)
    albedo = np.stack(
        (
            0.018 + thread * 0.030 + micro * 0.010,
            0.020 + thread * 0.034 + micro * 0.011,
            0.024 + thread * 0.040 + micro * 0.012,
        ),
        axis=-1,
    )
    roughness = np.clip(0.22 + (1.0 - thread) * 0.18 + micro * 0.08, 0.18, 0.54)
    save_rgb(TEXTURE_DIR / "carbon_albedo.webp", albedo, 94)
    save_rgb(TEXTURE_DIR / "carbon_normal.webp", normal_from_height(height, 28.0) / 255.0, 94)
    save_luma(TEXTURE_DIR / "carbon_roughness.webp", roughness, 94)


def skybox(rng: np.random.Generator) -> None:
    width = 4096
    height = 2048
    y, x = np.mgrid[0:height, 0:width].astype(np.float32)
    u = x / float(width)
    v = y / float(height)
    theta = u * math.tau
    phi = v * math.pi
    ray_x = np.sin(phi) * np.cos(theta)
    ray_y = np.cos(phi)
    ray_z = np.sin(phi) * np.sin(theta)
    up = np.clip(ray_y * 0.5 + 0.5, 0.0, 1.0)
    horizon = (1.0 - up) ** 2.2
    sky_low = np.array([0.70, 0.78, 0.82], dtype=np.float32)
    sky_high = np.array([0.055, 0.19, 0.46], dtype=np.float32)
    ground = np.array([0.042, 0.080, 0.050], dtype=np.float32)
    color = ground * (1.0 - up[..., None]) + (sky_low * (1.0 - up[..., None]) + sky_high * up[..., None])
    color += np.array([1.00, 0.55, 0.22], dtype=np.float32) * horizon[..., None] * 0.38

    sun = np.array([-0.42, 0.82, -0.38], dtype=np.float32)
    sun /= np.linalg.norm(sun)
    sun_dot = np.clip(ray_x * sun[0] + ray_y * sun[1] + ray_z * sun[2], 0.0, 1.0)
    color += np.array([5.5, 3.3, 1.05], dtype=np.float32) * (sun_dot**360)[..., None]
    color += np.array([1.15, 0.74, 0.38], dtype=np.float32) * (sun_dot**18)[..., None] * 0.42

    cloud_noise = fbm(rng, width, ((6, 0.44), (19, 0.31), (61, 0.18), (173, 0.07)))
    cloud_noise = np.asarray(
        Image.fromarray(np.uint8(cloud_noise * 255), mode="L")
        .resize((width, height), Image.Resampling.BICUBIC),
        dtype=np.float32,
    ) / 255.0
    cloud_band = np.clip((ray_y - 0.05) / 0.60, 0.0, 1.0) * np.clip((0.88 - ray_y) / 0.26, 0.0, 1.0)
    cloud = np.clip((cloud_noise - 0.58) * 3.2, 0.0, 1.0) * cloud_band
    cloud = np.asarray(
        Image.fromarray(np.uint8(cloud * 255), mode="L").filter(ImageFilter.GaussianBlur(radius=2.4)),
        dtype=np.float32,
    ) / 255.0
    color = color * (1.0 - cloud[..., None] * 0.30) + np.array([0.92, 0.94, 0.90], dtype=np.float32) * cloud[..., None]
    color = np.clip(color / (1.0 + color * 0.18), 0.0, 1.0)
    save_rgb(TEXTURE_DIR / "skybox_ims_evening.webp", color, 94)


def main() -> None:
    TEXTURE_DIR.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(20260612)
    asphalt_maps(rng)
    grass_maps(rng)
    concrete_maps(rng)
    carbon_maps(rng)
    skybox(rng)
    print(f"Generated PBR textures in {TEXTURE_DIR}")


if __name__ == "__main__":
    main()
