from selenium import webdriver
import time
import requests

# open chrome
browser = webdriver.Chrome()
browser.get("http://douyin.iiilab.com")

# input the source
# text = browser.find_element_by_css_selector('[class="form-control link-input"]')
text = browser.find_element_by_css_selector('input.form-control.link-input')
text.clear()
text.send_keys('http://v.douyin.com/duAERB/')

button = browser.find_element_by_css_selector('button.btn.btn-default')
button.click()

time.sleep(10)

# parse the data to get video url
result = browser.find_element_by_css_selector('a.btn.btn-success')
video_url = result.get_attribute('href')
print('标题：{} 超链接：{}'.format(result.text, result.get_attribute('href')))

# download video and save
try:
    response = requests.get(url=video_url, stream=True, timeout=10, verify=False)   # 开启流下载
    with open("save.mp4", 'wb') as f:
        for chunk in response.iter_content(1024):   # 返回迭代对象
            f.write(chunk)
        print('下载成功')
except:
    print('下载失败')

# at the end, quit
browser.quit()