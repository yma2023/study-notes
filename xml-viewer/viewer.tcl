package require Tk 8.6
wm withdraw .
proc fail {message} {
    tk_messageBox -icon error -title "XMLビューア" -message $message
    exit 1
}
if {[catch {package require Tkhtml 3.0} reason]} {
    fail "HTML表示には Tkhtml 3 が必要。\n$reason"
}
if {[auto_execok xsltproc] eq ""} {fail "xsltproc が見つからない。PATH を確認すること。"}
set base [file dirname [file normalize [info script]]]
switch [llength $argv] {
    0 {set filename [file join $base sample.xml]}
    1 {set filename [lindex $argv 0]}
    default {fail "起動例: wish viewer.tcl report.xml"}
}
proc render {filename stylesheet} {
    set command [list xsltproc --nonet --nowrite $stylesheet [file normalize $filename]]
    set stream [open [linsert $command 0 |] r]
    try {
        fconfigure $stream -encoding utf-8
        return [read $stream]
    } finally {
        close $stream
    }
}
wm title . "XML レビュー"
wm geometry . 1080x720
wm minsize . 560 360
ttk::style configure Comment.TLabel -font TkHeadingFont
ttk::frame .report -padding 4
place .report -relwidth 1 -relheight 0.8
html .report.doc -yscrollcommand {.report.bar set}
ttk::scrollbar .report.bar -command {.report.doc yview}
pack .report.bar -side right -fill y
pack .report.doc -fill both -expand 1
if {[catch {.report.doc parse -final [render $filename [file join $base report.xsl]]} reason]} {
    fail "[file tail $filename] を表示できない。\n$reason"
}

foreach side {left right} caption {修正内容 コメント} x {0 0.5} {
    set foot .comment_$side
    ttk::frame $foot -padding 6
    place $foot -relx $x -rely 0.8 -relwidth 0.5 -relheight 0.2
    ttk::label $foot.title -style Comment.TLabel -text $caption
    pack $foot.title -anchor w -pady {0 4}
    text $foot.input -wrap char -undo 1 -padx 8 -pady 6 -font TkTextFont \
        -height 1 -width 1 -yscrollcommand [list $foot.bar set]
    ttk::scrollbar $foot.bar -command [list $foot.input yview]
    pack $foot.bar -side right -fill y
    pack $foot.input -fill both -expand 1
}
set notes {}
foreach box [.report.doc search {div.review}] {
    set title [$box attribute data-title]
    set reason [$box attribute data-reason]
    if {$reason eq ""} {
        lappend notes $title
    } else {
        lappend notes "${title}：$reason"
    }
}
.comment_left.input insert 1.0 [join $notes "\n"]
wm deiconify .
