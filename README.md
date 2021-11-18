# hitomi-browser
**NSFW**  
hitomi-browser is [hitomi](https://hitomi.la) viewer for linux.
This tool comes with some utilities.

## Requirement
[gawl](https://github.com/mojyack/gawl)  

## How to Use
Run hitomi-browser

## Utilities
List works that contain [female:big breasts" female:sister group]:
```
hitomi-search -t "female:big breasts" female:sister group
```
List Japanese original works:
```
hitomi-search -s original -l japanese
```
Download two works with id ["1825469" "1825467"]:
```
hitomi-download 1825469 1825467
```
Download all english doujinshi to "dl" folder:
```
hitomi-download.py -s dl $(hitomi-search -w doujinshi -l english)
```
