#!/usr/bin/env python
# -*- coding: utf-8 -*-


import json
import time
import uuid
from datetime import datetime


from Log import Logger
from Config import Config
from Sqlite import Sqlite
from capture import Capture

import flask
from werkzeug import secure_filename

# set the logger
logger = Logger(log_path=Config.logdir+'/server.log', log_level=Config.loglevel, log_name='Server')
#sqlite = Sqlite('yuming.db')
sqlite = Sqlite()

class Server:
    def __init__(self):
        # get the logger
        self.logger = logger
        self.logger.debug("logger test ok")

        # init sqlite
        self.sqlite = sqlite
        self.cap = Capture()
        self.logger.debug("init over")

    def get_name_info_all(self):
        return self.sqlite.get_available_yuming_infos_all()

    def blacklist_push(self, name):
        info = {}
        info['name'] = name
        info['blacklist'] = 'yes'
        return self.sqlite.update_yuming_info(info)

    def blacklist_pop(self, name):
        info = {}
        info['name'] = name
        info['blacklist'] = 'no'
        return self.sqlite.update_yuming_info(info)

    def yuming_follow(self, name):
        info = {}
        info['name'] = name
        info['follow'] = 'yes'
        return self.sqlite.update_yuming_info(info)

    def yuming_unfollow(self, name):
        info = {}
        info['name'] = name
        info['follow'] = 'no'
        return self.sqlite.update_yuming_info(info)

    def search_yuming_info(self, name):
        try:
            info = None
            info = self.cap.get_domain_name_info(name)
            return info
        except Exception,ex:
            logger.error("search_yuming_info error:%s" %(ex))
            return info






# get the server
server = Server()
app = flask.Flask(__name__)

# web
@app.route('/')
@app.route('/index')
def index():
    try:
        #return "Hello, this is Server."
        name_info_list = []
        name_info_list = server.get_name_info_all()
        return flask.render_template('index.html', name_all = name_info_list)
    except Exception,ex:
        logger.error("index error:%s" %(ex))
        return "Hello, this is Server."

@app.route('/search.html')
def search():
    try:
        #return "Hello, this is Server."
        name_info_list = []
        return flask.render_template('search.html', name_all = name_info_list)
    except Exception,ex:
        logger.error("search.html error:%s" %(ex))
        return "Hello, this is Server."


# api
@app.route('/api/gettime')
def gettime():
    return "%s" %(datetime.now())

@app.route('/api/blacklist/push', methods=['GET','POST'])
def api_blacklist_push():
    try:
        response = {}
        # get the task id that need be deleted
        name = flask.request.form.get('name','null')
        if name == 'null':
            logger.warning("not get param name")
            response['result'] = 'error'
        else:
            logger.debug("get param name=%s" %(name))
            # to delete the task data
            server.blacklist_push(name)
            response['result'] = 'success'

        return json.dumps(response)

    except Exception,ex:
        logger.error("api_blacklist_push error:%s" %(ex))
        response['result'] = 'error'
        response['message'] = '%s' %(ex)
        return json.dumps(response)

@app.route('/api/blacklist/pop', methods=['GET','POST'])
def api_blacklist_pop():
    try:
        response = {}
        # get the task id that need be deleted
        name = flask.request.form.get('name','null')
        if name == 'null':
            logger.warning("not get param name")
            response['result'] = 'error'
        else:
            logger.debug("get param name=%s" %(name))
            # to delete the task data
            server.blacklist_pop(name)
            response['result'] = 'success'

        return json.dumps(response)
    except Exception,ex:
        logger.error("api_blacklist_pop error:%s" %(ex))
        response['result'] = 'error'
        response['message'] = '%s' %(ex)
        return json.dumps(response)

@app.route('/api/yuming/follow', methods=['GET','POST'])
def api_yuming_follow():
    try:
        response = {}
        # get the task id that need be deleted
        name = flask.request.form.get('name','null')
        if name == 'null':
            logger.warning("not get param name")
            response['result'] = 'error'
        else:
            logger.debug("get param name=%s" %(name))
            # to delete the task data
            server.yuming_follow(name)
            response['result'] = 'success'

        return json.dumps(response)
    except Exception,ex:
        logger.error("api_yuming_follow error:%s" %(ex))
        response['result'] = 'error'
        response['message'] = '%s' %(ex)
        return json.dumps(response)

@app.route('/api/yuming/unfollow', methods=['GET','POST'])
def api_yuming_unfollow():
    try:
        response = {}
        # get the task id that need be deleted
        name = flask.request.form.get('name','null')
        if name == 'null':
            logger.warning("not get param name")
            response['result'] = 'error'
        else:
            logger.debug("get param name=%s" %(name))
            # to delete the task data
            server.yuming_unfollow(name)
            response['result'] = 'success'

        return json.dumps(response)
    except Exception,ex:
        logger.error("yuming_unfollow error:%s" %(ex))
        response['result'] = 'error'
        response['message'] = '%s' %(ex)
        return json.dumps(response)

@app.route('/api/yuming/search', methods=['GET','POST'])
def api_yuming_search():
    try:
        response = {}
        # get the task id that need be deleted
        name = flask.request.form.get('name','null')
        if name == 'null':
            logger.warning("not get param name")
            response['result'] = 'error'
        else:
            logger.debug("get param name=%s" %(name))
            
            # should check the name, should end of '.com' or '.net'
            info = server.search_yuming_info(name)
            response['result'] = 'success'
            response['info'] = info
        return json.dumps(response)

    except Exception,ex:
        logger.error("api_yuming_search error:%s" %(ex))
        response['result'] = 'error'
        response['message'] = '%s' %(ex)
        return json.dumps(response)


if __name__ == "__main__":
    logger.info("__main__")
    app.run('0.0.0.0', 9090, debug = True,  threaded = True)
