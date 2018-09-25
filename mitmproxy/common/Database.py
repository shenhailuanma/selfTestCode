#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
# import MySQLdb
import pymysql as MySQLdb
from datetime import datetime
import time
import logging

class Database:

    def __init__(self, host='localhost', user='root', passwd='', database='', port=9306):

        if host != None:
            self.mysqlHost = host
        else:
            self.mysqlHost = 'localhost'

        if  user != None:   
            self.mysqlUser = user
        else:
            self.mysqlUser = 'root'

        if  passwd != None:
            self.mysqlPassword = passwd
        else:
            self.mysqlPassword = ''

        if  database != None:
            self.mysqlDatabase = database
        else:
            self.mysqlDatabase = ''   

        try:
            # set the logger
            self.log_level = logging.DEBUG
            
            self.logger = logging.getLogger('Database')
            self.logger.setLevel(self.log_level)

            # create a handler for print the log info on console.
            ch = logging.StreamHandler()
            ch.setLevel(self.log_level)

            # set the log format
            formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
            ch.setFormatter(formatter)

            # add the handlers to logger
            self.logger.addHandler(ch)

            self.local_db = MySQLdb.connect(host=self.mysqlHost,
                user=self.mysqlUser, password=self.mysqlPassword, db=self.mysqlDatabase,
                charset="utf8mb4", port=port)

            self.logger.info('Database init over.')

        except Exception as ex:
            self.logger.info('Database error:%s.' %(ex))
            exit(-1)

    def __connect_to_db(self):

        try:

            # ping the db, check the connect if active
            self.local_db.ping()
        except Exception as ex:
            self.logger.warning("local_db timeout:%s, now to reconnect." %ex)
            self.local_db = MySQLdb.connect(host=self.mysqlHost,user=self.mysqlUser, passwd=self.mysqlPassword, db=self.mysqlDatabase,charset="utf8")


        # create a cursor
        cursor = self.local_db.cursor()

        cursor.execute("SET NAMES utf8")

        self.local_db.commit()

        return cursor


    def __escape_tuple(self, *input_tuple):
        '''
        [private] escape all args.
        '''
        escaped_arr = [];
        
        for i in input_tuple:
            escaped_arr.append(MySQLdb.escape_string(str(i)));
        
        return tuple(escaped_arr);


    def __add_one_name_and_value(self, inlist, name, value):
        one_in = {}
        one_in['name']  = name
        one_in['value'] = value
        inlist.append(one_in)




    def __string_set_list(self,set_list):

        try:
            string_set_list = ''

            '''
                set_list format:
                [{"name":"port", "value":9090},{"name":"server","value":"10.33.0.123"}]
            '''
            set_cnt = 0
            for set_one in set_list:
                if set_cnt == 0:
                    if isinstance(set_one['value'], str):
                        string_set_list = "{0}='{1}'".format(set_one['name'],set_one['value'])
                    else:
                        string_set_list = "{0}={1}".format(set_one['name'],set_one['value'])
                    set_cnt += 1
                else:
                    if isinstance(set_one['value'], str):
                        string_set_list = "{0},{1}='{2}'".format(string_set_list, set_one['name'], set_one['value'])
                    else:
                        string_set_list = "{0},{1}={2}".format(string_set_list, set_one['name'],set_one['value'])

            return 'success',string_set_list
        except Exception as ex:
            return 'error',str(ex)


    def __string_where_list(self,where_list):

        try:
            string_where_list = ''

            '''
                where_list format:
                [{"name":"port", "value":9090},{"name":"server","value":"10.33.0.123"}]
            '''
            where_cnt = 0
            for where_one in where_list:
                if where_cnt == 0:
                    if isinstance(where_one['value'], str):
                        string_where_list = "{0}='{1}'".format(where_one['name'],where_one['value'])
                    else:
                        string_where_list = "{0}={1}".format(where_one['name'],where_one['value'])
                    where_cnt += 1
                else:
                    if isinstance(where_one['value'], str):
                        string_where_list = "{0} and {1}='{2}'".format(string_where_list, where_one['name'], where_one['value'])
                    else:
                        string_where_list = "{0} and {1}={2}".format(string_where_list, where_one['name'],where_one['value'])

            return 'success',string_where_list
        except Exception as ex:
            return 'error',str(ex)

    def __string_value_list(self,value_list):

        try:
            string_name_list = ''
            string_value_list = ''

            '''
                value_list format:
                [{"name":"port", "value":9090},{"name":"server","value":"10.33.0.123"}]
            '''
            value_cnt = 0
            for value_one in value_list:

                if value_cnt == 0:
                    string_name_list = "{0}".format(value_one['name'])
                    if isinstance(value_one['value'], str):
                        string_value_list = "'{0}'".format(value_one['value'])
                    else:
                        string_value_list = "{0}".format(value_one['value'])
                    value_cnt += 1
                else:
                    string_name_list = "{0},{1}".format(string_name_list, value_one['name'])
                    if isinstance(value_one['value'], str):
                        string_value_list = "{0},'{1}'".format(string_value_list, value_one['value'])
                    else:
                        string_value_list = "{0}, {1}".format(string_value_list, value_one['value'])

            return 'success',string_name_list,string_value_list
        except Exception as ex:
            return 'error',str(ex),str(ex)



    def db_update_common(self, table_name, set_list, where_list):
        result = 'error'

        try:

            result,set_list_string = self.__string_set_list(set_list)
            if result != 'success':
                self.logger.error("db_update_common, __string_set_list error:%s" %set_list_string)
                return result,set_list_string    


            result,where_list_string = self.__string_where_list(where_list)
            if result != 'success':
                self.logger.error("db_update_common, __string_where_list error:%s" %where_list_string)
                return result,where_list_string  


            conn = None;
            conn = self.__connect_to_db()
            
            sql = "UPDATE {0} SET {1} WHERE {2};".format(table_name, set_list_string, where_list_string)

            self.logger.debug("[db_update_common]%s" %(sql))
            conn.execute(sql);
            self.logger.debug("[db_update_common] execute ")
            self.local_db.commit()

            self.logger.debug("[db_update_common] commit ")

            if conn is not None:
                conn.close(); 

            result = 'success'
            self.logger.debug("[db_update_common] over ")
            return result,'success'
        except Exception as ex:
            if conn is not None:
                conn.close()
            self.local_db.rollback()

            msg = "error:%s." %(str(ex))
            self.logger.warning("db_update_common, %s" %msg)
            return result,msg    

    def db_delete_common(self, table_name, where_list):
        result = 'error'

        try:

            result,where_list_string = self.__string_where_list(where_list)
            if result != 'success':
                self.logger.error("db_delete_common, __string_where_list error:%s" %where_list_string)
                return result,where_list_string  


            conn = None;
            conn = self.__connect_to_db()
            
            sql = "DELETE FROM {0} WHERE {1};".format(table_name,  where_list_string)

            self.logger.debug("[db_delete_common]%s" %(sql));
            conn.execute(sql);
            self.local_db.commit();

            if conn is not None:
                conn.close(); 

            result = 'success'
            return result,'success'
        except Exception as ex:
            if conn is not None:
                conn.close()
            self.local_db.rollback()

            msg = "error:%s." %(str(ex))
            self.logger.warning("db_delete_common, %s" %msg)
            return result,msg  

    def db_insert_common(self, table_name, value_list):
        result = 'error'

        try:

            result, name_list_string, value_list_string = self.__string_value_list(value_list)
            if result != 'success':
                self.logger.error("db_insert_common, __string_where_list error:%s" %name_list_string)
                return result,name_list_string  


            conn = None;
            conn = self.__connect_to_db()
            
            sql = "INSERT INTO {0}({1})  VALUES({2});".format(table_name, name_list_string, value_list_string)

            self.logger.debug("[db_insert_common]%s" %(sql));
            conn.execute(sql);
            self.logger.debug("[db_insert_common] after execute")
            self.local_db.commit();
            self.logger.debug("[db_insert_common] after commit")

            if conn is not None:
                conn.close(); 

            self.logger.debug("[db_insert_common] after close")
            result = 'success'
            return result,'success'
        except Exception as ex:

            self.local_db.rollback()

            msg = "error:%s." %(str(ex))
            self.logger.warning("db_insert_common, %s" %msg)
            return result,msg            

    def get_video_info(self, urlMd5):
        try:
            result = None
            conn = None
            conn = self.__connect_to_db()

            sql = "SELECT * FROM short_video WHERE urlMd5='{0}';".format(urlMd5)
            self.logger.info("[get_video_info] sql:%s" %(sql))

            conn.execute(sql)
            dataset = conn.fetchall()

            for row in dataset:
                result = row[0]

            if conn is not None:
                conn.close(); 
            
            return result

        except Exception as ex:
            msg = "error:%s." %(str(ex))
            self.logger.warning("get_video_info, %s" %msg)
            return None

    def get_video_info_by_md5(self, md5):
        try:
            result = None
            conn = None
            conn = self.__connect_to_db()

            sql = "SELECT * FROM short_video WHERE md5='{0}';".format(md5)
            self.logger.info("[get_video_info_by_md5] sql:%s" %(sql))

            conn.execute(sql)
            dataset = conn.fetchall()

            for row in dataset:
                result = row[0]

            if conn is not None:
                conn.close(); 
            
            return result

        except Exception as ex:
            msg = "error:%s." %(str(ex))
            self.logger.warning("get_video_info_by_md5, %s" %msg)
            return None


    def update_video_status(self, urlMd5, status):

        where_list = []
        self.__add_one_name_and_value(where_list, "urlMd5", urlMd5)

        set_list = []
        self.__add_one_name_and_value(set_list, "status", status)

        result,msg = self.db_update_common('short_video', set_list, where_list)
        return result,msg

    def update_video_md5(self, urlMd5, md5):

        where_list = []
        self.__add_one_name_and_value(where_list, "urlMd5", urlMd5)

        set_list = []
        self.__add_one_name_and_value(set_list, "status", status)

        result,msg = self.db_update_common('short_video', set_list, where_list)
        return result,msg


    def insert_video_info(self, platform=0, status=0, title='', url='', md5='', urlmd5='', storepath=''):

        # `platform` int DEFAULT 0 COMMENT 'platform: 0:undefine 1:douyin, 2:kuaishou, ...',
        # `status` int DEFAULT 0 COMMENT 'status: 0:init -1:unlike 1:used, ...',
        # `title` varchar(32)  DEFAULT '' COMMENT 'title',
        # `url` varchar(1000)  DEFAULT '' COMMENT 'video url',
        # `md5`  varchar(32)  DEFAULT '' COMMENT 'md5',
        # `urlmd5` varchar(32)  DEFAULT '' COMMENT 'urlmd5',

        if urlmd5 == None or len(urlmd5) != 32:
            return 'error','urlmd5 error'

        value_list = []
        self.__add_one_name_and_value(value_list, "platform", platform)
        self.__add_one_name_and_value(value_list, "status", status)
        self.__add_one_name_and_value(value_list, "title", title)
        self.__add_one_name_and_value(value_list, "url", url)
        self.__add_one_name_and_value(value_list, "md5", md5)
        self.__add_one_name_and_value(value_list, "urlmd5", urlmd5)
        self.__add_one_name_and_value(value_list, "storepath", storepath)

        result,msg = self.db_insert_common('short_video', value_list)
        return result,msg


