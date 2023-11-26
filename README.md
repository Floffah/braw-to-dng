# BRAW to DNG

As of Blackmagic's camera firmare 8.1, stills are no longer saved as DNG files, but as single-frame BRAW video files. This is a small script to convert them back to DNG files.

NOTE that you will very likely lose quality/data by using this - you should import your braw stills into Davinci Resolve and export individual frames there.

Usage:
```
braw-to-dng -i /some/input/directory -o /some/output/directory -f dng|png
```

This is heavily based on Blackmagic's ExtractFrame sample.
