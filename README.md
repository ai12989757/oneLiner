# 界面

![image.png](https://cdn.nlark.com/yuque/0/2022/png/28934963/1652924727339-fd5a87b3-7ff0-479a-b8f0-bdee019d6b5a.png#clientId=u21dde9ea-4c25-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=24&id=u36a25ec2&margin=%5Bobject%20Object%5D&name=image.png&originHeight=24&originWidth=304&originalType=binary&ratio=1&rotation=0&showTitle=true&size=3417&status=done&style=none&taskId=u95c71dcf-f8ca-44a7-8b1c-432434398a5&title=%E5%A6%82%E5%90%8D%E6%89%80%E7%A4%BA%EF%BC%8C%E5%8F%AA%E6%9C%89%E4%B8%80%E8%A1%8C%EF%BC%8C%E6%9E%81%E7%AE%80&width=304 "如名所示，只有一行，极简")

## 下载地址

[oneLiner.zip](https://anonfiles.com/r1eaf2icy5/oneLiner_zip)

## 运行环境

windows
python3.x PySide2
maya2022 pymel-1.2.0

## 待优化

- [ ] 预览的列表框不能完美显示出选中的对象
- [ ] 添加对maya重名对象的预览支持
- [ ] 右击查看帮助改右边按钮/悬停，点击查看gif教程
- [ ] 安装时弹出窗口设置快捷键

## 使用教程

### 安装

- 打开文件目录，注意路径不要有特殊字母和中文
- 将install.mel文件拖拽进maya视图窗口
- 工具将自动添加到当前工具架上![icon.png](https://cdn.nlark.com/yuque/0/2022/png/28934963/1653010332896-2226b603-0a3e-44b9-b9cf-58629d0f9813.png#clientId=u880a299c-12f3-4&crop=0.1291&crop=0.1353&crop=0.8733&crop=0.8794&from=ui&height=21&id=a8zvm&margin=%5Bobject%20Object%5D&name=icon.png&originHeight=64&originWidth=64&originalType=binary&ratio=1&rotation=0&showTitle=false&size=715&status=done&style=none&taskId=ufad57041-42ba-482c-b55f-0cc0aee778f&title=&width=21)

**tips：推荐使用自定义快捷键，将工具架里的代码复制进热键编辑器里，指定一个快捷键**

### 具体操作

#### 基础操作

- 选中的对象执行脚本，选中对象的名称会预览在输入框下方，并实时更新

![01.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1652965696936-170e7deb-b9ce-4860-b5ab-ac11d33fb44d.gif#clientId=u9a1d0959-bb3b-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=u68d5d779&margin=%5Bobject%20Object%5D&name=01.gif&originHeight=208&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=160565&status=done&style=none&taskId=u1f3a0f2f-7d97-4786-a0eb-2e00ca939c2&title=%E5%9C%A8%E8%BE%93%E5%85%A5%E6%A1%86%E5%86%85%E7%9B%B4%E6%8E%A5%E8%BE%93%E5%85%A5%E8%A6%81%E5%8F%98%E6%9B%B4%E7%9A%84%E5%90%8D%E5%AD%97%EF%BC%8C%E6%96%B0%E5%90%8D%E5%AD%97%E5%B0%86%E5%9C%A8%E8%BE%93%E5%85%A5%E6%A1%86%E4%B8%8B%E6%96%B9%E9%A2%84%E8%A7%88%EF%BC%8C%E5%9B%9E%E8%BD%A6%E6%89%A7%E8%A1%8C%E9%87%8D%E5%91%BD%E5%90%8D "在输入框内直接输入要变更的名字，新名字将在输入框下方预览，回车执行重命名")

##### > 查找和替换

- 旧名字>新名字

![03_大于号.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1653003300264-164117df-ba36-40a0-8be3-ac8637d2644d.gif#clientId=u880a299c-12f3-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=bhGtc&margin=%5Bobject%20Object%5D&name=03_%E5%A4%A7%E4%BA%8E%E5%8F%B7.gif&originHeight=208&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=189712&status=done&style=none&taskId=ud95e5457-5182-4afb-b17e-764cbae3da9&title=%E5%8F%AF%E4%B8%8E%20%40%20%23%20%E4%B8%80%E8%B5%B7%E4%BD%BF%E7%94%A8 "可与 @ # 一起使用")

##### ! # @ 符号的使用 

- **!** 表示对象的旧名称，输入时键入 **! **操作用来代替旧名称

![02_感叹号.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1653003515473-b1d7d283-ef3b-40e3-881a-79ab94811e6b.gif#clientId=u880a299c-12f3-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=ud0fa7c37&margin=%5Bobject%20Object%5D&name=02_%E6%84%9F%E5%8F%B9%E5%8F%B7.gif&originHeight=208&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=97036&status=done&style=none&taskId=u47d44f07-cf37-4f97-be4e-d8870f6fbc7&title=%E5%8F%AF%E4%BB%A5%E5%9C%A8%E8%BE%93%E5%85%A5%E6%96%87%E6%9C%AC%E7%9A%84%E4%BB%BB%E4%BD%95%E4%BD%8D%E7%BD%AE%EF%BC%8C%E5%8F%AF%E4%BB%A5%E5%A4%9A%E6%AC%A1%E4%BD%BF%E7%94%A8%E5%8F%AF%E4%B8%8E%20%23%20%40%20%E4%B8%80%E8%B5%B7%E4%BD%BF%E7%94%A8 "可以在输入文本的任何位置，可以多次使用可与 # @ 一起使用")

- **#** 表示数字，根据选择对象的顺序排序

![01.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1652965696936-170e7deb-b9ce-4860-b5ab-ac11d33fb44d.gif#clientId=u9a1d0959-bb3b-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=E9Y7U&margin=%5Bobject%20Object%5D&name=01.gif&originHeight=208&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=160565&status=done&style=none&taskId=u1f3a0f2f-7d97-4786-a0eb-2e00ca939c2&title=%E5%8F%AF%E4%BB%A5%E9%94%AE%E5%85%A5%E5%A4%9A%E4%B8%AA%E7%94%A8%E6%9D%A5%E8%A1%A8%E7%A4%BA%E5%A4%9A%E4%B8%AA%E6%95%B0%E5%AD%97%E5%8F%AF%E4%B8%8E%20%21%20%40%20%E4%B8%80%E8%B5%B7%E4%BD%BF%E7%94%A8 "可以键入多个用来表示多个数字可与 ! @ 一起使用")

- **@** 表示字母，根据选择对象的顺序排序

![04_at.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1653004847682-d35257fe-71c6-487a-8262-27d3a00ce3b7.gif#clientId=u880a299c-12f3-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=ue4921293&margin=%5Bobject%20Object%5D&name=04_at.gif&originHeight=208&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=76420&status=done&style=none&taskId=u4ad604c5-0a02-4a06-9937-90e75504248&title=%E5%8F%AA%E8%83%BD%E9%94%AE%E5%85%A5%E4%B8%80%E6%AC%A1%E5%8F%AF%E4%B8%8E%20%21%20%23%20%E4%B8%80%E8%B5%B7%E4%BD%BF%E7%94%A8 "只能键入一次可与 ! # 一起使用")

##### /s /h 应用于选定/层级

- **/s** 模式默认，无需特殊标注
- **/h** 作用与所选对象及其层级，在末尾键入**/h**

![05_层级.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1653009187963-981de756-5f9f-466e-a596-9e3582ddaf8f.gif#clientId=u880a299c-12f3-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=u6418f12d&margin=%5Bobject%20Object%5D&name=05_%E5%B1%82%E7%BA%A7.gif&originHeight=208&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=175756&status=done&style=none&taskId=ue2bc8fbf-8e0e-4519-aff2-718a10040aa&title=%E5%8F%AF%E4%B8%8E%E5%85%B6%E4%BB%96%E7%AC%A6%E5%8F%B7%E4%B8%80%E8%B5%B7%E4%BD%BF%E7%94%A8 "可与其他符号一起使用")

##### + - -- 删除字符

- **+数字** 组合使用，从名字开端删除字符，**+**表示从前往后，**数字**表示删除几个字符
- **-数字** 组合使用，从名字结尾删除字符，**-**表示从后往前，**数字**表示删除几个字符
- **--数字** 组合使用，从结尾删除字符，**数字**表示删到只剩几个字符

![06_删除字符.gif](https://cdn.nlark.com/yuque/0/2022/gif/28934963/1653010002269-2d1e6af6-40d6-4a2b-8058-8231d56f2577.gif#clientId=u880a299c-12f3-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=uddb4d245&margin=%5Bobject%20Object%5D&name=06_%E5%88%A0%E9%99%A4%E5%AD%97%E7%AC%A6.gif&originHeight=210&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=6191458&status=done&style=none&taskId=ud0568dce-4e17-45dd-b920-04fa7aa862e&title=%E4%B8%8D%E5%8F%AF%E4%B8%8E%E5%85%B6%E4%BB%96%E7%AC%A6%E5%8F%B7%E4%B8%80%E8%B5%B7%E4%BD%BF%E7%94%A8 "不可与其他符号一起使用")

##### f: fs: fe: 附加功能

- **f: xxx** 选中所有含有**xxx**字符的对象，**xxx**为空则选择所有，回车后选中名字里含有**xxx**的对象
- **fs: xxx** 选中开头含有**xxx**字符的对象，**xxx**为空则选择所有，回车后选中开头含有**xxx**对象
- **fe: xxx** 选中结尾含有**xxx**字符的对象，**xxx**为空则选择所有，回车后选中结尾含有**xxx**对象

![07_附加功能.gif](<https://cdn.nlark.com/yuque/0/2022/gif/28934963/1653010018322-707c2a1e-5f34-42a7-8d03-ad5485c2f841.gif#clientId=u880a299c-12f3-4&crop=0&crop=0&crop=1&crop=1&from=ui&id=u30ff07ca&margin=%5Bobject%20Object%5D&name=07_%E9%99%84%E5%8A%A0%E5%8A%9F%E8%83%BD.gif&originHeight=210&originWidth=640&originalType=binary&ratio=1&rotation=0&showTitle=true&size=10429091&status=done&style=none&taskId=u057d21da-a78f-4800-a20a-132ad846c7b&title=%E5%8A%9F%E8%83%BD%E7%AD%89%E5%90%8C%E4%BA%8E%E5%91%BD%E4%BB%A4%20ls%20%22%2Axxx%2A%22%3B%20ls%20%22xxx%2A%22%3B%C2%A0%20ls%20%22%2Axxx%22%3B> "功能等同于命令 ls "*xxx*"; ls "xxx*";  ls "*xxx";")

##### 右击查看帮助

![image.png](https://cdn.nlark.com/yuque/0/2022/png/28934963/1652932809050-417f698c-0353-46a5-85b2-691971760d3e.png#clientId=u21dde9ea-4c25-4&crop=0.0108&crop=0.059&crop=0.9756&crop=0.9846&from=paste&height=376&id=QeIcX&margin=%5Bobject%20Object%5D&name=image.png&originHeight=390&originWidth=369&originalType=binary&ratio=1&rotation=0&showTitle=false&size=27003&status=done&style=none&taskId=udd45a5c5-5d1f-4b54-92cf-3280ba7dbcc&title=&width=356)

## 鸣谢

oneLiner原作者：
Fauzan Syabana
zansyabana@gmail.com
重命名功能代码沿用自原作者
oneLiner原文链接：
[https://www.highend3d.com/maya/script/oneliner-simple-renamer-tool-for-maya](https://www.highend3d.com/maya/script/oneliner-simple-renamer-tool-for-maya)
![image.png](https://cdn.nlark.com/yuque/0/2022/png/28934963/1652933330430-eac858e4-df0b-4b70-bf84-c4820688d024.png#clientId=u21dde9ea-4c25-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=129&id=ub7587bcf&margin=%5Bobject%20Object%5D&name=image.png&originHeight=129&originWidth=299&originalType=binary&ratio=1&rotation=0&showTitle=true&size=4229&status=done&style=none&taskId=ub5402b5e-ed6a-4618-a212-718ee1e11ba&title=%E5%8E%9FUI&width=299 "原UI")
