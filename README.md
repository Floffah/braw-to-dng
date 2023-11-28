# BRAW to DNG

As of Blackmagic's camera firmare 8.1, stills are no longer saved as DNG files, but as single-frame BRAW video files. This is a small script to convert them back to DNG files.

NOTE that you will very likely lose quality/data by using this - you should import your braw stills into Davinci Resolve and export individual frames there.

# Note if using this to edit in Lightroom

You shouldn't use this to edit in Lightroom. Instead, make sure you have the Blackmagic Raw SDK and Adobe Media Encoder installed on your computer. The BRAW SDK comes with an Adobe plugin for various Adobe products (excluding Lightroom unfortunately) like Media Encoder. You can check if it's installed by browsing to `/Library/Application Support/Adobe/Common/Plugins/7.0/Media Core` and checking if there's any Blackmagic Design related files there. If the installer didn't install it, create this folder and rerun the BRAW installer. Once done, import all of your BRAW files into Media Encoder, remove the H264 preset from your BRAW files, and add the `Tiff Sequence (Match Source)` preset. DNG is based on the Tiff format AND Lightroom supports it out of the box. You should use Tiff anyway instead of BRAW as CinemaDNG is becoming obsolete.

# Usage

```
braw-to-dng -i /some/input/directory -o /some/output/directory -f dng|png
```

This is heavily based on Blackmagic's ExtractFrame sample.
