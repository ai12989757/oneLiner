# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'install_UIcijPKg.ui'
##
## Created by: Qt User Interface Compiler version 5.15.2
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

class Ui_install(object):
    def setupUi(self, install):
        if not install.objectName():
            install.setObjectName(u"install")
        install.resize(250, 85)
        install.setMinimumSize(QSize(250, 85))
        install.setMaximumSize(QSize(250, 85))
        self.verticalLayout = QVBoxLayout(install)
        self.verticalLayout.setSpacing(2)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.verticalLayout.setContentsMargins(5, 5, 5, 5)

        self.lineEdit = QLineEdit(install)
        self.lineEdit.setObjectName(u"lineEdit")
        self.verticalLayout.addWidget(self.lineEdit)

        self.horizontalLayout = QHBoxLayout()
        self.horizontalLayout.setSpacing(2)
        self.horizontalLayout.setObjectName(u"horizontalLayout")
        self.pushButton_2 = QPushButton(install)
        self.pushButton_2.setObjectName(u"pushButton_2")
        self.pushButton_2.setMinimumSize(QSize(0, 30))
        self.pushButton_2.setMaximumSize(QSize(16777215, 30))
        self.horizontalLayout.addWidget(self.pushButton_2)

        self.pushButton = QPushButton(install)
        self.pushButton.setObjectName(u"pushButton")
        self.pushButton.setMinimumSize(QSize(0, 30))
        self.pushButton.setMaximumSize(QSize(16777215, 30))
        self.horizontalLayout.addWidget(self.pushButton)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.label = QLabel(install)
        self.label.setObjectName(u"label")
        self.label.setMinimumSize(QSize(0, 15))
        self.label.setMaximumSize(QSize(16777215, 10))
        self.verticalLayout.addWidget(self.label)

        self.retranslateUi(install)

        QMetaObject.connectSlotsByName(install)
    # setupUi

    def retranslateUi(self, install):
        install.setWindowTitle(QCoreApplication.translate("install", u"\u5b89\u88c5...", None))
        self.lineEdit.setPlaceholderText(QCoreApplication.translate("install", u"\u8bf7\u8f93\u5165\u4ee5\u8bbe\u5b9a\u5feb\u6377\u952e", None))
        self.pushButton_2.setText(QCoreApplication.translate("install", u"\u786e\u5b9a", None))
        self.pushButton.setText(QCoreApplication.translate("install", u"\u53d6\u6d88", None))
        self.label.setText(QCoreApplication.translate("install", u"\u70b9\u51fb\u53d6\u6d88\u53ef\u7a0d\u540e\u5728\u70ed\u952e\u7f16\u8f91\u5668\u91cc\u66f4\u6539\u5feb\u6377\u952e", None))
    # retranslateUi

