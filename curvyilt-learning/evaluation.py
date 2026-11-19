def metrics(target, nominal, outer, inner):
    return {
        "mismatched_pixels": int((target != nominal).sum().item()),
        "pvb_pixels": int(((outer == 1) & (inner == 0)).sum().item()),
    }
