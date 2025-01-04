#!/usr/bin/python
# -*- coding: utf-8 -*-
# changelog:
# 2023-05-04[11:14:00]:rebuild by yibai
# 修复重命名对象过多卡顿问题，只显示前19个选中对象的重命名结果，最后一行显示······

try:
    from shiboken6 import wrapInstance
except ImportError:
    from shiboken2 import wrapInstance
try:
    from PySide6.QtCore import *
    from PySide6.QtGui import *
    from PySide6.QtWidgets import *
except ImportError:
    from PySide2.QtCore import *
    from PySide2.QtGui import *
    from PySide2.QtWidgets import *

import maya.OpenMayaUI as omui
import maya.cmds as cmds

import oneLiner
try:
    reload(oneLiner)
except:
    from importlib import reload
    reload(oneLiner)

oneLinerPath = __file__.replace("\\", "/").replace("script/MainWindow.py", "")

def maya_main_window():
    """
    Return the Maya main window widget as a Python object
    """
    main_window_ptr = omui.MQtUtil.mainWindow()
    return wrapInstance(int(main_window_ptr), QWidget)

    # python3.x版本已删除long() 函数, 用int替代

class oneLinerUI(QWidget):
    def __init__(self, parent=maya_main_window()):
        super(oneLinerUI, self).__init__(parent)

        # 初始化
        self.height = 24
        self.listViewItemHeight = 20
        self.oldPos = self.pos()
        self.dragging = False
        self.pw = QCursor.pos().x()-40
        self.ph = QCursor.pos().y()-15

        # UI Set
        self.setGeometry(self.pw , self.ph, 320, self.height)     # 根据光标位置设置窗口位置
        self.setAttribute(Qt.WA_TranslucentBackground)                           # 设置窗口背景透明
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Window)                  # 设置窗口无边框

        # ListView Set
        self.lineEdit = QLineEdit(self)                                          # 创建输入框
        self.lineEdit.returnPressed.connect(self.runCommand)                     # 回车键触发事件, 执行命令
        self.lineEdit.setContextMenuPolicy(Qt.CustomContextMenu)                 # 鼠标右击触发事件, 显示提示文本
        self.lineEdit.customContextMenuRequested.connect(self.toolsMenu)      # 链接弹出菜单
        self.lineEdit.setFocus()                                                 # 激活lineEdit输入框
        self.lineEdit.setGeometry(0, 0, 320, self.height)                        # 设置输入框的位置和大小
        self.lineEdit.mousePressEvent = self.mousePressEvent
        self.lineEdit.mouseMoveEvent = self.mouseMoveEvent
        self.lineEdit.mouseReleaseEvent = self.mouseReleaseEvent

        # ListView Set
        self.listView = QListView(self)                                          # 创建列表框
        self.listView.setGeometry(0, 24, 320, self.height)                       # 设置列表框的位置和大小
        self.listView.setEditTriggers(QAbstractItemView.NoEditTriggers)          # 列表不可操作
        self.listView.setSelectionMode(QAbstractItemView.NoSelection)            # 不可选中
        self.listView.setFocusPolicy(Qt.NoFocus)                                 # 不可活动
        self.listView.setStyleSheet("QListView::item:hover { background: #3F7EA8;} QListView{ background: transparent;}") # 鼠标悬停背景颜色颜色
        self.listView.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)          # 关闭垂直滚动条
        self.lineEdit.textChanged.connect(self.changeList)                      # 输入框文本改变时，预览修改后的名称

        self.listViewItemSet()                                                   # 设置列表框的内容

    def listViewItemSet(self):
        self.items = cmds.ls(sl=True,fl=True)
        if len(self.items) == 0:
            self.height
            self.listView.setVisible(False) 
            self.lineEdit.setPlaceholderText(u"请选择物体, 或键入f:/fs:/fe: + 字符 以选择相关对象")
        else:
            if len(self.items) >= 20:
                self.items = self.items[:19]
            self.height = 20*len(self.items)+24+4
            self.listViewItemHeight = 24*len(self.items)
            self.listView.setVisible(True)
            self.setGeometry(self.pw , self.ph, 320, self.height) 
            self.lineEdit.setPlaceholderText(u"请输入重命名规则, 右击查看帮助")
            self.listView.setGeometry(0, 24, 320, self.listViewItemHeight)
            self.listView.setModel(QStringListModel(self.items))

    def paintEvent(self, event):
        # 圆角
        painter = QPainter(self)
        painter.setBrush(QBrush(QColor(0,0,0,100)))                              # 设置画出边框的颜色,RGBA 255
        painter.setPen(QPen(QColor(0,0,0,100)))                                  # 设置画出边框的颜色,RGBA 255
        painter.drawRoundedRect( 0, 0, 320, self.height, 5, 5);                  # 圆角设置

    def changeList(self):
        text = self.lineEdit.text()
        if text:
            if text.startswith('f:') or text.startswith('fs:') or text.startswith('fe:'):
                self.listView.setModel(QStringListModel(oneLiner.newNameView(text)))
        else:
            self.listView.setModel(QStringListModel(self.items))

    def runCommand(self):
        text = self.lineEdit.text()
        if text:
            self.lineEdit.clear()
            oneLiner.oneLiner(text)
            self.listViewItemSet()

    def event(self, eve):
        if eve.type() == QEvent.WindowDeactivate:
            self.close()
        return QWidget.event(self, eve)
    
    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton:
            self.oldPos = event.globalPos()
            self.dragging = True

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.MiddleButton:
            if self.dragging:
                delta = QPoint(event.globalPos() - self.oldPos)
                self.move(self.x() + delta.x(), self.y() + delta.y())
                self.oldPos = event.globalPos()

    def mouseReleaseEvent(self, event):
        self.dragging = False
        if event.button() == Qt.MiddleButton:
            self.oldPos = event.globalPos()

    def toolsMenu(self):
        menu = QMenu()
        
        # 选择前缀 action
        selectPrefixAction = QAction()
        selectPrefixAction.setText(u'前缀为...')
        selectPrefixAction.triggered.connect(lambda: self.lineEdit.setText('fs:'))
        selectPrefixAction.setIcon(QIcon(oneLinerPath+"images/icon/Prefix.png"))
        menu.addAction(selectPrefixAction)
        # 选择后缀 action
        selectSuffixAction = QAction()
        selectSuffixAction.setText(u'后缀为...')
        selectSuffixAction.triggered.connect(lambda: self.lineEdit.setText('fe:'))
        selectSuffixAction.setIcon(QIcon(oneLinerPath+"images/icon/Suffix.png"))
        menu.addAction(selectSuffixAction)
        # 选择含有字符 action
        selectContainAction = QAction()
        selectContainAction.setText(u'中间包含...')
        selectContainAction.triggered.connect(lambda: self.lineEdit.setText('f:'))
        selectContainAction.setIcon(QIcon(oneLinerPath+"images/icon/Contain.png"))
        menu.addAction(selectContainAction)

        menu.addSeparator()

        removePastedAction = QAction()
        removePastedAction.setText(u'清除 pasted__ 前缀')
        removePastedAction.triggered.connect(oneLiner.renamePastedPrefix)
        removePastedAction.setIcon(QIcon(oneLinerPath+"images/icon/Recycle.png"))
        menu.addAction(removePastedAction)

        menu.addSeparator()

        helpAction = QAction()
        helpAction.setText(u'帮助')
        helpAction.triggered.connect(self.helpWindow)
        helpAction.setIcon(QIcon(oneLinerPath+"images/icon/help.png"))
        menu.addAction(helpAction)

        menu.exec_(QCursor.pos())

    def helpWindow(self):
        try:
            helfWin.close()
            helfWin.deleteLater()
        except:
            pass

        helfWin = helpUI()
        helfWin.show()

class helpUI(QWidget):
    def __init__(self, parent=maya_main_window()):
        super(helpUI, self).__init__(parent)
        
        if cmds.window('helpUI',q=True, ex=True):
            cmds.deleteUI('helpUI')

        self.setWindowTitle('帮助')
        self.setObjectName('helpUI')
        # 在鼠标位置显示窗口
        #self.setGeometry(QCursor.pos().x(), QCursor.pos().y(), 660, 300)
        # 固定窗口大小不可更改大小
        self.resize(682, 840)
        self.setWindowFlags(Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowTitleHint)
        self.setFixedSize(682, 840)

        self.layout = QVBoxLayout()
        self.layout.setAlignment(Qt.AlignTop)
        self.layout.setContentsMargins(0, 0, 0, 0)
        
        self.setLayout(self.layout)

        # 创建一个滚动区域
        self.scrollArea = QScrollArea(self)
        self.scrollArea.setWidgetResizable(True)
        self.scrollArea.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)  # 禁用横向滚动条
        # 取消边框显示
        self.scrollArea.setFrameShape(QFrame.NoFrame)
        # 取消滚动条显示
        self.scrollArea.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        # 边框距离
        self.scrollArea.setContentsMargins(0, 0, 0, 0)
        
        self.layout.addWidget(self.scrollArea)

        # 创建一个容器小部件
        self.container = QWidget()
        self.scrollArea.setWidget(self.container)

        # 容器小部件的布局
        self.containerLayout = QVBoxLayout()
        self.containerLayout.setAlignment(Qt.AlignTop)
        self.containerLayout.setSpacing(10)
        self.container.setLayout(self.containerLayout)

        # 标题
        self.title = QLabel(u'重命名规则帮助')
        self.title.setFont(QFont("Microsoft YaHei", 20, QFont.Bold))
        self.title.setAlignment(Qt.AlignCenter | Qt.AlignTop)
        self.containerLayout.addWidget(self.title)

        # 基础用法
        self.gifLabel1 = gifLabel(
            iconPath=[oneLinerPath + "images/01.gif"], 
            title1="[基础用法]", 
            title2=["选中的对象执行脚本，选中对象的名称会预览在输入框下方，并实时更新"]
            )
        self.containerLayout.addWidget(self.gifLabel1)

        # ! # @ 符号的使用
        self.gifLabel2 = gifLabel(
            iconPath=[oneLinerPath + "images/02.gif",
                      oneLinerPath + "images/03.gif",
                      oneLinerPath + "images/04.gif"], 
            title1="[! # @] 符号的使用", 
            title2=["[!] 表示对象的旧名称，输入时键入 [!] 操作用来代替旧名称",
                    "[#] 表示数字，根据选择对象的顺序排序",
                    "[@] 表示字母，根据选择对象的顺序排序"]
            )
        self.containerLayout.addWidget(self.gifLabel2)

        # /s /h 应用于选定/层级
        self.gifLabel3 = gifLabel(
            iconPath=[None,
                      oneLinerPath + "images/05.gif"], 
            title1="[/s /h] 应用于选定/层级", 
            title2=["[/s] 模式默认，无需特殊标注",
                    "[/h] 作用与所选对象及其层级，在末尾键入 [/h] ，[不要既选父对象又选子对象]"]
            )
        self.containerLayout.addWidget(self.gifLabel3)

        # + - -- 删除字符
        self.gifLabel4 = gifLabel(
            iconPath=[None,
                None,
                oneLinerPath + "images/06.gif"], 
            title1="[+ - --] 删除字符", 
            title2=["[+数字] 组合使用，从名字开端删除字符，[+] 表示从前往后，数字表示删除几个字符",
                    "[-数字] 组合使用，从名字结尾删除字符，[-] 表示从后往前，数字表示删除几个字符",
                    "[--数字] 组合使用，从结尾删除字符，数字表示删到只剩几个字符"]
            )
        self.containerLayout.addWidget(self.gifLabel4)

        # f: fs: fe: 选择包含字符
        self.gifLabel5 = gifLabel(
            iconPath=[None,
                      None,
                      oneLinerPath + "images/07.gif"], 
            title1="[f: fs: fe:] 选择包含字符", 
            title2=["[f:字符] 选择包含字符的对象",
                    "[fs:字符] 选择包含字符的对象并在前面加上字符",
                    "[fe:字符] 选择包含字符的对象并在后面加上字符"]
            )
        self.containerLayout.addWidget(self.gifLabel5)

        # 鸣谢
        self.thanks = gifLabel(
            iconPath=[
                None,
                oneLinerPath + "images/09.png",
                None,
                None],
            title1="鸣谢",
            title2=["[Fauzan Syabana]",
                    "zansyabana@gmail.com",
                    "重命名功能代码沿用自原作者",
                    "开源协议: [MIT]"
                    ]
            )
        self.containerLayout.addWidget(self.thanks)
            


class gifLabel(QFrame):
    def __init__(self, parent=None, **kwargs):
        super(gifLabel, self).__init__(parent)
        self.iconPath = kwargs.get('iconPath', None)
        self.title1 = kwargs.get('title1', None)
        self.title1 = self.setFontStyle(self.title1, "Chocolate")
        self.title2 = kwargs.get('title2', None)
        # 设置边框
        self.setStyleSheet("border: 0px solid #6a6a6a;")
        self.layout = QVBoxLayout()
        self.layout.setAlignment(Qt.AlignTop)
        #self.layout.setContentsMargins(2, 0, 2, 0)
        self.layout.setSpacing(10)
        self.setLayout(self.layout)

        self.title1Label = QLabel(self.title1)
        self.title1Label.setStyleSheet("border: 0px solid #3F7EA8;")
        self.title1Label.setFont(QFont("Microsoft YaHei", 15, QFont.Bold))
        self.title1Label.setAlignment(Qt.AlignCenter | Qt.AlignTop)
        self.layout.addWidget(self.title1Label,)
    
        frame = QFrame()
        frame.setStyleSheet("border: 3px solid #6a6a6a;")
        self.layout.setContentsMargins(0, 0, 0, 0)
        frame_layout = QVBoxLayout()
        frame.setLayout(frame_layout)
        for index, i in enumerate(self.title2):
            title_label = QLabel()
            # 将 i 中 [ ] 替换为 <font color=red> </font>
            i = self.setFontStyle(i, "LightSeaGreen")
            title_label.setText(i)
            title_label.setStyleSheet("border: 0px solid #3F7EA8;")
            title_label.setFont(QFont("Microsoft YaHei", 10))
            title_label.setAlignment(Qt.AlignCenter | Qt.AlignTop)
            frame_layout.addWidget(title_label)

            if self.iconPath[index] != None:
                # 添加GIF图片
                gif_label = QLabel()
                gif_label.setStyleSheet("border: 0px solid #3F7EA8;")
                if self.iconPath[index].endswith(".gif"):
                    gif_movie = QMovie(self.iconPath[index])
                    gif_label.setMovie(gif_movie)
                    gif_label.setScaledContents(True)
                    gif_movie.start()
                else:
                    gif_label.setPixmap(QPixmap(self.iconPath[index]))
                gif_label.setAlignment(Qt.AlignCenter | Qt.AlignTop)
                frame_layout.addWidget(gif_label)

        self.layout.addWidget(frame)

    def setFontStyle(self, font, color):
        font = font.replace("[", f"<b style='color:{color};'>")
        font = font.replace("]", "</b>")
        return font


if __name__ == "__main__":
    try:
        olUI.close()
        olUI.deleteLater()
    except:
        pass

    olUI = oneLinerUI()
    olUI.show()