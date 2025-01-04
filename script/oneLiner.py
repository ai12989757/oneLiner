#!/usr/bin/python
# -*- coding: utf-8 -*-
#---------written by:----------------------
#-------Fauzan Syabana---------------------
#------zansyabana@gmail.com----------------
#Licensed under MIT License
#------------------------------------------
#----------change by:----------------------
#------------yibai-------------------------
#--------159689757@qq.com------------------
# Change Log: 
# - support python 3.x
# - create new UI
# - fix f:/fs:/fe: can't select all
# - - + don't rename shape name
# - fix - + can't rename shadingEngine node
#------------------------------------------

import maya.cmds as cmds

# region 撤回块
# 定义一个装饰器，用来存放撤回块
def undoBlock(func):
    def wrapper(*args, **kwargs):
        # 开始撤回块
        cmds.undoInfo(openChunk=True)
        # 执行函数
        func(*args, **kwargs)
        # 结束撤回块
        cmds.undoInfo(closeChunk=True)
    return wrapper

#selector
def selector(lookName):
    nameSplit = lookName.split(':')
    method = nameSplit[0]
    nameSel = nameSplit[1]
    if method == 'f':
        sel = cmds.ls("*{}*".format(nameSel), r=True)
    elif method == 'fe':
        sel = cmds.ls("*{}".format(nameSel), r=True)
    elif method == 'fs':
        sel = cmds.ls("{}*".format(nameSel), r=True)
    cmds.select(sel, ne=True)

@undoBlock
def oneLiner(nName, method='s'):

    # get selection method
    if nName.find(':') != -1:
        selector(nName)
    elif nName.find('/s') != -1:
        method = 's'
        nName = nName.replace('/s', '')
    elif nName.find('/h') != -1:
        method = 'h'
        nName = nName.replace('/h', '')
    elif nName.find('/a') != -1:
        if nName.find('>') != -1:
            method = 'a'
            print(method)
        nName = nName.replace('/a', '')

    if method == 's':
        slt = cmds.ls(selection=True)
    elif method == 'h':
        sltH = []
        slt = cmds.ls(selection=True)
        for i in slt:
            sltH.append(i)
            for child in reversed(cmds.listRelatives(i, ad=True, type='transform')):
                sltH.append(child)
        print(sltH)
        slt = sltH
    elif method == 'a':
        slt = cmds.ls()

    # find numbering replacement
    def numReplace(numName, idx, start=1):
        global padding
        alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        if numName.find('//') != -1:
            start = int(numName[numName.find('//') + 2:len(numName)])
            numName = numName.replace(numName[numName.find('//'):len(numName)], '')
            print(start)
        number = idx + start
        if numName.find('#') != -1:
            padding = numName.count('#')
            hastag = "{0:#>{1}}".format("#", padding)  # get how many '#' is in the new name
            num = "{0:0>{1}d}".format(number, padding)  # get number
            numName = numName.replace(hastag, num)
        # alphabetical numbering
        if numName.find('@') != -1:
            padding = numName.count('@')
            aSym = "{0:@>{1}}".format("@", padding) # get how many '@' is in the new name
            alphaNum = "{}".format(alpha[idx])  # get alphabetical number
            numName = numName.replace(aSym, alphaNum)

        return numName
    if nName.find(':') == -1:
        for i in slt:  # for every object in selection list
            # check if there is '>' that represents the replacement method
            curName = cmds.ls(i, long=True)[0].split('|')[-1]
            if nName.find('>') != -1:
                wordSplit = nName.split('>')
                oldWord = wordSplit[0]
                newWord = numReplace(wordSplit[1], slt.index(i))
                try:
                    cmds.rename(i, curName.replace(oldWord, newWord))
                except:
                    print("{} is not renamed".format(i))

            # check if the first character is '-' or '+', remove character method
            elif nName[0] == '-':
                if len(cmds.ls(curName, shapes=True)) == 0:
                    charToRemove = int(nName[1:len(nName)])
                    try:
                        cmds.rename(i, curName[0:-charToRemove])
                    except:
                        print("{} is not renamed".format(i))

            elif nName[0] == '+':
                if len(cmds.ls(curName, shapes=True)) == 0:
                    charToRemove = int(nName[1:len(nName)])
                    cmds.rename(i, curName[charToRemove:len(curName)])

            else:
                newName = numReplace(nName, slt.index(i))
                # get current Name if '!' mentioned
                print(i)
                if newName.find('!') != -1:
                    newName = newName.replace('!', curName)
                    print(newName)
                try:
                    cmds.rename(i, newName)
                except:
                    print("{} is not renamed".format(i))

def newNameView(nName, method='s'):
    changeName = []
    # get selection method
    if nName.find(':') != -1:
        pass
        #selector(nName)
    elif nName.find('/s') != -1:
        method = 's'
        nName = nName.replace('/s', '')
    elif nName.find('/h') != -1:
        method = 'h'
        nName = nName.replace('/h', '')
    elif nName.find('/a') != -1:
        if nName.find('>') != -1:
            method = 'a'
            #print(method)
        nName = nName.replace('/a', '')

    if method == 's':
        slt = cmds.ls(selection=True)
    elif method == 'h':
        sltH = []
        slt = cmds.ls(selection=True)
        for i in slt:
            sltH.append(i)
            for child in reversed(cmds.listRelatives(i, ad=True, type='transform')):
                sltH.append(child)
        #print(sltH)
        slt = sltH
    elif method == 'a':
        slt = cmds.ls()

    # find numbering replacement
    def numReplace(numName, idx, start=1):
        global padding
        alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        if numName.find('//') != -1:
            start = int(numName[numName.find('//') + 2:len(numName)])
            numName = numName.replace(numName[numName.find('//'):len(numName)], '')
            #print(start)
        number = idx + start
        if numName.find('#') != -1:
            padding = numName.count('#')
            hastag = "{0:#>{1}}".format("#", padding)  # get how many '#' is in the new name
            num = "{0:0>{1}d}".format(number, padding)  # get number
            numName = numName.replace(hastag, num)
        # alphabetical numbering
        if numName.find('@') != -1:
            padding = numName.count('@')
            aSym = "{0:@>{1}}".format("@", padding) # get how many '@' is in the new name
            alphaNum = "{}".format(alpha[idx])  # get alphabetical number
            numName = numName.replace(aSym, alphaNum)

        return numName
    if nName.find(':') == -1:
        for i in slt:  # for every object in selection list
            # check if there is '>' that represents the replacement method
            curName = cmds.ls(i, long=True)[0].split('|')[-1]
            if nName.find('>') != -1:
                wordSplit = nName.split('>')
                oldWord = wordSplit[0]
                newWord = numReplace(wordSplit[1], slt.index(i))
                newName = curName.replace(oldWord, newWord)
                changeName.append(newName)
            # check if the first character is '-' or '+', remove character method
            elif nName[0] == '-':
                charToRemove = int(nName[1:len(nName)])
                newName = curName[0:-charToRemove]
                changeName.append(newName)
            elif nName[0] == '+':
                charToRemove = int(nName[1:len(nName)])
                newName = curName[charToRemove:len(curName)]
                changeName.append(newName)
            else:
                newName = numReplace(nName, slt.index(i))
                # get current Name if '!' mentioned
                #print(i)
                if newName.find('!') != -1:
                    newName = newName.replace('!', curName)
                changeName.append(newName)
        
    return changeName

@undoBlock
def renamePastedPrefix():
    def getShortName(name):
        if '|' in name:
            return name.split('|')[-1]
        else:
            return name
        
    def getEndDigit(name):
        if 'pasted__' in name:
            name = name.replace("pasted__", "")
        digit = ''
        if name[-1].isdigit():
            while name[-1].isdigit():
                digit = digit + name[-1]
                name = name[:-1]
            return name,int(digit[::-1])
        else:
            return name, 0
    
    def renameAddDigit(name):
        if cmds.objExists(name):
            base_name = getShortName(name)
            base_name,digit = getEndDigit(base_name)
            while cmds.objExists(base_name + str(digit + 1)):
                digit += 1
            return base_name + str(digit + 1)

    while cmds.ls("pasted__*"):
        for i in cmds.ls("pasted__*"):
            if cmds.objExists(i):
                cmds.rename(i, renameAddDigit(i))
                
                    