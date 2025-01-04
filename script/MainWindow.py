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

from oneLiner import *

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
        self.lineEdit.customContextMenuRequested.connect(self.showHelpTips)      # 链接弹出菜单
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
            self.listView.setModel(QStringListModel(newNameView(text)))
        else:
            self.listView.setModel(QStringListModel(self.items))

    def runCommand(self):
        text = self.lineEdit.text()
        if text:
            self.lineEdit.clear()
            oneLiner(text)
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

    def showHelpTips(self):
        menu = QMenu()
        
        tip1 = menu.addAction('符号表示字符:')
        tip1.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))

        tip1.setIcon(QIcon(oneLinerPath+"images/icon/help.png"))
        # menu.addAction('执行命令').triggered.connect(self.runCommand) # 执行命令
        menu.addAction('! = 原名称(可以是名称的一部分)')
        menu.addAction('# = 代表数字, 可以多个##代表多个数字')
        menu.addAction('@ = 按选择的顺序用大写英文字母排序')
        menu.addSeparator()
        
        tip1 = menu.addAction('查找和替换:')
        tip1.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
        tip1.setIcon(QIcon(oneLinerPath+"images/icon/search.png"))
        menu.addAction('" 原名称 " > " 新名称 " (可以是名称的一部分)')
        menu.addSeparator()

        tip1 = menu.addAction('删除字符:')
        tip1.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
        tip1.setIcon(QIcon(oneLinerPath+"images/icon/clear.png"))
        menu.addAction('" + 数字 " 从开端删除几个字符')
        menu.addAction('" - 数字 " 从末尾删除几个字符')
        menu.addAction('" -- 数字 " 删到只剩几个字符(保留前面几个字符)')
        menu.addSeparator()

        tip1 = menu.addAction('作用于选定对象/选定对象的层级/所有:')
        tip1.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
        tip1.setIcon(QIcon(oneLinerPath+"images/icon/selection.png"))
        menu.addAction('"/s" 默认设置, 基于当前选择对象进行重命名')
        menu.addAction('"/h" 选择对象的所有层级进行重命名')
        menu.addAction('"/a" 作用于场景中的所有对象')
        menu.addSeparator()

        tip1 = menu.addAction('附加功能:')
        tip1.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
        tip1.setIcon(QIcon(oneLinerPath+"images/icon/ps.png"))
        menu.addAction('"f:xxx" 用法类似 ls "*name*" , 选中名字里含有xxx的对象')
        menu.addAction('"fs:xxx" 用法类似 ls "name*" , 选中名字前端是xxx的对象')
        menu.addAction('"fe:xxx" 用法类似 ls "*name" , 选中名字末尾是xxx的对象')
        menu.exec_(QCursor.pos())

if __name__ == "__main__":
    try:
        olUI.close()
        olUI.deleteLater()
    except:
        pass

    olUI = oneLinerUI()
    olUI.show()