#!/bin/sh
# Firmware 6 launcher: resume the last book, capture logs and handle restarts.
cd /mnt/ext1 || exit 1
last_book_file=/mnt/ext1/system/share/cr3/last-book.txt
if [ "$#" -eq 0 ] || [ -z "$1" ]; then
    last_book=
    if [ -f "$last_book_file" ]; then
        IFS= read -r last_book < "$last_book_file"
    fi
    if [ -n "$last_book" ] && [ -f "$last_book" ]; then
        set -- "$last_book"
    else
        set -- /mnt/ext1/system/share/cr3/Welcome.fb2
    fi
fi
if [ -f "$1" ]; then
    # A failed write must not truncate the last complete remembered path.
    last_book_temp=$(mktemp "${last_book_file}.tmp.XXXXXX")
    if [ -n "$last_book_temp" ]; then
        if printf '%s\n' "$1" > "$last_book_temp"; then
            mv -f "$last_book_temp" "$last_book_file"
        fi
        rm -f "$last_book_temp"
    fi
fi
while :; do
    /mnt/ext1/system/share/cr3/bin/cr3-pb.app "$@" > /mnt/ext1/system/cr3-pb.normal.log 2> /mnt/ext1/system/cr3-pb.crash.log
    status=$?
    [ -f /mnt/ext1/.cr3restart ] || exit "$status"
    rm -f /mnt/ext1/.cr3restart
done
