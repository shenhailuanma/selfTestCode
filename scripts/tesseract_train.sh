#!/bin/bash

set -e

### 1. 准备样本图片(手动)
# 准备N个样本图片，并全部转换成tif文件


### 2. 合并样本图片(手动)
LANGNAME="mlang"
FONTNAME="mfont" 
# 打开jtessboxeditor，点击Tools->Merge Tiff ，按住shift键选择前文提到的N个tif文件，并把生成的tif合并到新目录
# 例如命名为：lang.fontname.exp0.tif
# 注意：lang为语言名称，fontname为字体名称, num为序号, 后续都会用到, 可以修改成适当的名字


### 3. 生成box文件(手动)
# 语法：tesseract [lang].[fontname].exp[num].tif [lang].[fontname].exp[num] batch.nochop makebox  
# lang为语言名称，fontname为字体名称，num为序号；在tesseract中，一定要注意格式。
#tesseract $LANGNAME.$FONTNAME.exp0.tif $LANGNAME.$FONTNAME.exp0 -l eng -psm 7 batch.nochop makebox
#tesseract mlang.mfont.exp0.tif mlang.mfont.exp0 -l eng -psm 7 batch.nochop makebox

### 4. 修改box文件(手动)
# 切换到jTessBoxEditor工具的Box Editor页，点击open，打开前面的tiff文件，工具会自动加载对应的box文件。
# 修改box文件


### 5. 生成font_properties
echo $FONTNAME 0 0 0 0 0 >font_properties


### 6. 生成训练文件
tesseract $LANGNAME.$FONTNAME.exp0.tif $LANGNAME.$FONTNAME.exp0 -l eng -psm 7 nobatch box.train
#tesseract $LANGNAME.$FONTNAME.exp0.tif $LANGNAME.$FONTNAME.exp0 -psm 7 nobatch box.train


### 7. 生成字符集文件
unicharset_extractor $LANGNAME.$FONTNAME.exp0.box



### 8. 生成shape文件
shapeclustering -F font_properties -U unicharset -O $LANGNAME.unicharset $LANGNAME.$FONTNAME.exp0.tr


### 9. 生成聚集字符特征文件
mftraining -F font_properties -U unicharset -O $LANGNAME.unicharset $LANGNAME.$FONTNAME.exp0.tr


### 10. 生成字符正常化特征文件
cntraining $LANGNAME.$FONTNAME.exp0.tr


### 11. 把步骤9，步骤10生成的特征文件进行更名
mv normproto $FONTNAME.normproto
mv inttemp $FONTNAME.inttemp
mv pffmtable $FONTNAME.pffmtable
mv unicharset $FONTNAME.unicharset
mv shapetable $FONTNAME.shapetable

### 12. 合并训练文件
combine_tessdata $FONTNAME.


### 13. 测试使用


