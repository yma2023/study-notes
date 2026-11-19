# Curvy ILT 学習用・単体版

1枚の白黒PNGからマスクを最適化し、OASISと標準条件の予測レジスト像を生成する。

```sh
python -m pip install -r requirements.txt
python run_ilt.py examples/target.png --pixel-nm 1
```

入力は0/255のグレースケール正方形。1辺は280画素以上、8の倍数。白はパターン。付属画像は自作。CPUまたはCUDAを自動選択する。初回の疎通確認は `--iterations 1`、通常は200反復。

1. run_ilt.py が目標PNGを読み、角を調整した縮小目標を準備する。
2. optimizer.py が高露光量像と目標との二乗誤差、条件間の二乗誤差、周波数罰則を合計しSGDで仮マスクを更新する。
3. lithography.py がTCCカーネル24個を使い、各条件の光強度とレジスト像を計算する。
4. mask_oasis.py が二値マスクを横方向の矩形に変換し、output/mask.oas のMASKセル、層1/0に書く。矩形を読み戻し、全画素の一致を検証する。座標原点は画像の左下。画素境界に沿う階段形状を保持する。
5. 読み戻したマスクで再露光し、output/printed.png を保存する。evaluation.py が元の目標との不一致画素数とPVB画素数を表示する。

--pixel-nm はOASIS上の寸法換算（整数nm）。値を変えても付属カーネルの物理条件は再校正されない。

原典 [NVlabs/curvyILT](https://github.com/NVlabs/curvyILT) の単体版を再構成した非公式教材。光学カーネルも同リポジトリに由来する。Yang and Ren, “GPU-Accelerated Inverse Lithography Towards High Quality Curvy Mask Generation”, ISPD 2025。利用条件は同梱のNVIDIA LICENSE、研究・評価用途限定。
