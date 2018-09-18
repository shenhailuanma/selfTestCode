from selenium import webdriver
import time
import requests
from selenium.webdriver.common.action_chains import ActionChains

# open chrome
browser = webdriver.Chrome()
browser.get("http://v.douyin.com/duAERB/")

time.sleep(1)

# find play button
playButton = browser.find_element_by_css_selector('div.play-btn')
playButton.click()

time.sleep(5)

# 此处涉及右键保存
# 参考：https://www.cnblogs.com/tobecrazy/p/3969390.html
player = browser.find_element_by_css_selector('video.player')

actions = ActionChains(browser)
actions.move_to_element(player)
actions.context_click()
actions.perform()
