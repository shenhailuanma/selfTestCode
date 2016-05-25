#!/usr/bin/env python
# -*- coding: utf-8 -*-

class Config:
    '''
        this class define default configs
    '''
    # common
    loglevel = "info"
    logdir = '/usr/local/yuming/var/logs'
    logpath = '/usr/local/yuming/var/logs/yuming.log'
    sqlite_path = "/usr/local/yuming/data/yuming.db"
    sleep_time = 0.1

    # caputre mode
    cap_use_number = "no"
    cap_use_populer_number = "no"
    cap_use_string = "no"
    cap_use_pinyin = "no"
    cap_use_english = "no"

    # length limits
    cap_number_name_max = 99999
    cap_string_name_length = 3
    cap_string_number_name_length = 3

    # capture data
    cap_string_list = ['a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z']
    cap_number_list = ['0','1','2','3','4','5','6','7','8','9']

    cap_populer_numbers = ['51','52','91','92']
    cap_populer_english = ['love','live','baby','book']

    cap_pinyin_elem_a = ['a','ai','an','ang','ao']
    cap_pinyin_elem_b = ['ba','bai','ban','bang','bao','bei','ben','beng','bi','bian','biao','bie','bin','bing','bo','bu']
    cap_pinyin_elem_c = ['ca','cai','can','cang','cao','ce','cen','ceng','cha','chai','chan','chang','chao','che','chen','cheng','chi','chong','chou','chu','chua','chuai','chuan','chuang','chui','chun','chuo','ci','cong','cou','cu','cuan','cui','cun','cuo']
    cap_pinyin_elem_d = ['da','dai','dan','dang','dao','de','den','dei','deng','di','dia','dian','diao','die','ding','diu','dong','dou','du','duan','dui','dun','duo']
    cap_pinyin_elem_e = ['e','ei','en','eng','er']
    cap_pinyin_elem_f = ['fa','fan','fang','fei','fen','feng','fo','fou','fu']
    cap_pinyin_elem_g = ['ga','gai','gan','gang','gao','ge','gei','gen','geng','gong','gou','gu','gua','guai','guan','guang','gui','gun','guo']
    cap_pinyin_elem_h = ['ha','hai','han','hang','hao','he','hei','hen','heng','hong','hou','hu','hua','huai','huan','huang','hui','hun','huo']
    cap_pinyin_elem_i = []
    cap_pinyin_elem_j = ['ji','jia','jian','jiang','jiao','jie','jin','jing','jiong','jiu','ju','juan','jue','jun']
    cap_pinyin_elem_k = ['ka','kai','kan','kang','kao','ke','ken','keng','kong','kou','ku','kua','kuai','kuan','kuang','kui','kun','kuo']
    cap_pinyin_elem_l = ['la','lai','lan','lang','lao','le','lei','leng','li','lia','lian','liang','liao','lie','lin','ling','liu','long','lou','lu','lv','luan','lue','lun','luo']
    cap_pinyin_elem_m = ['ma','mai','man','mang','mao','me','mei','men','meng','mi','mian','miao','mie','min','ming','miu','mo','mou','mu']
    cap_pinyin_elem_n = ['na','nai','nan','nang','nao','ne','nei','nen','neng','ni','nian','niang','niao','nie','nin','ning','niu','nong','nou','nu','nv','nuan','nue','nuo']
    cap_pinyin_elem_o = ['o','ou']
    cap_pinyin_elem_p = ['pa','pai','pan','pang','pao','pei','pen','peng','pi','pian','piao','pie','pin','ping','po','pou','pu']
    cap_pinyin_elem_q = ['qi','qia','qian','qiang','qiao','qie','qin','qing','qiong','qiu','qu','quan','que','qun']
    cap_pinyin_elem_r = ['ran','rang','rao','re','ren','reng','ri','rong','rou','ru','ruan','rui','run','ruo']
    cap_pinyin_elem_s = ['sa','sai','san','sang','sao','se','sen','seng','sha','shai','shan','shang','shao','she','shei','shen','sheng','shi','shou','shu','shua','shuai','shuan','shuang','shui','shun','shuo','si','song','sou','su','suan','sui','sun','suo']
    cap_pinyin_elem_t = ['ta','tai','tan','tang','tao','te','teng','ti','tian','tiao','tie','ting','tong','tou','tu','tuan','tui','tun','tuo']
    cap_pinyin_elem_u = []
    cap_pinyin_elem_v = []
    cap_pinyin_elem_w = ['wa','wai','wan','wang','wei','wen','weng','wo','wu']
    cap_pinyin_elem_x = ['xi','xia','xian','xiang','xiao','xie','xin','xing','xiong','xiu','xu','xuan','xue','xun']
    cap_pinyin_elem_y = ['ya','yan','yang','yao','ye','yi','yin','ying','yo','yong','you','yu','yuan','yue','yun']
    cap_pinyin_elem_z = ['za','zai','zan','zang','zao','ze','zei','zen','zeng','zha','zhai','zhan','zhang','zhao','zhe','zhei','zhen','zheng','zhi','zhong','zhou','zhu','zhua','zhuai','zhuan','zhuang','zhui','zhun','zhuo','zi','zong','zou','zu','zuan','zui','zun','zuo']

    cap_pinyin_populer = ['ba','bi','bo','bu','ca','ci','cu','da','di','du','de','fa','fu','ga','gu','ji','ju','ka','ku','la','li','lu','le','mi','me','mu']


