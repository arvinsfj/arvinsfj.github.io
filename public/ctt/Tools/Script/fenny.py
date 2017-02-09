#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import datetime
import time
from datetime import timedelta

def sub(date1,date2):
    date1=time.strptime(date1,"%Y-%m-%d")
    date2=time.strptime(date2,"%Y-%m-%d")
    date1=datetime.datetime(date1[0],date1[1],date1[2])
    date2=datetime.datetime(date2[0],date2[1],date2[2])
    dayOfWeek = date2.weekday() #0-6 星期一到星期日
    date2 = date2+timedelta(days=5-dayOfWeek)
    return (date2-date1).days

if len(sys.argv) > 2:
    print ""
    print "Usage: \n./calcworkday.py\n./calcworkday.py <date>"
    print ""
    quit()

date1 = "2017-2-4"    #先前单休的周六日期
date2 = time.strftime("%Y-%m-%d")
if len(sys.argv) == 2:
    date2 = sys.argv[1]
days = sub(date1, date2)
if days < 0:
	days = -days
if (days/7)%2 == 0:
	print "\nFenny 单休\n"
else:
	print "\nFenny 双休\n"

