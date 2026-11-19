"""野菜屋の仕入れ伝票をXMLとXSLからHTMLに変換する。"""
import argparse
from pathlib import Path

from lxml import etree


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
        report = transform(etree.parse(str(args.xml), reader))
        args.output.write_bytes(bytes(report))
    except (OSError, etree.LxmlError) as error:
        parser.exit(1, f"HTMLを生成できない。{error}\n")


if __name__ == "__main__":
    main()
