"""野菜屋の仕入れ伝票をXMLとXSLからHTMLに変換する。"""
import argparse
from pathlib import Path

from lxml import etree


# この伝票の照合基準。キーはXMLのboxのtitleと対応する。
EXPECTED_VALUES = {"仕入れ数量": "5箱"}


def check_report(document):
    root = document.getroot()
    if root.tag != "report":
        raise ValueError("ルート要素には report を指定すること。")

    seen = set()
    for box in root.xpath("./box | ./review"):
        # 旧形式の伝票も、今回の照合結果で表示を決める。
        box.tag = "box"
        box.attrib.pop("mark", None)
        box.attrib.pop("reason", None)
        title = box.get("title")
        if title not in EXPECTED_VALUES:
            continue

        seen.add(title)
        actual = "".join(box.itertext()).strip()
        expected = EXPECTED_VALUES[title]
        if actual != expected:
            box.set("mark", "review")
            box.set("reason", f"伝票は「{actual or '未記入'}」、基準は「{expected}」。")

    for title, expected in EXPECTED_VALUES.items():
        if title not in seen:
            box = etree.SubElement(root, "box", title=title, mark="review")
            box.set("reason", f"項目が未記載。基準は「{expected}」。")
            box.text = "未記載"


def main():
    base = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", type=Path, nargs="?", default=base / "sample.xml")
    parser.add_argument("--xsl", type=Path, default=base / "report.xsl")
    parser.add_argument("-o", "--output", type=Path, default=base / "sample.html")
    args = parser.parse_args()
    if args.output.resolve() in (args.xml.resolve(), args.xsl.resolve()):
        parser.error("出力先には入力ファイルと異なるパスを指定すること。")
    try:
        reader = etree.XMLParser(resolve_entities=False, no_network=True)
        transform = etree.XSLT(etree.parse(str(args.xsl), reader),
                               access_control=etree.XSLTAccessControl.DENY_ALL)
        document = etree.parse(str(args.xml), reader)
        check_report(document)
        report = transform(document)
        args.output.write_bytes(bytes(report))
    except (OSError, ValueError, etree.LxmlError) as error:
        parser.exit(1, f"HTMLを生成できない。{error}\n")


if __name__ == "__main__":
    main()
