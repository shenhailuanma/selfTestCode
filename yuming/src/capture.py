#!/usr/bin/env python
# -*- coding: utf-8 -*-

import ConfigParser
import time
import json
import urllib2
import xml.dom.minidom
import sqlite3

from Log import Logger
from Config import Config
from Sqlite import Sqlite


logger = Logger(log_path=Config.logdir+'/capture.log', log_level=Config.loglevel, log_name='Capture')

class Capture:
    def __init__(self):

        # get the logger
        self.logger = logger
        self.logger.debug("logger test ok")

        # init sqlite
        self.sqlite = Sqlite()

        self.now_cap_type = None

        self.logger.debug("init over")

    def do_capture(self):
        try:
            self.logger.debug("do_capture begin")

            cap_cnt = 0

            for name_one in self.generate_domain_names():
                cap_cnt += 1
                name_info = self.get_domain_name_info(name_one)
                if name_info != None:
                    self.logger.info("[%s] name_info: %s" %(name_one, name_info))
                else:
                    self.logger.warning("[%s] get name info failed." %(name_one))

                if name_info != None and not name_info.has_key('flag'):
                    time.sleep(0.2)

            self.logger.info("do_capture over, cap_cnt=%s" %(cap_cnt))

        except Exception,ex:
            self.logger.error("do_capture error:%s" %(ex))

    def generate_domain_names(self):
        try:

            # pinyin
            if Config.cap_use_pinyin == 'yes':
                all_pinyin_elems = []
                all_pinyin_elems += Config.cap_pinyin_elem_a
                all_pinyin_elems += Config.cap_pinyin_elem_b
                all_pinyin_elems += Config.cap_pinyin_elem_c
                all_pinyin_elems += Config.cap_pinyin_elem_d
                all_pinyin_elems += Config.cap_pinyin_elem_e
                all_pinyin_elems += Config.cap_pinyin_elem_f
                all_pinyin_elems += Config.cap_pinyin_elem_g
                all_pinyin_elems += Config.cap_pinyin_elem_h
                all_pinyin_elems += Config.cap_pinyin_elem_i
                all_pinyin_elems += Config.cap_pinyin_elem_j
                all_pinyin_elems += Config.cap_pinyin_elem_k
                all_pinyin_elems += Config.cap_pinyin_elem_l
                all_pinyin_elems += Config.cap_pinyin_elem_m
                all_pinyin_elems += Config.cap_pinyin_elem_n
                all_pinyin_elems += Config.cap_pinyin_elem_o
                all_pinyin_elems += Config.cap_pinyin_elem_p
                all_pinyin_elems += Config.cap_pinyin_elem_q
                all_pinyin_elems += Config.cap_pinyin_elem_r
                all_pinyin_elems += Config.cap_pinyin_elem_s
                all_pinyin_elems += Config.cap_pinyin_elem_t
                all_pinyin_elems += Config.cap_pinyin_elem_u
                all_pinyin_elems += Config.cap_pinyin_elem_v
                all_pinyin_elems += Config.cap_pinyin_elem_w
                all_pinyin_elems += Config.cap_pinyin_elem_x
                all_pinyin_elems += Config.cap_pinyin_elem_y
                all_pinyin_elems += Config.cap_pinyin_elem_z

                for one in all_pinyin_elems:
                    yield "%s.com" %(one)
                for one in all_pinyin_elems:
                    for two in all_pinyin_elems:
                        yield "%s%s.com" %(one, two)
            '''
            if Config.cap_use_string == 'yes':
                use_string_list = Config.cap_string_list + Config.cap_number_list
                for one in use_string_list:
                    yield "%s.com" %(one)
                for one in use_string_list:
                    for two in use_string_list:
                        yield "%s%s.com" %(one, two)
                for one in use_string_list:
                    for two in use_string_list:
                        for three in use_string_list:
                            yield "%s%s%s.com" %(one, two, three)

                for one in Config.cap_string_list:
                    for two in Config.cap_string_list:
                        for three in Config.cap_string_list:
                            for four in Config.cap_string_list:
                                yield "%s%s%s%s.com" %(one, two, three, four)
            '''

            if True:
                end_list = ['cc','cn','com.cn','net','xyz','top','win']

                for end_one in end_list:
                    use_string_list = Config.cap_number_list
                    for one in use_string_list:
                        yield "%s.%s" %(one, end_one)
                    for one in use_string_list:
                        for two in use_string_list:
                            yield "%s%s.%s" %(one, two, end_one)
                    for one in use_string_list:
                        for two in use_string_list:
                            for three in use_string_list:
                                yield "%s%s%s.%s" %(one, two, three, end_one)
                    for one in use_string_list:
                        for two in use_string_list:
                            for three in use_string_list:
                                for four in use_string_list:
                                    yield "%s%s%s%s.%s" %(one, two, three, four, end_one)

            '''
            if Config.cap_use_number == 'yes':
                # one number
                for one in Config.cap_number_list:
                    yield "%s.com" %(one)
                # two numbers
                for one in Config.cap_number_list:
                    for two in Config.cap_number_list:
                        yield "%s%s.com" %(one, two)
                # three numbers
                for one in Config.cap_number_list:
                    for two in Config.cap_number_list:
                        for three in Config.cap_number_list:
                            yield "%s%s%s.com" %(one, two, three)
                # four numbers
                for one in Config.cap_number_list:
                    for two in Config.cap_number_list:
                        for three in Config.cap_number_list:
                            for four in Config.cap_number_list:
                                yield "%s%s%s%s.com" %(one, two, three, four)
                # five numbers
                for one in Config.cap_number_list:
                    for two in Config.cap_number_list:
                        for three in Config.cap_number_list:
                            for four in Config.cap_number_list:
                                for five in Config.cap_number_list:
                                    yield "%s%s%s%s%s.com" %(one, two, three, four, five)

            '''

            if Config.cap_use_english == 'yes':
                from english_words import wordMeaning
                self.now_cap_type = 'cap_use_english'
                use_string_list = wordMeaning
                for one in use_string_list:
                    if len(one) <= 6:
                        yield "%s.com" %(one)

        except Exception,ex:
            self.logger.error("get_domain_name_info error:%s" %(ex))


    def get_domain_name_info(self, domain_name, update=False):
  
        try:
            info = None

            # check the name if exist
            domain_name_info = self.sqlite.get_one_yuming_info(domain_name)
            if domain_name_info == None or update == True:
                
                state_info = self.get_domain_name_state_info(domain_name)
                if state_info != None:
                    info = {}
                    info['state'] = state_info
                    info['name']  = domain_name

                    detail_info = {}
                    if info['state'] != 'available':
                        detail_info = self.get_domain_name_detail_info(domain_name)
                        self.logger.debug("[%s] detail_info: %s" %(domain_name, json.dumps(detail_info)))
                        info.update(detail_info)

                    # insert data into database
                    if domain_name_info == None:
                        if self.now_cap_type == 'cap_use_english':
                            if len(info['name']) <= 10:
                                info['follow'] = 'yes'
                        self.sqlite.insert_one_yuming_info(info)

                else:
                    self.logger.warning("[%s] get state info failed." %(domain_name))

            else:
                info = domain_name_info
                info['flag'] = 'in_database'

            return info
        except Exception,ex:
            self.logger.error("[%s] get_domain_name_info error:%s" %(domain_name, ex))
            return None


    '''
        get_domain_name_state_info
        return: 0:name used, 1:name not used,  2: others
    '''
    def get_domain_name_state_info(self, domain_name):
        try:
            state = None

            api_url = 'http://panda.www.net.cn/cgi-bin/check.cgi?area_domain=' + domain_name
            response = self.http_get(api_url)
            if response != None:
                #self.logger.debug("response: %s" %(response))
                '''
                    the response data should be:
                        <property>
                        <returncode>200</returncode>
                        <key>fezha.com</key>
                        <original>210 : Domain name is available</original>
                        </property>
                    or 
                        <property>
                        <returncode>200</returncode>
                        <key>bi.com</key>
                        <original>211 : Domain exists</original>
                        </property>
                '''

                offset = response.find('<original>')
                if offset != -1:
                    self.logger.debug("[%s] offset: %s, msg:%s" %(domain_name, offset, response[offset+10:offset+13]))
                    if response[offset+10:offset+13] == '210':
                        state = "available"
                    else:
                        state = "used"

                else:
                    self.logger.debug("[%s]not find <original>" %(domain_name))

                '''
                response_xml = xml.dom.minidom.parseString(response)
                original = response_xml.documentElement.getElementsByTagName("original")[0]
                if original:
                    state_msg = original.childNodes[0].data
                    self.logger.debug("state_msg: %s" %(state_msg))
                    if state_msg == '210 : Domain name is available':
                        state = 1
                    elif state_msg == '211 : Domain exists':
                        state = 0
                    else:
                        state = 0

                else:
                    self.logger.debug("no original")
                '''


            return state
        except Exception,ex:
            self.logger.error("[%s] get_domain_name_state_info error:%s" %(domain_name, ex))
            return None

    '''
        get_domain_name_state_info
        return: 0:name used, 1:name not used,  2: others
    '''
    def get_domain_name_detail_info(self, domain_name):
        try:
            detail_info = {}

            api_url = 'http://tool.chinaz.com/domaindel/?wd=' + domain_name
            response = self.http_get(api_url)
            if response != None:
                #self.logger.debug("response: %s" %(response))
                '''
                    the response data should be:

                '''

                fint_str = '域名到期时间</div><div class="fr WhLeList-right"><span>'
                offset = response.find(fint_str)
                if offset != -1:
                    #self.logger.debug("offset: {0}".format(offset))
                    self.logger.debug("[{2}] offset: {0}, msg:{1}".format(offset, response[offset:offset+len(fint_str)+10], domain_name))

                    expiration_date = response[offset+len(fint_str):offset+len(fint_str)+10]
                    # check date format
                    if self.is_valid_date(expiration_date, domain_name):
                        detail_info['expiration_date'] = expiration_date

                else:
                    self.logger.warning("[%s] not find 域名到期时间" %(domain_name))

                fint_str = '域名删除时间</div><div class="fr WhLeList-right"><span>'
                offset = response.find(fint_str)
                if offset != -1:
                    self.logger.debug("[{2}] offset: {0}, msg:{1}".format(offset, response[offset:offset+len(fint_str)+10], domain_name))

                    delete_date = response[offset+len(fint_str):offset+len(fint_str)+10]
                    if self.is_valid_date(delete_date, domain_name):
                        detail_info['delete_date'] = delete_date
                else:
                    self.logger.warning("[%s] not find 域名删除时间" %(domain_name))

                fint_str = '域名创建时间</div><div class="fr WhLeList-right"><span>'
                offset = response.find(fint_str)
                if offset != -1:
                    self.logger.debug("[{2}] offset: {0}, msg:{1}".format(offset, response[offset:offset+len(fint_str)+10], domain_name))

                    registration_date = response[offset+len(fint_str):offset+len(fint_str)+10]
                    if self.is_valid_date(registration_date, domain_name):
                        detail_info['registration_date'] = registration_date
                else:
                    self.logger.warning("[%s] not find 域名创建时间" %(domain_name))


            return detail_info
        except Exception,ex:
            self.logger.error("[%s] get_domain_name_detail_info error:%s" %(domain_name, ex))
            return {}

    def is_valid_date(self, date_str, domain_name):
        try:
            time.strptime(date_str, "%Y-%m-%d")
            return True
        except Exception,ex:
            self.logger.error("[%s] is_valid_date(%s) error:%s" %(domain_name, date_str,ex))
            return False

    def http_post(self, url, data="", timeout=10):

        maxTryTimes = 3

        try_times = 0

        while try_times < maxTryTimes:
            try:
                headers = {}
                headers['Accept'] = '*/*'
                headers['User-Agent'] = 'Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.95 Safari/537.36'
                headers['Content-Type'] = 'application/json;text/plain;charset=UTF-8'
                request = urllib2.Request(url=url, data=data, headers=headers)
                
                req = urllib2.urlopen(request,timeout=timeout)
            except Exception,ex:
                try_times += 1
                self.logger.error("[http_post] try_times:%s, post data:%s." %(try_times,data))

                if try_times >= maxTryTimes:
                    self.logger.error("http_post error: %s" %(ex))
                    return None

                continue
            
            return req.read()

        return None

    def http_get(self, url, timeout=10):

        maxTryTimes = 1

        try_times = 0

        while try_times < maxTryTimes:
            try:
                headers = {}
                headers['Accept'] = '*/*'
                headers['User-Agent'] = 'Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.95 Safari/537.36'
                headers['Content-Type'] = 'application/json;text/plain;charset=UTF-8'
                request = urllib2.Request(url=url, data=None, headers=headers)
                
                req = urllib2.urlopen(request,timeout=timeout)
            except Exception,ex:
                try_times += 1
                self.logger.error("[http_get] try_times:%s." %(try_times))

                if try_times >= maxTryTimes:
                    self.logger.error("http_get error: %s" %(ex))
                    return None

                continue
            
            return req.read()

        return None


if __name__ == "__main__":

    logger.debug("main begin")
    cap = Capture()
    cap.do_capture()

    # test generate
    '''
    cnt = 0
    for one in cap.generate_domain_names():
        #logger.debug("name: %s, cnt = %s" %(one, cnt))
        cnt += 1
        print "name: %s, cnt = %s" %(one, cnt)
    '''

    logger.debug("main over")
