#!/usr/bin/env python
# -*- coding: utf-8 -*-

# https://github.com/owncloud/pyocclient/

import owncloud

oc = owncloud.Client('http://39.107.241.50:18080')

oc.login('zhangxu', 'zx@12346')

try:
    oc.mkdir('testdir')
except Exception as e:
    print("mkdir failed:" + str(e))

oc.put_file('testdir/16.mp4', '/Users/zhangxu/douyin/16.mp4')

link_info = oc.share_file_with_link('testdir/16.mp4')

print("Here is your link: " + link_info.get_link())

