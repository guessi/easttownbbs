#!/bin/sh
# 抓取中央氣象局氣象報導資料

#天氣概況		W001.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W001.txt" > /home/bbs/gem/@/@001
#今日白天天氣預報	W002.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W002.txt" > /home/bbs/gem/@/@002
#明日白天天氣預報	W003.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W003.txt" > /home/bbs/gem/@/@003
#近海漁業(甲)		W004.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W004.txt" > /home/bbs/gem/@/@004
#近海漁業(乙)		W005.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W005.txt" > /home/bbs/gem/@/@005
#三天漁業(甲)		W006.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W006.txt" > /home/bbs/gem/@/@006
#三天漁業(乙)		W007.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W007.txt" > /home/bbs/gem/@/@007
#三天漁業(丙)		W008.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W008.txt" > /home/bbs/gem/@/@008
#全球都市天氣預報	W010.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W010.txt" > /home/bbs/gem/@/@010
#一週天氣預報		W011.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W011.txt" > /home/bbs/gem/@/@011
#一週旅遊天氣預報	W012.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W012.txt" > /home/bbs/gem/@/@012
#口語化天氣概況		W016.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W016.txt" > /home/bbs/gem/@/@016
#大陸主要城市預報	W018.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W018.txt" > /home/bbs/gem/@/@018
#舒適度指數預報		W020.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W020.txt" > /home/bbs/gem/@/@020
#今夜至明日清晨天氣預報	W042.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W042.txt" > /home/bbs/gem/@/@042
#明夜至後天清晨天氣預報	W043.txt
/usr/local/bin/lynx -source "ftp://ftpsv.cwb.gov.tw/pub/forecast/W043.txt" > /home/bbs/gem/@/@043
