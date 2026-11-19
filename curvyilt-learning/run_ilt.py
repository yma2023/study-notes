import argparse
from pathlib import Path

import cv2
import numpy as np
import torch
from torch.nn import functional as F

from evaluation import metrics
from lithography import DEVICE
from mask_oasis import read_mask, write_mask
from optimizer import Solver, corner_retargeting_morph


def main():
    parser = argparse.ArgumentParser(description="Single-image Curvy ILT")
    parser.add_argument("image", type=Path)
    parser.add_argument("--pixel-nm", type=int, required=True)
    parser.add_argument("--iterations", type=int, default=200)
    parser.add_argument("--output", type=Path, default=Path("output"))
    args = parser.parse_args()
    image = cv2.imread(str(args.image), cv2.IMREAD_UNCHANGED)
    if (image is None or image.ndim != 2 or not np.isin(image, [0, 255]).all()
            or image.shape[0] != image.shape[1] or image.shape[0] % 8
            or image.shape[0] < 280):
        parser.error("Use a square binary grayscale PNG (0/255), side >=280 and divisible by 8.")
    if args.pixel_nm < 1 or args.iterations < 1:
        parser.error("Pitch and iterations must be positive integers.")
    solver = Solver(str(args.image), morph=3, scale_factor=8)
    target = corner_retargeting_morph(image.astype(np.float32) / 255, 39, 39)
    solver.target_s = F.avg_pool2d(torch.from_numpy(target)[None, None].to(DEVICE), 8)
    for _ in range(args.iterations):
        solver.optimize()

    args.output.mkdir(parents=True, exist_ok=True)
    model = solver.litho
    with torch.no_grad():
        model.mask.copy_(F.interpolate(model.avepool(model.mask_s), scale_factor=8,
                                       mode="bicubic", align_corners=False, antialias=True))
        mask = model.forward_test(use_morph=True)[0][0, 0].cpu().numpy() >= 0.5
        path = args.output / "mask.oas"
        write_mask(path, mask, args.pixel_nm)
        restored = read_mask(path, mask.shape, args.pixel_nm)
        if not np.array_equal(mask, restored):
            raise ValueError("OASIS round-trip changed mask pixels")
        model.mask.copy_(torch.from_numpy(restored).to(DEVICE))
        _, _, printed, outer, inner = model.forward_test(use_morph=False)
        pixels = printed[0, 0].cpu().numpy().astype(np.uint8) * 255
        if not cv2.imwrite(str(args.output / "printed.png"), pixels):
            raise OSError("Could not save printed.png")
        print(metrics(solver.target, printed, outer, inner))


if __name__ == "__main__":
    main()
