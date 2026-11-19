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
    0 {set files [list [file join $base left.xml] [file join $base right.xml]]}
    2 {set files $argv}
    default {fail "起動例: wish viewer.tcl left.xml right.xml"}
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
foreach side {left right} caption {左 右} x {0 0.5} filename $files {
    set top .$side
    ttk::frame $top -padding 4
    place $top -relx $x -rely 0 -relwidth 0.5 -relheight 0.8
    html $top.doc -yscrollcommand [list $top.bar set]
    ttk::scrollbar $top.bar -command [list $top.doc yview]
    pack $top.bar -side right -fill y
    pack $top.doc -fill both -expand 1
    if {[catch {$top.doc parse -final [render $filename [file join $base report.xsl]]} reason]} {
        fail "[file tail $filename] を表示できない。\n$reason"
    }
    set foot .comment_$side
    ttk::frame $foot -padding 6
    place $foot -relx $x -rely 0.8 -relwidth 0.5 -relheight 0.2
    ttk::label $foot.title -style Comment.TLabel -text "$caption のコメント"
    pack $foot.title -anchor w -pady {0 4}
    text $foot.input -wrap char -undo 1 -padx 8 -pady 6 -font TkTextFont \
        -height 1 -width 1 -yscrollcommand [list $foot.bar set]
    ttk::scrollbar $foot.bar -command [list $foot.input yview]
    pack $foot.bar -side right -fill y
    pack $foot.input -fill both -expand 1
}
wm deiconify .
