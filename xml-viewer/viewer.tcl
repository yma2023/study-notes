package require Tk 8.4
wm withdraw .
proc fail {message} {
    tk_messageBox -icon error -title "XMLビューア" -message $message
    exit 1
}
if {[catch {package require Tkhtml 3.0} reason]} {
    fail "HTML表示には Tkhtml 3 が必要。\n$reason"
}
switch [llength $argv] {
    0 {set filename [file join [file dirname [info script]] sample.html]}
    1 {set filename [lindex $argv 0]}
    default {fail "起動例: wish viewer.tcl sample.html"}
}
set documentName [file tail $filename]
if {[catch {
    set stream [open [file normalize $filename] r]
    fconfigure $stream -encoding utf-8
    set failed [catch {read $stream} page]
    close $stream
    if {$failed} {error $page}
} reason]} {fail "$documentName を読み込めない。\n$reason"}
wm title . "伝票レビュー"
wm geometry . 1080x720
wm minsize . 560 360
frame .header -relief groove -borderwidth 2 -padx 12 -pady 6
label .header.title -text "伝票レビュー"
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
if {[catch {.body.report.doc parse -final $page} reason]} {
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
    set title [$box attribute -default "" data-title]
    set reason [$box attribute -default "" data-reason]
    if {$reason eq ""} {
        lappend notes $title
    } else {
        lappend notes "${title}：$reason"
    }
}
.body.comment_left.input insert 1.0 [join $notes "\n"]
wm deiconify .
