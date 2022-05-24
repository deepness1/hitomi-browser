# hitomi-browser
**NSFW**  
hitomi-browser is an unofficial [hitomi.la](https://hitomi.la) viewer for Linux.  
This tool comes with some utilities.

## Requirement
[gawl](https://github.com/mojyack/gawl)  

## How to Use
Run hitomi-browser

## Utilities
### hitomi-search
Search for galleries that contain all elements.  
The first character of each argument stands for a category.  
Available categories are:  
```
'a': artist  
'g': group  
's': series  
'c': character  
'w': work type ["doujinshi", "artistcg", "gamecg", "manga"]  
't': tag  
'l': language  
'k': keyword  
```
#### Example
List works that are tagged [female:big breasts" female:sister group]:
```
hitomi-search "tfemale:big breasts" tfemale:sister tgroup
```
List japanese original works:
```
hitomi-search soriginal ljapanese
```
### hitomi-download
Download galleries with a given id.
#### Example
Download two works with id ["1825469" "1825467"]:
```
hitomi-download 1825469 1825467
```
Download all english doujinshi to "dl" folder:
```
hitomi-download -s dl $(hitomi-search wdoujinshi lenglish)
```
