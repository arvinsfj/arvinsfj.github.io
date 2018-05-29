#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import string
import json
import md5
import httplib,urllib,urllib2

headers = {"Cookie": "appver=1.5.0.75771", "Referer": "http://music.163.com/", "Accept": "application/json"}

#type：搜索的类型 歌曲 1 专辑 10 歌手 100 歌单 1000 用户 1002 mv 1004 歌词 1006 主播电台 1009
def musicSearch(word, offset, limit):
    url = "http://music.163.com/api/search/pc"
    parameters = { "s":word, "type":1, "offset":offset, "limit":limit }
    data_encode = urllib.urlencode(parameters)
    req = urllib2.Request(url, data_encode, headers)
    response = urllib2.urlopen(req)
    res = response.read()
    return str(res)

def musicInfo(mid):
    url = "http://music.163.com/api/song/detail/"
    parameters = { "id": mid, "ids": "["+mid+"]" }
    data_encode = urllib.urlencode(parameters)
    url += "?"+data_encode
    response = urllib2.urlopen(url)
    res= response.read()
    print str(res)

def musicAlbums(aid, limit):
    url = "http://music.163.com/api/artist/albums/"+aid
    parameters = { "limit": limit }
    data_encode = urllib.urlencode(parameters)
    url += "?"+data_encode
    response = urllib2.urlopen(url)
    res= response.read()
    print str(res)

def musicPlaylist(aid):
    url = "http://music.163.com/api/playlist/detail"
    parameters = { "id": aid }
    data_encode = urllib.urlencode(parameters)
    url += "?"+data_encode
    response = urllib2.urlopen(url)
    res= response.read()
    print str(res)

def musicText(mid):
    url = "http://music.163.com/api/song/lyric"
    parameters = { "id": mid, "lv": -1, "kv": -1, "tv": -1 }
    data_encode = urllib.urlencode(parameters)
    url += "?"+data_encode
    response = urllib2.urlopen(url)
    res= response.read()
    print str(res)

def musicMV(mid):
    url = "http://music.163.com/api/mv/detail"
    parameters = { "id": mid, "type": "mp4" }
    data_encode = urllib.urlencode(parameters)
    url += "?"+data_encode
    response = urllib2.urlopen(url)
    res = response.read()
    print str(res)

def encrypted_id(id):  
    byte1 = bytearray('3go8&$8*3*3h0k(2)2')  
    byte2 = bytearray(id)  
    byte1_len = len(byte1)  
    for i in xrange(len(byte2)):  
        byte2[i] = byte2[i]^byte1[i%byte1_len]  
    m = md5.new()  
    m.update(byte2)
    result = m.digest().encode('base64')[:-1]
    result = result.replace('/', '_')  
    result = result.replace('+', '-')  
    return result

#print musicSearch("undo", 1, 0, 10)
#print musicSearch("刘若英", 1, 0, 10)
#print musicInfo("25949862")
#print musicAlbums("8326", 10)
#print musicPlaylist("37880978")
#print musicText("25949862")
#print musicMV("319104")
#print encrypted_id("2544269907079544")#6634453162708212,2544269907079544
#http://m2.music.126.net/E3Yr5zI6mhMFDbSmhkfoFQ==/2544269907079544.mp3

def musicSave(name, path, url):
    fpath = path + name + '.mp3'
    if os.path.exists(fpath):
        return
    print "Downloading", fpath, "..."
    try:
        resp = urllib2.urlopen(url, timeout = 60)
        data = resp.read()
        resp.close()
    except urllib2.URLError as e:
        print type(e)    #not catch
        pass
    except socket.timeout as e:
        print type(e)    #catched
        pass
    else:
        with open(fpath, 'wb') as mp3:
            mp3.write(data)


isSave = True
jsonStr = musicSearch("小苏菲", 0, 100)
data = json.loads(jsonStr)
songs = data["result"]["songs"]
for i in xrange(len(songs)):
    song = songs[i]
    name = song["name"]
    name = string.replace(name, '/', '_')
    name = string.replace(name, '?', '_')
    name = string.replace(name, '!', '_')
    url = None
    if song["hMusic"]:
        dfsId = song["hMusic"]["dfsId"]
        url = "http://m2.music.126.net/%s/%s.mp3" % (encrypted_id(str(dfsId)), dfsId)
    else:
        url = song["mp3Url"]

    if isSave:
        musicSave(name, "/Users/arvin/Desktop/Music/", url)
    else:
        print name ,"\n" ,url
