# サンプルの内容と表示形式はここで編集する。
set sampleXml {<?xml version='1.0' encoding='UTF-8'?>
<report title="野菜屋の仕入れ伝票">
  <box title="品名">トマト</box>
  <box title="仕入れ先">ひなた農園</box>
  <box title="仕入れ数量" mark="review" reason="週末の特売に合わせて、3箱から5箱に増やす。">3箱</box>
  <box title="規格">Mサイズ</box>
  <box title="入荷予定">金曜日の朝</box>
  <box title="受け取り場所">店舗裏口</box>
  <box title="保管場所">青果用冷蔵庫</box>
  <box title="用途">週末の特売</box>
</report>}

set reportXsl {<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" encoding="UTF-8"/>
  <xsl:template match="/report">
    <html lang="ja">
      <head><title><xsl:value-of select="@title"/></title></head>
      <body style="margin:22px; color:#20334a; background-color:#f5f7fa; font-family:sans-serif">
        <h1 style="font-size:24px"><xsl:value-of select="@title"/></h1>
        <xsl:apply-templates select="box"/>
      </body>
    </html>
  </xsl:template>
  <xsl:template match="box">
    <xsl:variable name="background">
      <xsl:choose>
        <xsl:when test="@mark='review'">#fff1c2</xsl:when>
        <xsl:otherwise>#ffffff</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <div class="box {@mark}" data-title="{@title}" data-reason="{@reason}" style="margin:0 0 12px; padding:12px; border:1px solid #bbc7cf; background-color:{$background}">
      <h2 style="margin:0; font-size:17px"><xsl:value-of select="@title"/></h2>
      <p style="margin:6px 0 0"><xsl:value-of select="."/></p>
    </div>
  </xsl:template>
</xsl:stylesheet>}

package require Tk 8.4
wm withdraw .
proc fail {message} {
    tk_messageBox -icon error -title "XMLビューア" -message $message
    exit 1
}
if {[catch {package require Tkhtml 3.0} reason]} {
    fail "HTML表示には Tkhtml 3 が必要。\n$reason"
}
if {[auto_execok xsltproc] eq ""} {fail "xsltproc が見つからない。PATH を確認すること。"}
# Tcl 8.4にはfile tempfileがないため、排他的に作成してパスを記録する。
proc temporaryInput {data pathsVar} {
    upvar 1 $pathsVar paths
    foreach key {TMPDIR TEMP TMP} {
        if {[info exists ::env($key)] && [file isdirectory $::env($key)]} {
            set dir $::env($key)
            break
        }
    }
    if {![info exists dir]} {set dir /tmp}
    for {set n 0} {$n < 100} {incr n} {
        set path [file join $dir xml-review-[pid]-$n.tmp]
        if {[catch {open $path {WRONLY CREAT EXCL} 0600} stream]} {
            if {[lindex $::errorCode 1] eq "EEXIST"} {continue}
            error $stream
        }
        lappend paths $path
        set failed [catch {
            fconfigure $stream -encoding utf-8 -translation lf
            puts -nonewline $stream $data
        } message]
        close $stream
        if {$failed} {error $message}
        return $path
    }
    error "一時ファイルを作成できない。"
}

proc render {filename sampleXml reportXsl} {
    set paths {}
    set failed [catch {
        set style [temporaryInput $reportXsl paths]
        if {$filename eq ""} {
            set filename [temporaryInput $sampleXml paths]
        }
        set command [list xsltproc --nonet --nowrite $style [file normalize $filename]]
        set stream [open [linsert $command 0 |] r]
        fconfigure $stream -encoding utf-8
        set readFailed [catch {read $stream} result]
        close $stream
        if {$readFailed} {error $result}
        set result
    } html]
    foreach path $paths {file delete -- $path}
    if {$failed} {error $html}
    return $html
}

switch [llength $argv] {
    0 {set filename ""; set documentName "内蔵サンプル"}
    1 {set filename [lindex $argv 0]; set documentName [file tail $filename]}
    default {fail "起動例: wish viewer.tcl report.xml"}
}
wm title . "XML レビュー"
wm geometry . 1080x720
wm minsize . 560 360
frame .header -relief groove -borderwidth 2 -padx 12 -pady 6
label .header.title -text "XML レビュー"
label .header.file -text $documentName
pack .header.title -side left
pack .header.file -side right
pack .header -side top -fill x
frame .body
pack .body -fill both -expand 1

frame .body.report -padx 4 -pady 4
place .body.report -relwidth 1 -relheight 0.8
html .body.report.doc -yscrollcommand {.body.report.bar set}
scrollbar .body.report.bar -command {.body.report.doc yview}
pack .body.report.bar -side right -fill y
pack .body.report.doc -fill both -expand 1
if {[catch {.body.report.doc parse -final [render $filename $sampleXml $reportXsl]} reason]} {
    fail "$documentName を表示できない。\n$reason"
}

foreach side {left right} caption {修正内容 コメント} x {0 0.5} {
    set foot .body.comment_$side
    frame $foot -padx 6 -pady 6
    place $foot -relx $x -rely 0.8 -relwidth 0.5 -relheight 0.2
    label $foot.title -text $caption
    pack $foot.title -anchor w -pady 2
    text $foot.input -wrap char -undo 1 -padx 8 -pady 6 \
        -height 1 -width 1 -yscrollcommand [list $foot.bar set]
    scrollbar $foot.bar -command [list $foot.input yview]
    pack $foot.bar -side right -fill y
    pack $foot.input -fill both -expand 1
}
set notes {}
foreach box [.body.report.doc search {div.review}] {
    set title [$box attribute data-title]
    set reason [$box attribute data-reason]
    if {$reason eq ""} {
        lappend notes $title
    } else {
        lappend notes "${title}：$reason"
    }
}
.body.comment_left.input insert 1.0 [join $notes "\n"]
wm deiconify .
