#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requests
import redis

# this file should move to /Users/zhangxu/python3_env_tf/lib/python3.6/site-packages

redisClient = redis.StrictRedis(host='115.159.157.98', port=17379, db=0, password='redis12346')

def request(flow):
    # print("zx get url:" + flow.request.url)
    url='http://v'
    #筛选出以上面url为开头的url
    if flow.request.url.startswith(url):
        print("xxxxxxx get video url:" + flow.request.url)
        # create download task
        redisClient.lpush('douyinTask', str(flow.request.url))

