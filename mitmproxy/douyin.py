#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requests
import redis
import json

# this file should move to /Users/zhangxu/python3_env_tf/lib/python3.6/site-packages

redisClient = redis.StrictRedis(host='115.159.157.98', port=17379, db=0, password='redis12346')

def request(flow):
    # print("zx get url:" + flow.request.url)
    url='http://v'
    #筛选出以上面url为开头的url
    if flow.request.url.startswith(url):
        print("xxxxxxx get video url:" + flow.request.url)
        # create download task
        # redisClient.lpush('douyinTask', str(flow.request.url))


def response(flow):
    url = 'https://api.amemv.com/aweme/v1/feed'
    if flow.request.url.startswith(url):
        print("xxxxxxx get response url:" + flow.request.url)
        data = flow.response.content
        # print(data)
        try:
            dataJson = json.loads(data)

            # parse json
            aweme_list = dataJson['aweme_list']
            for aweme in aweme_list:
                print(aweme['aweme_id'])
                print(aweme['author_user_id'])
                print(aweme['desc'])
                print(aweme['author']['nickname'])
                print(aweme['statistics']['digg_count'])
                
                videoUrl = aweme['video']['bit_rate'][0]['play_addr']['url_list'][0]

                if aweme['statistics']['digg_count'] > 10000:
                    print("digg_count > 1000 to download.")
                    redisClient.lpush('douyinTask', videoUrl)

            # redisClient.lpush('douyinVideosList', json.dumps(dataJson))

        except Exception as e:
            print(e)

        