"""Encode each white run as a rectangle. Layout coordinates increase upward."""
import gdstk
import numpy as np


def write_mask(path, mask, pitch_nm):
    library = gdstk.Library(unit=1e-6, precision=1e-9)
    cell = library.new_cell("MASK")
    pitch = pitch_nm / 1000
    for row, pixels in enumerate(mask):
        edges = np.flatnonzero(np.diff(np.pad(pixels.astype(int), 1)))
        bottom = mask.shape[0] - 1 - row
        for left, right in edges.reshape(-1, 2):
            cell.add(gdstk.rectangle((left * pitch, bottom * pitch),
                                     (right * pitch, (bottom + 1) * pitch), layer=1))
    library.write_oas(str(path))


def read_mask(path, shape, pitch_nm):
    library = gdstk.read_oas(str(path), unit=1e-6)
    mask = np.zeros(shape, dtype=np.uint8)
    for polygon in library["MASK"].get_polygons(layer=1, datatype=0):
        bounds = np.rint(np.asarray(polygon.bounding_box()) * 1000 / pitch_nm).astype(int)
        (left, bottom), (right, top) = bounds
        mask[shape[0]-top:shape[0]-bottom, left:right] = 1
    return mask
