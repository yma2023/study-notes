# 野菜屋の仕入れ伝票レビュー

![上段に仕入れ伝票を表示し、黄色の仕入れ数量に対応する説明を左下に表示した画面](classic-tk-review.png)

## 生成済みのサンプルを開く

このフォルダーで次のコマンドを実行する。表示に必要なのは Tcl/Tk と、その環境用の Tkhtml 3。GUIはTcl/Tk 8.4を互換目標とし、8.6で動作確認した。

```sh
wish viewer.tcl                 # 同梱の sample.html を表示
wish viewer.tcl order.html      # 指定したUTF-8のHTMLを表示
```

ヘッダーの下は全幅の伝票で、下段だけを左右に分割する。左下に修正対象の項目名と理由を表示し、右下は空欄で開く。コメントの編集は終了時に破棄される。

## XMLからHTMLを生成する

入力例は `sample.xml`、表示ルールは `report.xsl`、生成結果は `sample.html`。XMLやXSLを編集したら、Pythonで再生成してからGUIで開き直す。

```sh
python3 -m pip install lxml
python3 generate_html.py
wish viewer.tcl
```

ファイルを指定する場合は次のように実行する。

```sh
python3 generate_html.py order.xml --xsl report.xsl -o order.html
wish viewer.tcl order.html
```

## 黄色にするタグ

`report` 直下の通常項目は `box`、修正対象は `review` で囲む。XSLが修正対象の背景を黄色にし、項目名と理由をHTMLに埋め込む。複数の修正対象は左下に伝票順で並ぶ。従来の `<box mark="review">` も利用できる。

```xml
<report title="野菜屋の仕入れ伝票">
  <box title="品名">トマト</box>
  <review title="仕入れ数量" reason="週末の特売に合わせて、3箱から5箱に増やす。">3箱</review>
</report>
```
