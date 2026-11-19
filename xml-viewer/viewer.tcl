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
frame .header -background #17324d -padx 18 -pady 9
label .header.title -text "XML レビュー" -font TkHeadingFont -foreground white -background #17324d
label .header.file -text [file tail $filename] -foreground #dbebf5 -background #17324d
pack .header.title -side left
pack .header.file -side right
pack .header -side top -fill x
frame .body
pack .body -fill both -expand 1

ttk::frame .body.report -padding 4
place .body.report -relwidth 1 -relheight 0.8
html .body.report.doc -yscrollcommand {.body.report.bar set}
ttk::scrollbar .body.report.bar -command {.body.report.doc yview}
pack .body.report.bar -side right -fill y
pack .body.report.doc -fill both -expand 1
if {[catch {.body.report.doc parse -final [render $filename [file join $base report.xsl]]} reason]} {
    fail "[file tail $filename] を表示できない。\n$reason"
}

foreach side {left right} caption {修正内容 コメント} x {0 0.5} {
    set foot .body.comment_$side
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
