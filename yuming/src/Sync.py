#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
from datetime import datetime
from datetime import timedelta
import time

from Log import Logger
from Config import Config
from Sqlite import Sqlite
from capture import Capture
from Email import Email

logger = Logger(log_path=Config.logdir+'/sync.log', log_level=Config.loglevel, log_name='sync')


class Sync:
    def __init__(self):
        self.logger = logger

        self.sqlite = Sqlite()
        self.cap = Capture()

        self.report_available_yuming = []

        self.logger.debug('init over')

    '''
        sync the following yuming that near expiration_date
    '''
    def sync_follow_yuming(self, dtday=1):
        try:

            expiration_date = (datetime.now() + timedelta(days=dtday)).strftime("%Y-%m-%d")

            # get data from database
            where = "expiration_date<='{0}' and follow='yes' ".format(expiration_date)
            yuming_list = self.sqlite.get_yuming_infos_by_where(where)

            self.logger.info('[sync_follow_yuming] yuming_list cnt:%s' %(len(yuming_list)))
            for one_yuming in yuming_list:
                self.logger.info('[sync_follow_yuming] yuming:%s' %(json.dumps(one_yuming)))
                self.sync_one_yuming_info(one_yuming)
                time.sleep(0.5)

            # get available follow yumings
            where = "state='available' and follow='yes' "
            
            yuming_list = self.sqlite.get_yuming_infos_by_where(where)
            self.logger.info('[sync_follow_yuming] get available follow yumings cnt:%s' %(len(yuming_list)))
            
            yuming_name_list = []
            for one_yuming in yuming_list:
                if one_yuming.has_key('name'):
                    yuming_name_list.append(one_yuming['name'])
                    
            # report
            self.do_report(yuming_name_list)

            self.logger.debug('sync_follow_yuming over')

        except Exception,ex:
            self.logger.error("sync_follow_yuming error:%s" %(ex))

    def sync_follow_delete_yuming(self, dtday=1):
        try:

            expiration_date = (datetime.now() + timedelta(days=dtday)).strftime("%Y-%m-%d")

            # get data from database
            where = "delete_date<='{0}' and follow='yes' ".format(expiration_date)
            yuming_list = self.sqlite.get_yuming_infos_by_where(where)

            self.logger.info('[sync_follow_delete_yuming] yuming_list cnt:%s' %(len(yuming_list)))
            for one_yuming in yuming_list:
                self.logger.info('[sync_follow_delete_yuming] yuming:%s' %(json.dumps(one_yuming)))
                self.sync_one_yuming_info(one_yuming)
                time.sleep(0.5)

            # get available follow yumings
            where = "state='available' and follow='yes' "
            
            yuming_list = self.sqlite.get_yuming_infos_by_where(where)
            self.logger.info('[sync_follow_delete_yuming] get available follow yumings cnt:%s' %(len(yuming_list)))
            
            yuming_name_list = []
            for one_yuming in yuming_list:
                if one_yuming.has_key('name'):
                    yuming_name_list.append(one_yuming['name'])
                    
            # report
            self.do_report(yuming_name_list)

            self.logger.debug('sync_follow_delete_yuming over')

        except Exception,ex:
            self.logger.error("sync_follow_delete_yuming error:%s" %(ex))

    def sync_near_expiration_date_yuming(self, dtday=1):
        try:

            expiration_date = (datetime.now() + timedelta(days=dtday)).strftime("%Y-%m-%d")

            # get data from database
            where = " blacklist='no' and expiration_date<='{0}' ".format(expiration_date)
            yuming_list = self.sqlite.get_yuming_infos_by_where(where)

            self.logger.info('[sync_near_expiration_date_yuming] yuming_list cnt:%s' %(len(yuming_list)))
            for one_yuming in yuming_list:
                self.logger.info('[sync_near_expiration_date_yuming] yuming:%s' %(json.dumps(one_yuming)))
                self.sync_one_yuming_info(one_yuming)
                time.sleep(0.5)

            self.logger.debug('sync_near_expiration_date_yuming over')

        except Exception,ex:
            self.logger.error("sync_near_expiration_date_yuming error:%s" %(ex))

    def sync_null_date_yuming(self, dtday=1):
        try:

            # get data from database
            where = "state='used' and  expiration_date is NULL"
            yuming_list = self.sqlite.get_yuming_infos_by_where(where)

            self.logger.info('[sync_null_date_yuming] yuming_list cnt:%s' %(len(yuming_list)))
            for one_yuming in yuming_list:
                self.logger.info('[sync_null_date_yuming] yuming:%s' %(json.dumps(one_yuming)))
                self.sync_one_yuming_info(one_yuming)
                time.sleep(0.5)

            self.logger.debug('sync_null_date_yuming over')

        except Exception,ex:
            self.logger.error("sync_null_date_yuming error:%s" %(ex))

    def sync_one_yuming_info(self, yuming_info):
        try:
            # get last yuming info
            info = self.cap.get_domain_name_info(yuming_info['name'], True)

            if info != None:
                if info.has_key('state') and info['state'] == 'available':
                    self.report_available_yuming.append(yuming_info['name'])

                change_cnt = 0
                # update info
                update_info = {}
                update_info['name'] = yuming_info['name']
                update_info['state'] = info['state']
                if yuming_info.has_key('state') and update_info['state'] != yuming_info['state']:
                    self.logger.info('yuming:state from %s change to %s.' %(yuming_info['state'],update_info['state']))
                    change_cnt += 1

                if info.has_key('registration_date') and info['registration_date'] != None:
                    update_info['registration_date'] = info['registration_date']
                    if yuming_info.has_key('registration_date') and update_info['registration_date'] != yuming_info['registration_date']:
                        self.logger.info('yuming:registration_date from %s change to %s.' %(yuming_info['registration_date'],update_info['registration_date']))
                        change_cnt += 1
                    
                if info.has_key('expiration_date') and info['expiration_date'] != None:
                    update_info['expiration_date'] = info['expiration_date']
                    if yuming_info.has_key('expiration_date') and update_info['expiration_date'] != yuming_info['expiration_date']:
                        self.logger.info('yuming:expiration_date from %s change to %s.' %(yuming_info['expiration_date'],update_info['expiration_date']))
                        change_cnt += 1

                if info.has_key('delete_date') and info['delete_date'] != None:
                    update_info['delete_date'] = info['delete_date']       
                    if yuming_info.has_key('delete_date') and update_info['delete_date'] != yuming_info['delete_date']:
                        self.logger.info('yuming:delete_date from %s change to %s.' %(yuming_info['delete_date'],update_info['delete_date']))
                        change_cnt += 1

                if change_cnt > 0:
                    self.sqlite.update_yuming_info(update_info)
                else:
                    self.logger.info('yuming:%s not change, no need to update.' %(yuming_info['name']))

        except Exception,ex:
            self.logger.error("sync_one_yuming_info error:%s" %(ex))

    def do_report(self, yumings):
        try:
            report_string = '\n'
            # get available domain_names
            available_names = yumings
            if len(available_names) > 0:
                report_string = report_string + 'follow available names:\n'
                for one_name in available_names:
                    report_string = report_string + '\t%s\n' %(one_name)

                report_string = report_string + '\n\n'
                

                smtp_server = 'smtp.sina.com'
                smtp_port = 465

                user_name = 'watchingmans@sina.com'
                user_passwd = 'WatchingMan'

                email = Email(smtp_server, smtp_port, user_name, user_passwd)
                # report 
                self.logger.info("report_string:%s" %(report_string))
                email.send_mail("445061875@qq.com", report_string, 'To buy')
                email.send_mail("zhangxu@tvie.com.cn", report_string, 'To buy')
        except Exception,ex:
            self.logger.error("do_report error:%s" %(ex))
