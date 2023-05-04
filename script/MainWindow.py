#!/usr/bin/python
# -*- coding: utf-8 -*-
# changelog:
# 2023-05-04[11:14:00]:rebuild by yibai
# 修复重命名对象过多卡顿问题，只显示前19个选中对象的重命名结果，最后一行显示······

from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

from shiboken2 import wrapInstance
import maya.OpenMayaUI as omui
import maya.cmds as cmds

from main import Ui_Form
from oneLiner import *

import sys
sys.path
for i in sys.path:
    if 'oneLiner' in i:
        oneLinerPath = i.replace('script/','')

def maya_main_window():
    """
    Return the Maya main window widget as a Python object
    """
    main_window_ptr = omui.MQtUtil.mainWindow()
    try:
        return wrapInstance(long(main_window_ptr), QWidget)
    except:
        return wrapInstance(int(main_window_ptr), QWidget)
    # python3.x版本已删除long() 函数, 用int替代
    
class oneLinerUI(Ui_Form, QWidget):
    def __init__(self, parent=maya_main_window()):
        super(oneLinerUI, self).__init__(parent)
        self.setupUi(self)

        self.setAttribute(Qt.WA_TranslucentBackground)                           # 设置窗口背景透明
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Window)                  # 设置窗口无边框
        self.setGeometry(QCursor.pos().x()-40, QCursor.pos().y()-15, 320,20)     # 根据光标位置设置窗口位置
 
        # ListView Set
        self.lineEdit.returnPressed.connect(self.runCommand)                     # 回车键触发事件, 执行命令
        self.lineEdit.setContextMenuPolicy(Qt.CustomContextMenu)                 # 鼠标右击触发事件, 显示提示文本
        self.lineEdit.customContextMenuRequested.connect(self.showHelpTips)      # 链接弹出菜单
        self.lineEdit.setFocus()                                                 # 激活lineEdit输入框

        sel = cmds.ls(sl=True,fl=True)
        if len(sel) == 1:
            self.resize(304, 44)
            self.listView.setVisible(True)
            self.lineEdit.setPlaceholderText("输入以重命名, 右击查看帮助")
        elif len(sel) == 0:
            self.resize(304, 24)
            self.lineEdit.setPlaceholderText("请选择物体, 或键入f:/fs:/fe: + 字符 以选择相关对象")
            # 不显示listView
            self.listView.setVisible(False)
        elif len(sel) > 20:
            self.listView.setVisible(True)
            self.resize(330420, 20*(18-(0.03*len(sel)))+24)                # 根据选中对象的数量动态调整弹窗的大小
            self.lineEdit.setPlaceholderText("输入以重命名, 右击查看帮助")
        else:
            self.listView.setVisible(True)
            self.resize(330420, len(sel)*(18-(0.03*len(sel)))+24)                # 根据选中对象的数量动态调整弹窗的大小
            self.lineEdit.setPlaceholderText("输入以重命名, 右击查看帮助")
        self.lineEdit.textChanged.connect(self.changeList)                       # 输入框文本改变时，预览修改后的名称

        # ListView Set
        # 如果选中的物体数量大于20, 则显示只显示19个, 否则显示全部，且在最后一行添加···
        if len(sel) > 20:
            self.listView.setModel(QStringListModel(sel[0:19]+['······']))          # 将选中物体的名称添加到listView中
        else:
            self.listView.setModel(QStringListModel(sel))                            # 将选中物体的名称添加到listView中
        self.listView.setEditTriggers(QAbstractItemView.NoEditTriggers)          # 列表不可操作
        self.listView.setSelectionMode(QAbstractItemView.NoSelection)            # 不可选中
        self.listView.setFocusPolicy(Qt.NoFocus)                                 # 不可活动
        self.listView.setStyleSheet("QListView::item:hover { background: #3F7EA8;} QListView{ background: transparent;}") # 鼠标悬停背景颜色颜色
        self.listView.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)          # 关闭垂直滚动条
        
    def paintEvent(self, event):
        # 圆角
        painter = QPainter(self)
        painter.setRenderHint(painter.Antialiasing)                              # 抗锯齿
        painter.setBrush(QBrush(QColor(0,0,0,100)))                              # 设置画出边框的颜色,RGBA 255
        painter.setPen(QPen(QColor(0,0,0,100)))                                  # 设置画出边框的颜色,RGBA 255
        painter.drawRoundedRect( 0, 0, self.width(), self.height(), 5, 5);       # 圆角设置

    def event(self, eve):
        if eve.type() == QEvent.WindowDeactivate:
            self.close()
        return QWidget.event(self, eve)

    def changeList(self):
        text = self.lineEdit.text()
        if text:
            self.listView.setModel(QStringListModel(newNameView(text)))
            if len(newNameView(text)) == 1:
                self.resize(304, 44)
            else:
                if cmds.ls(sl=True):
                    if len(cmds.ls(sl=True,fl=True)) > 20:
                        self.resize(304, 20*(18-(0.03*len(newNameView(text))))+24)
                    else:
                        self.resize(304, len(newNameView(text))*(18-(0.03*len(newNameView(text))))+24) 
                else:
                    self.resize(304, 24)
        else:
            self.listView.setModel(QStringListModel(cmds.ls(sl=True)))

    def runCommand(self):
        text = self.lineEdit.text()
        if text:
            self.lineEdit.clear()
            oneLiner(text)
            self.listView.setModel(QStringListModel(cmds.ls(sl=True)))
            if len(cmds.ls(sl=True,fl=True)) == 1:
                self.resize(304, 44)
                self.listView.setVisible(True)
                self.lineEdit.setPlaceholderText("输入以重命名, 右击查看帮助") 
            elif len(cmds.ls(sl=True,fl=True)) == 0:
                self.resize(304, 24)
                self.lineEdit.setPlaceholderText("请选择物体, 或键入f:/fs:/fe: + 字符 以选择相关对象") 
            else:
                self.listView.setVisible(True)
                if len(cmds.ls(sl=True,fl=True)) > 20:
                    self.resize(304, 20*(18-(0.03*len(cmds.ls(sl=True,fl=True))))+24)
                else:
                    self.resize(304, len(cmds.ls(sl=True,fl=True))*(18-(0.03*len(cmds.ls(sl=True,fl=True))))+24)                    
                   
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