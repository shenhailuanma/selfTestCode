#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
from datetime import datetime
from datetime import timedelta
import time

from Log import Logger
from Config import Config
from Sqlite import Sqlite
from Email import email
from capture import Capture

class Report:
    def __init__(self):
        # get the logger
        self.logger = Logger(log_path=Config.logdir+'/Report.log', log_level='debug', log_name='Report')
        self.logger.debug("logger test ok")

        # init sqlite
        self.sqlite = Sqlite()
        self.cap = Capture()
        self.logger.debug("init over")

    def get_available_name(self):
        try:
            result = self.sqlite.get_available_yuming_infos()
            return result
        except Exception,ex:
            self.logger.error("get_available_name error:%s" %(ex))
            return []

    def get_name_near_expiration_date(self):
        try:
            want_date = (datetime.now() + timedelta(days=5)).strftime("%Y-%m-%d")
            self.logger.info("get_name_near_expiration_date:%s" %(want_date))

            result = self.sqlite.get_yuming_infos_by_expiration_date(want_date)
            return result
        except Exception,ex:
            self.logger.error("get_name_near_expiration_date error:%s" %(ex))
            return []

    def do_report(self):
        try:
            report_string = '\n'
            # get available domain_names
            available_names = self.get_available_name()
            report_string = report_string + 'available_names:\n'
            for one_name in available_names:
                report_string = report_string + '\t%s\n' %(one_name)

            report_string = report_string + '\n\n'


            # update names
            update_names = [] + available_names

            # get domain_names which near expiration_date
            near_expiration_date_names = self.get_name_near_expiration_date()
            report_string = report_string + 'near_expiration_date:\n'
            for one_name in near_expiration_date_names:
                report_string = report_string + '\t%s\n' %(json.dumps(one_name))
                update_names.append(one_name['name'])
            report_string = report_string + '\n\n'

            # report 
            self.logger.info("report_string:%s" %(report_string))
            email.send_mail("zhangxu@tvie.com.cn", report_string, 'report')


        except Exception,ex:
            self.logger.error("do_report error:%s" %(ex))



if __name__ == "__main__":

    report = Report()

    report.do_report()
