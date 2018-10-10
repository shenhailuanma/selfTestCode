#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requests
import redis
import time
import hashlib
import os
import owncloud


class CopyVideo(object):
    """docstring for CopyVideo"""
    def __init__(self):
        super(CopyVideo, self).__init__()
        # self.arg = arg

        self.uploadCount = 0

        # redis connect
        self.redisClient = redis.StrictRedis(host='115.159.157.98', port=17379, db=0, password='redis12346')

        # nextcloud
        # self.oc = owncloud.Client('http://127.0.0.1:18080')
        # self.oc.login('zhangxu', 'zx@12346')
        # self.oc.logout()

        self.oc2 = owncloud.Client('http://127.0.0.1:18181')
        self.oc2.login('zhangxu', 'zx@12346')
        self.oc2.logout()


    def getTask(self):
        # 从任务队列中取任务
        task = self.redisClient.rpop("copyTask")
        return task

    def run(self):
        while True:
            try:
                task = self.getTask()
                if task != None:
                    print("get one task, to copy")
                    print(task)

            except Exception as e:
                print("error:")
                print(e)

            time.sleep(1)

    def uploadFileToCloud(self, file_path, file_name, oc):
        try:
            oc.login('zhangxu', 'zx@12346')

            dirname = time.strftime("%Y-%m-%d", time.localtime()) 
            dirname = dirname + '-%d' %(self.uploadCount/200)

            try:
                oc.mkdir(dirname)
            except Exception as e:
                print("mkdir failed:" + str(e))

            # upload
            oc.put_file(dirname + '/' + file_name, file_path)

            # logout
            oc.logout()
            print("uploadFileToCloud success, file:" + dirname + '/' + file_name)
            
        except Exception as e:
            print("uploadFileToCloud error:")
            print(e)


if __name__ == '__main__':
    douyin = CopyVideo()

